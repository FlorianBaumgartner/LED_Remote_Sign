#include "utils.h"
#include <WiFi.h>
#include <time.h>
#include "console.h"
#include "esp_wifi.h"

#include "HTTPClient.h"

Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::raw_offset = 0;
int32_t Utils::dst_offset = 0;

bool Utils::begin(void)
{
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
      console.log.println("[UTILS] Country: Unknown or other");
    }
  }

  updateTimeZoneOffset();
  struct tm timeinfo;
  return getCurrentTime(timeinfo);
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
  console.printf("[UTILS] Time Offset: %ld\n", raw_offset + dst_offset);
  configTime(raw_offset + dst_offset, 0, ntpServer);
  return true;
}
