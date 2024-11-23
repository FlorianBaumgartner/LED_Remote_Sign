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
  "<style>"
  ".double-range { width: 100%; max-width: 500px; margin: 10px auto; font-family: Arial, sans-serif; color: #FFFFFF; }"
  ".range-slider { position: relative; width: calc(100% - 20px); margin: 0 auto; height: 10px; background-color: #e1e9f6; border-radius: 5px; "
  "overflow: hidden; box-sizing: border-box; padding: 0 10px; }"
  ".range-fill { position: absolute; height: 100%; background-color: #007bff; border-radius: 5px; }"
  ".range-input { position: relative; width: 100%; height: 10px; margin-top: -10px; }"
  ".range-input input { position: absolute; width: 100%; height: 10px; margin: 0; top: 0; pointer-events: none; -webkit-appearance: none; "
  "background: none; }"
  ".range-input input::-webkit-slider-thumb { height: 20px; width: 20px; border-radius: 50%; background-color: #007bff; border: 4px solid #80bfff; "
  "pointer-events: auto; -webkit-appearance: none; cursor: pointer; }"
  ".range-input input::-moz-range-thumb { height: 20px; width: 20px; border-radius: 50%; background-color: #007bff; border: 4px solid #80bfff; "
  "pointer-events: auto; cursor: pointer; }"
  ".time-display { display: flex; justify-content: space-between; margin-top: 5px; font-size: 14px; }"
  "</style>"
  "<div class='double-range'>"
  "<h2 style='color: #FFFFFF; margin-bottom: 5px;'>Select Time Interval</h2>"
  "<div class='range-slider'>"
  "<span class='range-fill'></span>"
  "</div>"
  "<div class='range-input'>"
  "<input type='range' class='min' min='0' max='1425' value='0' step='15'>"
  "<input type='range' class='max' min='0' max='1425' value='120' step='15'>"
  "</div>"
  "<div class='time-display'>"
  "<span id='startTime'>00:00</span><span id='endTime'>02:00</span>"
  "</div>"
  "<script>"
  "const rangeFill = document.querySelector('.range-fill');"
  "const rangeInputs = document.querySelectorAll('.range-input input');"
  "const startTime = document.getElementById('startTime');"
  "const endTime = document.getElementById('endTime');"
  "function updateSlider() {"
  "  const min = parseInt(rangeInputs[0].value);"
  "  const max = parseInt(rangeInputs[1].value);"
  "  if (min >= max) rangeInputs[0].value = max - 15;"
  "  if (max <= min) rangeInputs[1].value = min + 15;"
  "  const minValue = parseInt(rangeInputs[0].value);"
  "  const maxValue = parseInt(rangeInputs[1].value);"
  "  const minPercent = (minValue / 1425) * 100;"
  "  const maxPercent = (maxValue / 1425) * 100;"
  "  rangeFill.style.left = `${minPercent}%`;"
  "  rangeFill.style.right = `${100 - maxPercent}%`;"
  "  startTime.textContent = formatTime(minValue);"
  "  endTime.textContent = formatTime(maxValue);"
  "  document.getElementById('time_interval').value = `${formatTime(minValue)}-${formatTime(maxValue)}`;"
  "}"
  "function formatTime(value) {"
  "  const hours = Math.floor(value / 60).toString().padStart(2, '0');"
  "  const minutes = (value % 60).toString().padStart(2, '0');"
  "  return `${hours}:${minutes}`;"
  "}"
  "rangeInputs.forEach(input => input.addEventListener('input', updateSlider));"
  "updateSlider();"
  "</script>",
  "00:00-02:00",    // Default value
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
  pinMode(buttonPin, INPUT_PULLUP);

  esp_reset_reason_t resetReason = esp_reset_reason();
  console.log.printf("[UTILS] Reset reason: %s\n", resetReasons[resetReason]);    // Panic, Watchdog is bad

  WiFi.begin();
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(0);
  static std::vector<const char*> menuItems = {"wifi", "param", "info", "sep", "update"};    // Don't display "Exit" in the menu
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
  reconnectWiFi(5, true);
  connectionState = false;
  xTaskCreate(updateTask, "utils", 6000, NULL, 18, NULL);    // Stack Watermark: 4672

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

