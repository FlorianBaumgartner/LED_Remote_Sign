#include "utils.h"
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
  wm.setMenu(menuItems);
  wm.setClass("invert");    // Dark theme
  wm.setConfigPortalSSID(Device::getDeviceName());
  wm.addParameter(&time_interval_slider);
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
      if((!connectionStateOld && connectionState) || !timezoneValid)    // Retry getting time zone info
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