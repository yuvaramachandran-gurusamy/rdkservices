<!-- Generated automatically, DO NOT EDIT! -->
<a name="LoggingPreferencesPlugin"></a>
# LoggingPreferencesPlugin

**Version: [1.0.2](https://github.com/rdkcentral/rdkservices/blob/main/LoggingPreferences/CHANGELOG.md)**

A org.rdk.LoggingPreferences plugin for Thunder framework.

### Table of Contents

- [Abbreviation, Acronyms and Terms](#Abbreviation,_Acronyms_and_Terms)
- [Description](#Description)
- [Configuration](#Configuration)
- [Methods](#Methods)
- [Notifications](#Notifications)

<a name="Abbreviation,_Acronyms_and_Terms"></a>
# Abbreviation, Acronyms and Terms

[[Refer to this link](userguide/aat.md)]

<a name="Description"></a>
# Description

The `LoggingPreferences` plugin allows you to control key press logging on a set-top box.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#Thunder)].

<a name="Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *org.rdk.LoggingPreferences*) |
| classname | string | Class name: *org.rdk.LoggingPreferences* |
| locator | string | Library name: *libWPEFrameworkLoggingPreferences.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="Methods"></a>
# Methods

The following methods are provided by the org.rdk.LoggingPreferences plugin:

LoggingPreferences interface methods:

| Method | Description |
| :-------- | :-------- |
| [isKeystrokeMaskEnabled](#isKeystrokeMaskEnabled) | Gets logging keystroke mask status (enabled or disabled) |
| [setKeystrokeMaskEnabled](#setKeystrokeMaskEnabled) | Sets the keystroke logging mask property |


<a name="isKeystrokeMaskEnabled"></a>
## *isKeystrokeMaskEnabled*

Gets logging keystroke mask status (enabled or disabled).

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.keystrokeMaskEnabled | boolean | Whether keystroke mask is enabled (`true`) or disabled (`false`). If `true`, then any system that logs keystrokes must mask the actual keystroke being used. Default value is `false` |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.LoggingPreferences.isKeystrokeMaskEnabled"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "keystrokeMaskEnabled": false,
        "success": true
    }
}
```

<a name="setKeystrokeMaskEnabled"></a>
## *setKeystrokeMaskEnabled*

Sets the keystroke logging mask  If a keystroke mask is successfully changed, then this method triggers an `onKeystrokeMaskEnabledChange` 

### Events

| Event | Description |
| :-------- | :-------- |
| [onKeystrokeMaskEnabledChange](#onKeystrokeMaskEnabledChange) | Triggered if the keystroke mask is changed successfully |
### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.keystrokeMaskEnabled | boolean | Whether keystroke mask is enabled (`true`) or disabled (`false`). If `true`, then any system that logs keystrokes must mask the actual keystroke being used. Default value is `false` |

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
    "method": "org.rdk.LoggingPreferences.setKeystrokeMaskEnabled",
    "params": {
        "keystrokeMaskEnabled": false
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

<a name="Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#Thunder)] for information on how to register for a notification.

The following events are provided by the org.rdk.LoggingPreferences plugin:

LoggingPreferences interface events:

| Event | Description |
| :-------- | :-------- |
| [onKeystrokeMaskEnabledChange](#onKeystrokeMaskEnabledChange) | Triggered when the keystroke mask is changed |


<a name="onKeystrokeMaskEnabledChange"></a>
## *onKeystrokeMaskEnabledChange*

Triggered when the keystroke mask is changed.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.keystrokeMaskEnabled | boolean | Whether keystroke mask is enabled (`true`) or disabled (`false`). If `true`, then any system that logs keystrokes must mask the actual keystroke being used. Default value is `false` |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.onKeystrokeMaskEnabledChange",
    "params": {
        "keystrokeMaskEnabled": false
    }
}
```

