#include "utils.h"
#include <WiFi.h>
#include <time.h>
#include "HTTPClient.h"
#include "console.h"
#include "esp_wifi.h"

WiFiManager Utils::wm(console.log);
Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::raw_offset = 0;
int32_t Utils::dst_offset = 0;
bool Utils::connectionState = false;
const char* Utils::resetReasons[] = {"Unknown",       "Power-on", "External",   "Software", "Panic", "Interrupt Watchdog",
                                     "Task Watchdog", "Watchdog", "Deep Sleep", "Brownout", "SDIO"};


bool Utils::begin(void)
{
  esp_reset_reason_t resetReason = esp_reset_reason();
  console.log.printf("[UTILS] Reset reason: %s\n", resetReasons[resetReason]);

  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalBlocking(false);
  // wm.setConfigPortalTimeout(60);
  if(wm.autoConnect(WIFI_STA_SSID))
  {
    console.ok.printf("[UTILS] Connected to %s\n", wm.getWiFiSSID().c_str());
  }
  else
  {
    console.warning.println("[UTILS] Configportal running");
  }

  connectionState = false;
  xTaskCreate(updateTask, "utils", 2048, NULL, 10, NULL);
  return true;
}

uint32_t Utils::getUnixTime()
{
  struct tm timeinfo;
  static uint32_t lastCheck = 0;
  if(millis() - lastCheck > 10000)    // Check every 10 seconds what the current time offset is
  {
    lastCheck = millis();
    updateTimeZoneOffset();
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

    if(millis() - t >= 1000 / UTILS_UPDATE_RATE)
    {
      t = millis();

      connectionStateOld = connectionState;
      connectionState = WiFi.status() == WL_CONNECTED;
      if(!connectionStateOld && connectionState)
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
        updateTimeZoneOffset();
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