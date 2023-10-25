<!-- Generated automatically, DO NOT EDIT! -->
<a name="OpenCDMi_Plugin"></a>
# OpenCDMi Plugin

**Version: [1.0.3](https://github.com/rdkcentral/rdkservices/blob/main/OpenCDMi/CHANGELOG.md)**

A OCDM plugin for Thunder framework.

### Table of Contents

- [Abbreviation, Acronyms and Terms](#Abbreviation,_Acronyms_and_Terms)
- [Description](#Description)
- [Configuration](#Configuration)
- [Properties](#Properties)

<a name="Abbreviation,_Acronyms_and_Terms"></a>
# Abbreviation, Acronyms and Terms

[[Refer to this link](userguide/aat.md)]

<a name="Description"></a>
# Description

The `OpenCDMi` plugin allows you view Open Content Decryption Module (OCDM) properties.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#Thunder)].

<a name="Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *OCDM*) |
| classname | string | Class name: *OCDM* |
| locator | string | Library name: *libWPEFrameworkOCDM.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.location | string | <sup>*(optional)*</sup> The location |
| configuration?.connector | string | <sup>*(optional)*</sup> The connector |
| configuration?.sharepath | string | <sup>*(optional)*</sup> The sharepath |
| configuration?.sharesize | string | <sup>*(optional)*</sup> The sharesize |
| configuration?.systems | array | <sup>*(optional)*</sup> A list of key systems |
| configuration?.systems[#] | object | <sup>*(optional)*</sup> System properties |
| configuration?.systems[#]?.name | string | <sup>*(optional)*</sup> Property name |
| configuration?.systems[#]?.designators | array | <sup>*(optional)*</sup> designator |
| configuration?.systems[#]?.designators[#] | object | <sup>*(optional)*</sup> System properties |
| configuration?.systems[#]?.designators[#].name | string | Property name |

<a name="Properties"></a>
# Properties

The following properties are provided by the OCDM plugin:

OCDM interface properties:

| Property | Description |
| :-------- | :-------- |
| [drms](#drms) <sup>RO</sup> | Supported DRM systems |
| [keysystems](#keysystems) <sup>RO</sup> | DRM key systems |


<a name="drms"></a>
## *drms*

Provides access to the supported DRM systems.

> This property is **read-only**.

### Events

No Events

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Supported DRM systems |
| (property)[#] | object |  |
| (property)[#].name | string | The name of the DRM system |
| (property)[#].keysystems | array |  |
| (property)[#].keysystems[#] | string | An identifier of a key system |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "OCDM.drms"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": [
        {
            "name": "PlayReady",
            "keysystems": [
                "com.microsoft.playready"
            ]
        }
    ]
}
```

<a name="keysystems"></a>
## *keysystems*

Provides access to the DRM key systems.

> This property is **read-only**.

### Events

No Events

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | DRM key systems |
| (property)[#] | string | An identifier of a key system |

> The *drm system* argument shall be passed as the index to the property, e.g. *OCDM.1.keysystems@PlayReady*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid DRM name |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "OCDM.keysystems@PlayReady"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": [
        "com.microsoft.playready"
    ]
}
```