bool Utils::reconnectWiFi(int retries, bool verbose)
{
  wm.setConnectRetries(retries);
  if(WiFi.softAPgetStationNum() > 0)    // Abort if someone has already connected to the device (AP)
  {
    console.warning.println("[UTILS] Aborting reconnect: Client connected to config portal.");
    return false;
  }
  if(wm.autoConnect(Device::getDeviceName().c_str()))
  {
    console.ok.printf("[UTILS] Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    wm.startConfigPortal(Device::getDeviceName().c_str());
    return true;
  }
  if(verbose)
  {
    console.warning.println(String("[UTILS] Configportal running: ") + Device::getDeviceName());
  }
  return false;
}


uint32_t Utils::getUnixTime()
{
  struct tm timeinfo;
  static bool timezoneValidOld = false;
  static int lastCheck = -TIMEZONE_UPDATE_INTERVAL * 1000;
  if(millis() - lastCheck > TIMEZONE_UPDATE_INTERVAL * 1000)    // Check every n seconds what the current time offset is
  {
    lastCheck = millis();
    timezoneValidOld = timezoneValid;
    timezoneValid = updateTimeZoneOffset();
    if(timezoneValid && !timezoneValidOld)
    {
      console.log.printf("[UTILS] Updated time zone to: %d h\n", (raw_offset + dst_offset) / 3600);
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
  if(!getConnectionState())
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
  static const char* ntpServer = "pool.ntp.org";

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
  static HTTPClient http;
  static const int retryCount = 3;
  for(int i = 0; i < retryCount; i++)
  {
    http.begin(timeApiUrl);
    int httpCode = http.GET();
    if(httpCode != 200)
    {
      console.error.printf("[UTILS] Unable to fetch the time info from WorldTimeAPI: %d\n", httpCode);
      http.end();
      delay(250);
      continue;
    }
    String response = http.getString();
    raw_offset = response.substring(response.indexOf("\"raw_offset\":") + 13, response.indexOf(",\"week_number\"")).toInt();
    dst_offset = response.substring(response.indexOf("\"dst_offset\":") + 13, response.indexOf(",\"dst_from\"")).toInt();
    http.end();
    return true;
  }
  return false;
}

bool Utils::getOffsetFromIpapi()
{
  static const char* timeApiUrl = "https://api.ipapi.is/";
  static HTTPClient http;
  static const int retryCount = 3;
  for(int i = 0; i < retryCount; i++)
  {
    http.begin(timeApiUrl);
    http.setUserAgent("Mozilla/5.0 (compatible; LivFloSign/1.0)");
    int httpCode = http.GET();
    if(httpCode != 200)
    {
      if(httpCode == 403)
      {
        console.error.println("[UTILS] To many requests to Ipapi, try again later.");
        http.end();
        return false;
      }
      console.error.printf("[UTILS] Unable to fetch the time info from Ipapi: %d\n", httpCode);
      http.end();
      delay(250);
      continue;
    }
    String payload = http.getString();
    http.end();
    static StaticJsonDocument<2048> doc;
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
  return false;
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

      static bool forceWiFiOff = false;
      static int tConnected = -1;
      connectionStateOld = connectionState;
      connectionState = WiFi.isConnected() && !forceWiFiOff;
      if((!connectionStateOld && connectionState) || (!timezoneValid && !wm.getConfigPortalActive()))    // Retry getting time zone info
      {
        console.ok.println("[UTILS] Connected to WiFi");
        static bool firstRun = true;
        tConnected = millis();

        static wifi_country_t myCountry;
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
        static struct tm timeinfo;
        getCurrentTime(timeinfo);
      }
      else if(connectionStateOld && !connectionState)
      {
        console.warning.println("[UTILS] Disconnected from WiFi");
      }

      if(WiFi.localIP() == IPAddress(0, 0, 0, 0))    // Check if the IP was set to 0.0.0.0 (disconnected)
      {
        if((millis() - tConnected > 2000) && !forceWiFiOff)
        {
          console.warning.println("[UTILS] WiFi connected but IP is 0.0.0.0, disconnecting");
          connectionState = false;
          forceWiFiOff = true;
          wm.disconnect();
          WiFi.disconnect();
          WiFi._setStatus(WL_DISCONNECTED);    // Force the status to disconnected
        }
      }
      else
      {
        forceWiFiOff = false;
      }

      if(!connectionState)    // WHile connected, check if the IP was set to 0.0.0.0 (disconnected)
      {
        static int t = 0;
        if(millis() - t > WIFI_RECONNECT_INTERVAL * 1000)
        {
          t = millis();
          tConnected = t;    // Reset the timer, to enable a new connection
          reconnectWiFi();
        }
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
