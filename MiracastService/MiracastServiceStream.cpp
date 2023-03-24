#include <gst/audio/audio.h>
#include <gst/app/gstappsink.h>

#include <map>
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <curl/curl.h>
#include <unistd.h>
#include <regex>

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "logger.h"


GMainLoop* gLoop;
//std::thread *m_gstThread;
static void on_pad_added(GstElement *gstelement, GstPad *pad, gpointer user_data)
//int GstBusCallback(GstBus *, GstMessage *message, gpointer data);

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *rtpmp2tdepay;
    GstElement *tsdemux;
    GstElement *vQueue;
    GstElement *videoFilter;
    GstElement *videoDecoder;
    GstElement *videoSink;
    GstElement *aQueue;
    GstElement *audioFilter;
    GstElement *audioDecoder;
    GstElement *audioSink;
} CustomData;

gboolean GstBusCallback(GstBus *bus, GstMessage *message, CustomData *data);

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  MIRACASTLOG_INFO ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    MIRACASTLOG_INFO ("%sANY\n", pfx);
    return;
  }
  if (gst_caps_is_empty (caps)) {
    MIRACASTLOG_INFO ("%sEMPTY\n", pfx);
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);

    MIRACASTLOG_INFO ("%s%s\n", pfx, gst_structure_get_name (structure));
    gst_structure_foreach (structure, print_field, (gpointer) pfx);
  }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  if (!pad) {
    MIRACASTLOG_ERROR ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  MIRACASTLOG_INFO("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}

static void pad_added_handler(GstElement *gstelement, GstPad *new_pad, CustomData *data)
{
    bool result = TRUE;
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    
    MIRACASTLOG_INFO("Entering..!!!");
    
    if(!data->pipeline) {
        MIRACASTLOG_ERROR("failed to link elements!");
        gst_object_unref(data->pipeline);
        data->pipeline = NULL;
        return;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);

    char *pad_name = gst_structure_get_name(new_pad_struct);

    MIRACASTLOG_INFO("Pad Name: %s", pad_name);

    if (strncmp (pad_name, "audio", strlen("audio")) == 0)
    {
        GstElement *sink = (GstElement *)data->aQueue;
        GstPad* sinkpad = gst_element_get_static_pad (sink, "sink");
        bool linked = GST_PAD_LINK_SUCCESSFUL(gst_pad_link (new_pad, sinkpad));
        if (!linked)
        {
            MIRACASTLOG_ERROR("Failed to link demux and audio pad (%s)", pad_name);
        }
        else
        {
            MIRACASTLOG_INFO("Configured audio pad");
        }
        gst_object_unref (sinkpad);
    }
    else if (strncmp (pad_name, "video", strlen("video")) == 0)
    {
        GstElement *sink = (GstElement *)data->vQueue;
        GstPad* sinkpad = gst_element_get_static_pad (sink, "sink");
        bool linked = GST_PAD_LINK_SUCCESSFUL(gst_pad_link (new_pad, sinkpad));
        if (!linked)
        {
            MIRACASTLOG_ERROR("Failed to link demux and video pad (%s)", pad_name);
        }
        else
        {
            MIRACASTLOG_INFO("Configured video pad");
        }
        gst_object_unref (sinkpad);
    }
    MIRACASTLOG_INFO("Exiting..!!!");
}

bool createPipeline() {
    CustomData data;
    GstStateChangeReturn ret;

    MIRACASTLOG_INFO("Entering..!!!");
    
    gst_init(NULL, NULL);
    
    MIRACASTLOG_TRACE("Creating Pipeline...");
    
    data.pipeline = gst_pipeline_new("miracast-player");
    if (!data.pipeline) { MIRACASTLOG_ERROR("Failed to create gstreamer pipeline"); return false; }
    
    data.source = gst_element_factory_make("udpsrc", "udpsrc");
    data.rtpmp2tdepay = gst_element_factory_make("rtpmp2tdepay", "rtpmp2tdepay");
    data.tsdemux = gst_element_factory_make("tsdemux", "tsdemux");
    data.vQueue = gst_element_factory_make("queue", "vQueue");
    data.videoFilter = gst_element_factory_make("brcmvidfilter", "brcmvidfilter");
    data.videoDecoder = gst_element_factory_make("brcmvideodecoder", "brcmvideodecoder");
    data.videoSink = gst_element_factory_make("brcmvideosink", "brcmvideosink");
    data.aQueue = gst_element_factory_make("queue", "aQueue");
    data.audioFilter = gst_element_factory_make("brcmaudfilter", "brcmaudfilter");
    data.audioDecoder = gst_element_factory_make("brcmaudiodecoder", "brcmaudiodecoder");
    data.audioSink = gst_element_factory_make("brcmaudiosink", "brcmaudiosink");

    if(!data.source) MIRACASTLOG_ERROR("Could not load udpsrc element.");
    if(!data.rtpmp2tdepay) MIRACASTLOG_ERROR("Could not load rtpmp2tdepay element.");
    if(!data.tsdemux) MIRACASTLOG_ERROR("Could not load tsdemux element.");
    if(!data.vQueue) MIRACASTLOG_ERROR("Could not load queue element.");
    if(!data.videoFilter) MIRACASTLOG_ERROR("Could not load brcmvidfilter element.");
    if(!data.videoDecoder) MIRACASTLOG_ERROR("Could not load brcmvideodecoder element.");
    if(!data.videoSink) MIRACASTLOG_ERROR("Could not load brcmvideosink element.");
    if(!data.aQueue) MIRACASTLOG_ERROR("Could not load queue element.");
    if(!data.audioFilter) MIRACASTLOG_ERROR("Could not load brcmaudfilter element.");
    if(!data.audioDecoder) MIRACASTLOG_ERROR("Could not load brcmaudiodecoder element.");
    if(!data.audioSink) MIRACASTLOG_ERROR("Could not load brcmaudiosink element.");
    
    if(!data.source || !data.rtpmp2tdepay || !data.tsdemux || 
    !data.vQueue || !data.videoFilter || !data.videoDecoder || !data.videoSink || 
    !data.aQueue || !data.audioFilter || !data.audioDecoder || !data.audioSink)
    {
        return false;
    } 

    MIRACASTLOG_INFO("Add all the elements to the Pipeline ");

    /* Add all the elements into the pipeline */
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.rtpmp2tdepay, data.tsdemux, data.vQueue, data.videoFilter, data.videoDecoder, data.videoSink, data.aQueue, data.audioFilter, data.audioDecoder, data.audioSink, NULL);
    
    MIRACASTLOG_INFO("Link all the emenets together. ");
    /* Link the elements together */
    if(!gst_element_link_many (data.source, data.rtpmp2tdepay, data.tsdemux, NULL))
    {
        MIRACASTLOG_ERROR ("Elements (rtpmp2tdepay & tsdemux) could not be linked.\n");
        gst_object_unref (data.pipeline);
        return false;
    } 
    if(!gst_element_link_many(data.vQueue, data.videoFilter, data.videoDecoder, data.videoSink, NULL))
    {
        MIRACASTLOG_ERROR ("Elements (Video) could not be linked.\n");
        gst_object_unref (data.pipeline);
        return false;
    }
    
    if(!gst_element_link_many(data.aQueue, data.audioFilter, data.audioDecoder, data.audioSink, NULL))
    {
        MIRACASTLOG_ERROR ("Elements (Audio) could not be linked.\n");
        gst_object_unref (data.pipeline);
        return false;
    }
    print_pad_capabilities(data.source, "sink");
    print_pad_capabilities(data.rtpmp2tdepay, "sink");

    MIRACASTLOG_INFO("Set the Port to udp source.");
    /* Set the udpsrc Port to stream the data */
    g_object_set(G_OBJECT(data.source), "port", 1990, NULL);
    GstCaps *caps = gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "video", NULL); 
    g_object_set(data.source, "caps", caps, NULL);

    print_pad_capabilities(data.tsdemux, "sink");
    MIRACASTLOG_INFO("Connect to the pad-added signal.");
    /* Connect to the pad-added signal */
    g_signal_connect(data.tsdemux, "pad-added", G_CALLBACK(pad_added_handler), (gpointer)&data);
    
    MIRACASTLOG_INFO("Listen to the bus.");
     /* Listen to the bus */
    GstBus *bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_watch(bus, GstBusCallback, &data);
    gst_object_unref(bus);

    MIRACASTLOG_INFO("Start Playing");
    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MIRACASTLOG_INFO("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
        return false;
    }
    MIRACASTLOG_INFO("Exiting..!!!");
    return true;
}

