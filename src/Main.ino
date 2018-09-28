/**
 * A simple Azure IoT Central example for sending telemetry from a DHT22 sensor using
 * an Esspressif ESP32 and the Arduino framework.
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include "Esp32MQTTClient.h"
#include <DHTesp.h>
#include <Ticker.h>

#define MAX_MESSAGE_LEN 1024
#define max(a, b) ((a) > (b) ? (a) : (b));

// Change these as required
#define DHTPIN 15
#define DHTTYPE DHT22

// WiFI SSID and password
const char *ssid = "<YOUR WIFI SSID>";
const char *password = "<YOUR WIFI PASSWORD>";

/**
 * String containing Hostname, Device Id & Device Key in the format:
 * "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"
 * "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"
 * 
 * For Azure IoT Central, you can generate the required connection string using the tool
 * available here:
 * 
 * https://github.com/Azure/dps-keygen
 */
static const char *connectionString = "<YOUR HUB CONNECTION STRING";

// Globals
static bool hasIoTHub = false;
static DHTesp dht;
static TempAndHumidity newValues;
static float heatIndex;
static float dewPoint;
static float cr;
static String comfortStatus;
static TaskHandle_t tempTaskHandle = NULL;
static Ticker tempTicker;
static ComfortState cf;
static bool tasksEnabled = false;

// Forward declarations
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();

/**
 * initDHT()
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initDHT()
{
  // Initialize temperature sensor
  dht.setup(DHTPIN, DHTesp::DHT22);
  Serial.println("DHT initiated");

  // Start task to get temperature
  xTaskCreatePinnedToCore(
      tempTask,        /* Function to implement the task */
      "tempTask ",     /* Name of the task */
      6000,            /* Stack size in words */
      NULL,            /* Task input parameter */
      5,               /* Priority of the task */
      &tempTaskHandle, /* Task handle. */
      1);              /* Core where the task should run */

  if (tempTaskHandle == NULL)
  {
    Serial.println("Failed to start task for temperature update");
    return false;
  }

  // Start update of environment data every 20 seconds
  tempTicker.attach(20, triggerGetTemp);
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp()
{
  if (tempTaskHandle != NULL)
  {
    xTaskResumeFromISR(tempTaskHandle);
  }
}

static String lastComfortStatus;
time_t lastSent = 0;
/**
 * sendTelemetry()
 * Assembles the JSON message and sends to IoT Hub / Central
 * @return void
 */
void sendTelemetry()
{
  // Don't send faster than once per minute
  time_t currentMillis = millis();
  if (hasIoTHub && (currentMillis > lastSent + 60 * 1000))
  {
    lastSent = currentMillis;

    StaticJsonBuffer<MAX_MESSAGE_LEN> json;
    JsonObject &root = json.createObject();

    root["temperature"] = newValues.temperature;
    root["humidity"] = newValues.humidity;
    root["heatIndex"] = heatIndex;
    root["dewPoint"] = dewPoint;

    if (comfortStatus != lastComfortStatus)
    {
      root["comfortStatus"] = comfortStatus;
      lastComfortStatus = comfortStatus;
      Serial.print("Comfort status changed: ");
      Serial.println(comfortStatus);
    }

    char buff[MAX_MESSAGE_LEN];
    root.printTo(buff);

    Serial.println(buff);

    if (Esp32MQTTClient_SendEvent(buff))
    {
      Serial.println("Sending data succeed");
    }
    else
    {
      Serial.println("Failure...");
    }
  }
}

/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters)
{
  Serial.println("tempTask loop started");
  while (1) // tempTask loop
  {
    if (tasksEnabled)
    {
      // Get temperature values
      if (getTemperature())
        sendTelemetry();
    }
    // Got sleep again
    vTaskSuspend(NULL);
  }
}

/**
 * getTemperature
 * Reads temperature from DHTxx sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature()
{
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0)
  {
    Serial.println("DHTxx error status: " + String(dht.getStatusString()));
    return false;
  }

  heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  switch (cf)
  {
  case Comfort_OK:
    comfortStatus = "Comfort_OK";
    break;
  case Comfort_TooHot:
    comfortStatus = "Comfort_TooHot";
    break;
  case Comfort_TooCold:
    comfortStatus = "Comfort_TooCold";
    break;
  case Comfort_TooDry:
    comfortStatus = "Comfort_TooDry";
    break;
  case Comfort_TooHumid:
    comfortStatus = "Comfort_TooHumid";
    break;
  case Comfort_HotAndHumid:
    comfortStatus = "Comfort_HotAndHumid";
    break;
  case Comfort_HotAndDry:
    comfortStatus = "Comfort_HotAndDry";
    break;
  case Comfort_ColdAndHumid:
    comfortStatus = "Comfort_ColdAndHumid";
    break;
  case Comfort_ColdAndDry:
    comfortStatus = "Comfort_ColdAndDry";
    break;
  default:
    comfortStatus = "Unknown:";
    break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  return true;
}

/**
 * setup()
 * Runs once after the device boots 
 * @return void
 */
void setup()
{
  Serial.begin(115200);

  Serial.println("Starting connecting WiFi.");
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!Esp32MQTTClient_Init((const uint8_t *)connectionString))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
  }

  // Signal iot hub connected
  hasIoTHub = true;
  // Signal end of setup() to tasks
  initDHT();
  tasksEnabled = true;
}

/**
 * loop()
 * Runs repeatedly as part of the ESP32 Arduino framework implementation
 * @return void
 */
void loop()
{
  if (!tasksEnabled)
  {
    // Wait 2 seconds to let system settle down
    delay(2000);
    // Enable task that will read values from the DHT sensor
    tasksEnabled = true;
    if (tempTaskHandle != NULL)
    {
      vTaskResume(tempTaskHandle);
    }
  }
  yield();
}
