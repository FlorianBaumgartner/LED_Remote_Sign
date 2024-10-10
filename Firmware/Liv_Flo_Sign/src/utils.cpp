#include "utils.h"
#include <WiFi.h>
#include <time.h>
#include "console.h"
#include "esp_wifi.h"

#include "HTTPClient.h"

Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::time_offset = 0;

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
  if(!getLocalTime(&timeinfo))
  {
    console.error.println("[UTILS] Failed to obtain time");
    return false;
  }
  console.log.printf("[UTILS] Current time: %02d.%02d.%04d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

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
  if(!getLocalTime(&timeinfo))
  {
    console.error.println("[UTILS] Failed to obtain time");
    return 0;    // Return 0 in case of failure
  }
  time_t now;
  time(&now);    // Get current time as Unix timestamp
  return now;
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
  int32_t raw_offset = response.substring(response.indexOf("\"raw_offset\":") + 13, response.indexOf(",\"week_number\"")).toInt();
  int32_t dst_offset = response.substring(response.indexOf("\"dst_offset\":") + 13, response.indexOf(",\"dst_from\"")).toInt();
  http.end();
  time_offset = raw_offset + dst_offset;
  console.printf("[UTILS] Time Offset: %ld\n", time_offset);
  configTime(time_offset, 0, ntpServer);
  return true;
}
