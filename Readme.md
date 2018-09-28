# ESP32-IoTCentral-DHT22

A simple sample showing how to get data from a DHTxx sensor into Azure IoT Central.

Developed and tested with an ESP32-Pico-Kit, but it should work with any ESP32 based
device.

## Dependencies

Note: If you are using Platformio, the dependencies should be auto-resolved at build time.
Otherwise, you will need the following libraries:

* ArduinoJson - https://github.com/bblanchon/ArduinoJson
* ESP32 Azure IoT Arduino - https://github.com/VSChina/ESP32_AzureIoT_Arduino
* DHT sensor library for ESPx - https://github.com/beegee-tokyo/DHTesp

## Azure IoT Connection Strings with IoT Central

This sample DOES NOT include the code necessary to use the IoT Hub Device provisioning
service, which is used by Azure IoT Central. Therefore you must manually convert the values
from IoT Central into a Device Connection String using the tool available here:

https://github.com/Azure/dps-keygen


## Azure IoT Central Configuration

Measurements are sent at appx 1 message per minute to prevent message overage charges.

The following table shows how to set up the measurements in IoT Central:

| Display Name | Field Name  | Units | Min | Max | Decimals |
|--------------|-------------|:-----:|----:|----:|---------:|
| Temperature  | temperature |   C   |   0 | 100 |        2 |
| Humidity     | humidity    |   %   |   0 | 100 |        0 |
| Heat Index   | heatIndex   |   C   |   0 | 100 |        2 |
| Dew Point    | dewPoint    |   C   |   0 | 100 |        2 |

Additionally, a State enumeration that indicates the "Comfort Status" will be sent only when it changes. You can adjust how this is calculated by providing your own ComfortProfile to DHTesp. See DHTesp.h in the DHT sensor library for more information.

| Display Name   | Field Name    | Value name     | Value Display        |
|----------------|---------------|----------------|----------------------|
| Comfort Status | comfortStatus |                |                      |
|                |               | OK             | Comfort_OK           |
|                |               | Too Hot        | Comfort_TooHot       |
|                |               | Too Cold       | Comfort_TooCold      |
|                |               | Too Dry        | Comfort_TooDry       |
|                |               | Too Humid      | Comfort_TooHumid     |
|                |               | Hot and Humid  | Comfort_HotAndHumid  |
|                |               | Hot and Dry    | Comfort_HotAndDry    |
|                |               | Cold and Humid | Comfort_ColdAndHumid |
|                |               | Cold and Dry   | Comfort_ColdAndDry   |
|                |               | Unknown        | Unknown              |
