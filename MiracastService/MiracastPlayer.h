#ifndef _MIRACAST_PLAYER_H_
#define _MIRACAST_PLAYER_H_

#include <string>
#include <vector>
#include <gst/gst.h>
#include <glib.h>
#include <pthread.h>
#include <stdint.h>

class MiracastPlayer
{
public:
    static MiracastPlayer *getInstance();
    static void destroyInstance();
    bool launch();
    bool stop();
    bool pause();
    int getPlayerstate();
    bool setUri(std::string ipaddr, std::string port);
    std::string getUri();
    double getDuration();
    double getCurrentPosition();
    bool seekTo(double seconds);

private:
    GstElement *m_pipeline{nullptr};
    std::string m_uri;
    int m_bus_watch_id{-1};
    bool m_bBuffering;
    bool m_is_live;
    bool m_bReady;
    int m_buffering_level;
    double m_currentPosition;
    pthread_t m_playback_thread;

    static MiracastPlayer *mMiracastPlayer;
    MiracastPlayer();
    virtual ~MiracastPlayer();
    MiracastPlayer &operator=(const MiracastPlayer &) = delete;
    MiracastPlayer(const MiracastPlayer &) = delete;

    bool createPipeline();
    static gboolean busMessageCb(GstBus *bus, GstMessage *msg, gpointer user_data);
    bool changePipelineState(GstState state) const;

    static void *playbackThread(void *ctx);
    GMainLoop *m_main_loop{nullptr};
    GMainContext *m_main_loop_context{nullptr};
};

#endif /* MiracastPlayer_hpp */