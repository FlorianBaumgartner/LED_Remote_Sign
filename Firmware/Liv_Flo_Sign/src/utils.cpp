#include "utils.h"
#include <WiFi.h>
#include <time.h>
#include "HTTPClient.h"
#include "console.h"
#include "esp_wifi.h"
#include "device.h"

WiFiManagerCustom Utils::wm(console.log);
Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::raw_offset = 0;
int32_t Utils::dst_offset = 0;
int Utils::buttonPin = -1;
bool Utils::connectionState = false;
bool Utils::shortPressEvent = false;
bool Utils::longPressEvent = false;
bool Utils::timezoneValid = false;

const char* Utils::resetReasons[] = {"Unknown",       "Power-on", "External",   "Software", "Panic", "Interrupt Watchdog",
                                     "Task Watchdog", "Watchdog", "Deep Sleep", "Brownout", "SDIO"};

WiFiManagerParameter Utils::custom_mqtt_server("server", "mqtt server", "", 40);

const char* colorPickerHTML = 
  "<!DOCTYPE html><html><head>"
  "<meta charset='utf-8'>"
  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
  "<title>ESP Color Picker</title>"
  "<script src='https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js'></script>"
  "</head><body>"
  "<h1>RGB Color Picker</h1>"
  "<form action='/color' method='GET'>"
  "<input class='jscolor' name='rgb' value='FFFFFF'>"
  "<input type='submit' value='Set Color'>"
  "</form>"
  "</body></html>";


bool Utils::begin(void)
{
  esp_reset_reason_t resetReason = esp_reset_reason();
  console.log.printf("[UTILS] Reset reason: %s\n", resetReasons[resetReason]);    // Panic, Watchdog is bad

  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(180);
  wm.setConnectRetries(100);
  std::vector<const char*> menuItems = {"wifi", "param", "info", "sep", "update"};    // Don't display "Exit" in the menu
  wm.setMenu(menuItems);
  wm.setClass("invert");        // Dark theme
  wm.setConfigPortalSSID(Device::getDeviceName());
  wm.addParameter(&custom_mqtt_server);
  const char* menuhtml = "<form action='/param' method='get'><button>Custom</button></form><br/>\n";
  // wm.setCustomMenuHTML(colorPickerHTML);
  wm.setCustomMenuHTML(menuhtml);
  wm.setSaveParamsCallback(saveParamsCallback);

  if(wm.autoConnect(WIFI_STA_SSID))
  {
    console.ok.printf("[UTILS] Connected to %s\n", wm.getWiFiSSID().c_str());
    wm.startConfigPortal(Device::getDeviceName().c_str());
  }
  else
  {
    console.warning.println("[UTILS] Configportal running");
  }

  connectionState = false;
  xTaskCreate(updateTask, "utils", 4096, NULL, 18, NULL);
  return true;
}

void Utils::saveParamsCallback()
{
  console.log.println("Get Params:");
  console.log.print(custom_mqtt_server.getID());
  console.log.print(" : ");
  console.log.println(custom_mqtt_server.getValue());
}

uint32_t Utils::getUnixTime()
{
  struct tm timeinfo;
  static uint32_t lastCheck = 0;
  if(millis() - lastCheck > 10000)    // Check every 10 seconds what the current time offset is
  {
    lastCheck = millis();
    timezoneValid = updateTimeZoneOffset();
  }
  getCurrentTime(timeinfo);
  time_t now;
  time(&now);    // Get current time as Unix timestamp
  return now;
}

bool Utils::getCurrentTime(struct tm& timeinfo)
{
  // Check if we are connected to the internet
  if(WiFi.status() != WL_CONNECTED)
  {
    return false;
  }
  if(!getLocalTime(&timeinfo))
  {
    console.error.println("[UTILS] Failed to obtain time");
    return false;
  }
  return true;
}

bool Utils::updateTimeZoneOffset()
{
  const char* ntpServer = "pool.ntp.org";
  const char* timeApiUrl = "http://worldtimeapi.org/api/ip";

  HTTPClient http;
  http.begin(timeApiUrl);
  int httpCode = http.GET();
  if(httpCode != 200)
  {
    console.error.printf("[UTILS] Unable to fetch the time info\n");
    configTime(0, 0, ntpServer);
    return false;
  }
  String response = http.getString();
  raw_offset = response.substring(response.indexOf("\"raw_offset\":") + 13, response.indexOf(",\"week_number\"")).toInt();
  dst_offset = response.substring(response.indexOf("\"dst_offset\":") + 13, response.indexOf(",\"dst_from\"")).toInt();
  http.end();
  configTime(raw_offset + dst_offset, 0, ntpServer);
  return true;
}

void Utils::updateTask(void* pvParameter)
{
  Utils* ref = (Utils*)pvParameter;
  static bool connectionStateOld = false;
  static uint32_t t = 0;

  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    wm.process();    // Keep the WifiManager responsive

    static uint32_t buttonPressTime = 0;
    static bool buttonOld = false, buttonNew = false, longPressEarly = false;
    buttonOld = buttonNew;
    buttonNew = !digitalRead(buttonPin);

    if(!longPressEarly)
    {
      if(millis() - buttonPressTime > BUTTON_LONG_PRESS_TIME * 1000)
      {
        longPressEvent = true;
        longPressEarly = true;    // Prevent multiple long press events
      }
      if(buttonOld && !buttonNew)    // Button was released
      {
        shortPressEvent = true;
      }
    }
    if(!buttonNew)
    {
      buttonPressTime = millis();    // Keep the time of the last time the button was unpressed
      longPressEarly = false;
    }


    if(millis() - t >= 1000 / UTILS_UPDATE_RATE)
    {
      t = millis();

      connectionStateOld = connectionState;
      connectionState = WiFi.status() == WL_CONNECTED;
      if((!connectionStateOld && connectionState) ||  !timezoneValid)     // Retry getting time zone info
      {
        console.ok.println("[UTILS] Connected to WiFi");
        static bool firstRun = true;

        wifi_country_t myCountry;
        if(esp_wifi_get_country(&myCountry) == ESP_OK)
        {
          if(strncmp(myCountry.cc, "CH", 2) == 0)
          {
            country = Utils::Switzerland;
            console.log.println("[UTILS] Country: Switzerland");
          }
          else if(strncmp(myCountry.cc, "US", 2) == 0)
          {
            country = Utils::USA;
            console.log.println("[UTILS] Country: USA");
          }
          else
          {
            country = Utils::Unknown;
            console.log.println("[UTILS] Country: Unknown");
          }
        }
        timezoneValid = updateTimeZoneOffset();
        console.printf("[UTILS] Time Offset: %d h\n", (raw_offset + dst_offset) / 3600);
        struct tm timeinfo;
        getCurrentTime(timeinfo);
      }
      else if(connectionStateOld && !connectionState)
      {
        console.warning.println("[UTILS] Disconnected from WiFi");
      }
    }

    vTaskDelayUntil(&task_last_tick, pdMS_TO_TICKS(10));    // 10ms delay keep the WifiManager responsive
  }
}