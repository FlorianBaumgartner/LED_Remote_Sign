#include "utils.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include "HTTPClient.h"
#include "console.h"
#include "device.h"
#include "esp_wifi.h"

WiFiManagerCustom Utils::wm(console.log);
Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::raw_offset = 0;
int32_t Utils::dst_offset = 0;
int Utils::buttonPin = -1;
hw_timer_t* Utils::Timer0_Cfg = NULL;
bool Utils::connectionState = false;
bool Utils::shortPressEvent = false;
bool Utils::longPressEvent = false;
bool Utils::timezoneValid = false;

const char* Utils::resetReasons[] = {"Unknown",       "Power-on", "External",   "Software", "Panic", "Interrupt Watchdog",
                                     "Task Watchdog", "Watchdog", "Deep Sleep", "Brownout", "SDIO"};

WiFiManagerParameter Utils::time_interval_slider(
  "time_interval",    // ID for the slider
  "<link href='https://cdnjs.cloudflare.com/ajax/libs/noUiSlider/15.7.0/nouislider.min.css' rel='stylesheet'>"
  "<style>"
  "#slider { margin-top: 10px; }"
  "#slider .noUi-handle { background-color: #0073e6; border-radius: 50%; height: 16px; width: 16px; top: -5px; }"
  "#slider .noUi-connect { background: #0073e6; }"
  ".noUi-target, .noUi-base, .noUi-connects { background: transparent; border: none; }"
  ".noUi-horizontal .noUi-handle:before, .noUi-horizontal .noUi-handle:after, .noUi-tooltip { display: none; }"
  "</style>"
  "<div style='color: #FFFFFF; font-family: Arial, sans-serif; font-size: 14px;'>Select Time Interval</div>"
  "<div id='slider'></div>"
  "<div style='display: flex; justify-content: space-between; color: #FFFFFF; font-family: Arial, sans-serif;'>"
  "<span id='startTime'>00:00</span><span id='endTime'>23:59</span></div>"
  "<script src='https://cdnjs.cloudflare.com/ajax/libs/noUiSlider/15.7.0/nouislider.min.js'></script>"
  "<script>"
  "const slider = document.getElementById('slider');"
  "noUiSlider.create(slider, {"
  "  start: [0, 95], connect: true, range: { 'min': 0, 'max': 95 }, step: 1,"
  "  format: {"
  "    to: v => {"
  "      const h = Math.floor(v / 4).toString().padStart(2, '0');"
  "      const m = ((v % 4) * 15).toString().padStart(2, '0');"
  "      return `${h}:${m}`;"
  "    },"
  "    from: Number }"
  "});"
  "const startTime = document.getElementById('startTime');"
  "const endTime = document.getElementById('endTime');"
  "slider.noUiSlider.on('update', v => { startTime.textContent = v[0]; endTime.textContent = v[1]; });"
  "slider.noUiSlider.on('change', v => { document.getElementById('time_interval').value = `${v[0]}-${v[1]}`; });"
  "</script>",
  "00:00-23:59",    // Default value
  16                // Length of value storage
);


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
  const char* headhtml =
    "<link rel='icon' type='image/png' "
    "href='data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/"
    "YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/"
    "x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//"
    "Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/"
    "8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/"
    "sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/"
    "1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++"
    "H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+"
    "jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/"
    "+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+"
    "HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mB"
    "x5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' />";
  wm.setCustomHeadElement(headhtml);
  wm.setMenu(menuItems);
  wm.setDarkMode(true);
  wm.setConfigPortalSSID(Device::getDeviceName());
  wm.startWebPortal();
  wm.addParameter(&time_interval_slider);
  wm.setSaveParamsCallback(saveParamsCallback);

  if(wm.autoConnect(Device::getDeviceName().c_str()))
  {
    console.ok.printf("[UTILS] Connected to %s\n", wm.getWiFiSSID().c_str());
    wm.startConfigPortal(Device::getDeviceName().c_str());
  }
  else
  {
    console.warning.println(String("[UTILS] Configportal running: ") + Device::getDeviceName());
  }

  connectionState = false;
  xTaskCreate(updateTask, "utils", 4096, NULL, 18, NULL);

  Timer0_Cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &timerISR, true);
  timerAlarmWrite(Timer0_Cfg, 1000000 / BUTTON_TIMER_RATE, true);
  timerAlarmEnable(Timer0_Cfg);
  return true;
}

