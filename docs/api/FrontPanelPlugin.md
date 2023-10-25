<!-- Generated automatically, DO NOT EDIT! -->
<a name="FrontPanel_Plugin"></a>
# FrontPanel Plugin

**Version: [1.0.5](https://github.com/rdkcentral/rdkservices/blob/main/FrontPanel/CHANGELOG.md)**

A org.rdk.FrontPanel plugin for Thunder framework.

### Table of Contents

- [Abbreviation, Acronyms and Terms](#Abbreviation,_Acronyms_and_Terms)
- [Description](#Description)
- [Configuration](#Configuration)
- [Methods](#Methods)

<a name="Abbreviation,_Acronyms_and_Terms"></a>
# Abbreviation, Acronyms and Terms

[[Refer to this link](userguide/aat.md)]

<a name="Description"></a>
# Description

The `FrontPanel` plugin allows you to control brightness and power state for LEDs on a set-top box.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#Thunder)].

<a name="Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *org.rdk.FrontPanel*) |
| classname | string | Class name: *org.rdk.FrontPanel* |
| locator | string | Library name: *libWPEFrameworkFrontPanel.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="Methods"></a>
# Methods

The following methods are provided by the org.rdk.FrontPanel plugin:

FrontPanel interface methods:

| Method | Description |
| :-------- | :-------- |
| [getBrightness](#getBrightness) | Get the brightness of the specified LED or FrontPanel |
| [getClockBrightness](#getClockBrightness) | Returns the current clock brightness value |
| [getFrontPanelLights](#getFrontPanelLights) | Returns a list of supported Front Panel LEDs and their properties |
| [getPreferences](#getPreferences) | Returns the preferences that are saved in the `/opt/fp_service_preferences |
| [is24HourClock](#is24HourClock) | Gets the currently set clock mode (12 or 24 hour) |
| [powerLedOff](#powerLedOff) | Switches the specified LED off |
| [powerLedOn](#powerLedOn) | Switches the specified LED indicator on |
| [set24HourClock](#set24HourClock) | Sets the clock mode to either 12 or 24 hour |
| [setBlink](#setBlink) | Sets a blinking pattern for a particular LED indicator |
| [setBrightness](#setBrightness) | Sets the brightness of the specified LED indicator |
| [setClockBrightness](#setClockBrightness) | Sets the clock brightness |
| [setClockTestPattern](#setClockTestPattern) | Allows you to set a test pattern on the STB clock (`88 88`) |
| [setLED](#setLED) | Set preferences for the specified Front Panel LED indicator |
| [setPreferences](#setPreferences) | Sets preferences for Front Panel LED indicators which are saved to `/opt/fp_service_preferences |


<a name="getBrightness"></a>
## *getBrightness*

Get the brightness of the specified LED or FrontPanel.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.index | string | The index name of a front panel indicator. Possible values: `data_led`, `record_led`, `power_led` |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.brightness | integer | A brightness value from 0 - 100 |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.getBrightness",
    "params": {
        "index": "power_led"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "brightness": 50,
        "success": true
    }
}
```

<a name="getClockBrightness"></a>
## *getClockBrightness*

Returns the current clock brightness value.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.brightness | integer | A brightness value from 0 - 100 |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.getClockBrightness"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "brightness": 50,
        "success": true
    }
}
```

<a name="getFrontPanelLights"></a>
## *getFrontPanelLights*

Returns a list of supported Front Panel LEDs and their properties.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.supportedLights | array | A string [] of Front Panel lights that can be used as indices in other `FrontPanel` API functions |
| result.supportedLights[#] | string |  |
| result.supportedLightsInfo | object | A list of properties for each supported LED |
| result.supportedLightsInfo.power_led | object | A power LED indicator |
| result.supportedLightsInfo.power_led.range | string | Determines the types of values for the `min` and `max` arguments. Valid values are `boolean`: the `min` and `max` values are 0 (off) or 1 (on), or `int`: the `min` and `max` values are expressed as a brightness range |
| result.supportedLightsInfo.power_led.min | integer | Minimum value (off) |
| result.supportedLightsInfo.power_led.max | integer | Maximum value (LED is the brightest). In the case of a boolean range, 1 indicates that the LED is on |
| result.supportedLightsInfo.power_led.step | integer | Available only if the `range` argument is `int`. The Step size of brightness control. For example, an LED may have a range from 0 to 100 but with a step of 20 which means the LED would only support the values of 0, 20, 40, 60, 80, and 100 |
| result.supportedLightsInfo.power_led.colorMode | integer | The color mode setting. Valid values are: `0` - the LED color cannot be changed; `1` - the LED can be set to any color (using rgb-hex code); `2` - the LED can be set to an enumeration of colors as specified by the `colors` property |
| result.supportedLightsInfo.power_led?.colors | array | <sup>*(optional)*</sup> A string [] of available colors to set if `colorMode` is set to `2` |
| result.supportedLightsInfo.power_led?.colors[#] | string | <sup>*(optional)*</sup>  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.getFrontPanelLights"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "supportedLights": [
            "power_led"
        ],
        "supportedLightsInfo": {
            "power_led": {
                "range": "int",
                "min": 0,
                "max": 100,
                "step": 10,
                "colorMode": 2,
                "colors": [
                    "red"
                ]
            }
        },
        "success": true
    }
}
```

<a name="getPreferences"></a>
## *getPreferences*

Returns the preferences that are saved in the `/opt/fp_service_preferences.json` file.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.preferences | object | A list of properties for the LED |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.getPreferences"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "preferences": {},
        "success": true
    }
}
```

<a name="is24HourClock"></a>
## *is24HourClock*

Gets the currently set clock mode (12 or 24 hour).  
**Note:** On Xi6, this method always returns `false` despite having successfully set the clock to 24 hour mode.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.is24Hour | boolean | The mode of the clock. `true` for 24 hour clock, `false` for 12 hour clock |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.is24HourClock"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "is24Hour": false,
        "success": true
    }
}
```

<a name="powerLedOff"></a>
## *powerLedOff*

Switches the specified LED off.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.index | string | The index name of a front panel indicator. Possible values: `data_led`, `record_led`, `power_led` |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.powerLedOff",
    "params": {
        "index": "power_led"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="powerLedOn"></a>
## *powerLedOn*

Switches the specified LED indicator on. The LED must be powered on prior to setting its brightness.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.index | string | The index name of a front panel indicator. Possible values: `data_led`, `record_led`, `power_led` |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.powerLedOn",
    "params": {
        "index": "power_led"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="set24HourClock"></a>
## *set24HourClock*

Sets the clock mode to either 12 or 24 hour.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.is24Hour | boolean | The mode of the clock. `true` for 24 hour clock, `false` for 12 hour clock |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.set24HourClock",
    "params": {
        "is24Hour": false
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setBlink"></a>
## *setBlink*

Sets a blinking pattern for a particular LED indicator.  
**Note:** This API does not currently work nor does it provide a meaningful error status.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.blinkInfo | object |  |
| params.blinkInfo.ledIndicator | string | The index name of an LED indicator (`record_led`, `power_led`, `data_led`) |
| params.blinkInfo.iterations | integer | Defines the number of times to loop through the pattern array: a value of `0` indicates iterating through the array one time and is similar to a `do... while` loop; for values greater than `0`, the pattern is looped the specified amount of times; a value of `-1`, loops indefinitely until either `setLED`, `animate` or `setBrightness` is called or `setBlink` is called again |
| params.blinkInfo.pattern | array | An array of blink properties |
| params.blinkInfo.pattern[#] | object |  |
| params.blinkInfo.pattern[#].brightness | integer | A brightness value from 0 - 100 |
| params.blinkInfo.pattern[#].duration | integer | Step duration time in milliseconds |
| params.blinkInfo.pattern[#]?.color | string | <sup>*(optional)*</sup> The color which should be set if `colorMode` is set to `2` (see `getFrontPanelLights`) |
| params.blinkInfo.pattern[#]?.red | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |
| params.blinkInfo.pattern[#]?.green | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |
| params.blinkInfo.pattern[#]?.blue | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setBlink",
    "params": {
        "blinkInfo": {
            "ledIndicator": "power_led",
            "iterations": 10,
            "pattern": [
                {
                    "brightness": 50,
                    "duration": 100,
                    "color": "red",
                    "red": 0,
                    "green": 0,
                    "blue": 0
                }
            ]
        }
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setBrightness"></a>
## *setBrightness*

Sets the brightness of the specified LED indicator. If no indicator is specified, then FrontPanel all indicators are set.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.brightness | integer | A brightness value from 0 - 100 |
| params?.index | string | <sup>*(optional)*</sup> The index name of a front panel indicator. Possible values: `data_led`, `record_led`, `power_led` |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setBrightness",
    "params": {
        "brightness": 50,
        "index": "power_led"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setClockBrightness"></a>
## *setClockBrightness*

Sets the clock brightness.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.brightness | integer | A brightness value from 0 - 100 |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setClockBrightness",
    "params": {
        "brightness": 50
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setClockTestPattern"></a>
## *setClockTestPattern*

Allows you to set a test pattern on the STB clock (`88 88`).

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.show | boolean | If `true`, the test pattern is visible for the duration of the `timerInterval` value |
| params.timeInterval | integer | The duration, in seconds, indicating how long the test pattern displays |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setClockTestPattern",
    "params": {
        "show": true,
        "timeInterval": 4
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setLED"></a>
## *setLED*

Set preferences for the specified Front Panel LED indicator. Data are not validated in this call.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ledIndicator | string | The index name of an LED indicator (`record_led`, `power_led`, `data_led`) |
| params.brightness | integer | A brightness value from 0 - 100 |
| params?.color | string | <sup>*(optional)*</sup> The color which should be set if `colorMode` is set to `2` (see `getFrontPanelLights`) |
| params?.red | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |
| params?.green | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |
| params?.blue | integer | <sup>*(optional)*</sup> If `colorMode` is set to `0`, RGB values are used to set LED colors |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setLED",
    "params": {
        "ledIndicator": "power_led",
        "brightness": 50,
        "color": "red",
        "red": 0,
        "green": 0,
        "blue": 0
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="setPreferences"></a>
## *setPreferences*

Sets preferences for Front Panel LED indicators which are saved to `/opt/fp_service_preferences.json`. This function neither validates an input nor changes LED states (color, brightness). It's the users responsibility to provide valid and updated data.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.preferences | object | A list of properties for the LED |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.FrontPanel.setPreferences",
    "params": {
        "preferences": {}
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