void GStreamerThreadFunc(void *) 
{
    MIRACASTLOG_INFO("Starting GStreamerThread");
    
    if(createPipeline() == false){
        MIRACASTLOG_ERROR("Failed to create GStreamer Pipeline.");
    }
}

gboolean GstBusCallback(GstBus *bus, GstMessage *message, CustomData *data) {
    GError* error = NULL;
    gchar* debug = NULL;

    if(!data->pipeline) {
        MIRACASTLOG_WARNING("NULL Pipeline");
        return false;
    }

    switch (GST_MESSAGE_TYPE(message)){
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &error, &debug);
            MIRACASTLOG_ERROR("error! code: %d, %s, Debug: %s", error->code, error->message, debug);
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "error-pipeline");
            break;

        case GST_MESSAGE_WARNING:
            gst_message_parse_warning(message, &error, &debug);
            MIRACASTLOG_WARNING("warning! code: %d, %s, Debug: %s", error->code, error->message, debug);
            break;

        case GST_MESSAGE_EOS:
            MIRACASTLOG_INFO("EOS message received");
            break;

        case GST_MESSAGE_STATE_CHANGED:
            gchar* filename;
            GstState oldstate, newstate, pending;
            gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);

            MIRACASTLOG_VERBOSE("%s old_state %s, new_state %s, pending %s",
                    GST_MESSAGE_SRC_NAME(message) ? GST_MESSAGE_SRC_NAME(message) : "",
                    gst_element_state_get_name (oldstate), gst_element_state_get_name (newstate), gst_element_state_get_name (pending));

            if (GST_ELEMENT(GST_MESSAGE_SRC(message)) != data->pipeline)
                break;

            filename = g_strdup_printf("%s-%s", gst_element_state_get_name(oldstate), gst_element_state_get_name(newstate));
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, filename);
            g_free(filename);
            break;

        default:
            MIRACASTLOG_WARNING("Msg Src=%s, Type=%s", GST_MESSAGE_SRC_NAME(message), GST_MESSAGE_TYPE_NAME(message));
            break;
    }

    if(error)
        g_error_free(error);

    if(debug)
        g_free(debug);

    return true;
}

#if 0
int main() {
    gLoop = g_main_loop_new(g_main_context_default(), FALSE);
    gst_init(NULL, NULL);

#if 1
    g_timeout_add_seconds(0, [](void *) -> int {
            createPipeline(); return G_SOURCE_REMOVE;
        }, NULL);
    g_timeout_add_seconds(3, [](void *) -> int {
            gst_element_set_state(m_pipeline, GST_STATE_PLAYING); return G_SOURCE_REMOVE;
        }, NULL);
//    g_timeout_add_seconds(8, [](void *) -> int {
//            gst_element_set_state(m_pipeline, GST_STATE_PAUSED); return G_SOURCE_REMOVE;
//        }, NULL);
//    g_timeout_add_seconds(10, [](void *) -> int {
//            gst_element_set_state(m_pipeline, GST_STATE_PLAYING); return G_SOURCE_REMOVE;
//        }, NULL);
#else
    m_gstThread = new std::thread([]{
            GStreamerThreadFunc(NULL);
    });
#endif

    g_main_loop_run(gLoop);
}
#endif