void Utils::saveParamsCallback()
{
  console.log.printf("[UTILS] Saving parameters\n");

  // Access the list of parameters safely
  WiFiManagerParameter** params = wm.getParameters();
  int numParams = wm.getParametersCount();
  console.log.printf("[UTILS] Parameter count: %d\n", numParams);

  // Retrieve and print time interval parameter
  if(time_interval_slider.getValue() != nullptr)
  {
    console.log.printf("[UTILS] Time Interval: %s\n", time_interval_slider.getValue());
  }
  else
  {
    console.log.printf("[UTILS] Time Interval parameter is null or uninitialized\n");
  }
}


uint32_t Utils::getUnixTime()
{
  struct tm timeinfo;
  static int lastCheck = -TIMEZONE_UPDATE_INTERVAL * 1000;
  static bool timezoneValidOld = false;
  if(millis() - lastCheck > TIMEZONE_UPDATE_INTERVAL * 1000)    // Check every n seconds what the current time offset is
  {
    lastCheck = millis();
    timezoneValidOld = timezoneValid;
    timezoneValid = updateTimeZoneOffset();
    if(!timezoneValidOld && timezoneValid)
    {
      console.ok.printf("[UTILS] Time Offset: %d h\n", (raw_offset + dst_offset) / 3600);
    }
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

  bool receivedTimeOffset = (USE_IPAPI) ? getOffsetFromIpapi() : getOffsetFromWorldTimeAPI();
  if(receivedTimeOffset)
  {
    configTime(raw_offset + dst_offset, 0, ntpServer);
    return true;
  }
  console.error.println("[UTILS] Failed to obtain time zone offset");
  configTime(0, 0, ntpServer);
  return false;
}

bool Utils::getOffsetFromWorldTimeAPI()
{
  static const char* timeApiUrl = "http://worldtimeapi.org/api/ip";
  HTTPClient http;
  http.begin(timeApiUrl);
  int httpCode = http.GET();
  if(httpCode != 200)
  {
    console.error.printf("[UTILS] Unable to fetch the time info from WorldTimeAPI: %d\n", httpCode);
    http.end();
    return false;
  }
  String response = http.getString();
  raw_offset = response.substring(response.indexOf("\"raw_offset\":") + 13, response.indexOf(",\"week_number\"")).toInt();
  dst_offset = response.substring(response.indexOf("\"dst_offset\":") + 13, response.indexOf(",\"dst_from\"")).toInt();
  http.end();
  return true;
}

bool Utils::getOffsetFromIpapi()
{
  static const char* timeApiUrl = "https://api.ipapi.is/";
  HTTPClient http;
  http.begin(timeApiUrl);
  int httpCode = http.GET();
  if(httpCode != 200)
  {
    console.error.printf("[UTILS] Unable to fetch the time info from Ipapi: %d\n", httpCode);
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  static JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if(error)
  {
    console.error.printf("[UTILS] Failed to parse JSON: %s\n", error.c_str());
    return false;
  }
  String localTime = doc["location"]["local_time"].as<String>();    // e.g., "2024-11-14T20:42:49-06:00"
  bool is_dst = doc["location"]["is_dst"].as<bool>();

  localTime = localTime.substring(19);    // Remove the date part
  int offsetMinutes = localTime.substring(localTime.length() - 2).toInt();
  localTime = localTime.substring(0, localTime.length() - 3);
  int offsetHours = localTime.toInt();
  raw_offset = offsetHours * 3600 + offsetMinutes * 60;
  dst_offset = is_dst ? 3600 : 0;    // 1 hour if DST is active, else 0
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
      if((!connectionStateOld && connectionState) || (!timezoneValid && !wm.getConfigPortalActive()))    // Retry getting time zone info
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
        getUnixTime();    // Get time offset from the internet
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

void Utils::timerISR(void)
{
  static uint32_t buttonPressTime = millis();
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
}
