#include "utils.h"
#include <ArduinoJson.h>
#include <ESP32Ping.h>
#include <WiFi.h>
#include <time.h>
#include "HTTPClient.h"
#include "console.h"
#include "device.h"
#include "esp_wifi.h"

CustomWiFiManager Utils::wm(console.log);
Preferences Utils::preferences;
Utils::Country Utils::country = Utils::Unknown;
int32_t Utils::raw_offset = 0;
int32_t Utils::dst_offset = 0;
int Utils::buttonPin = -1;
hw_timer_t* Utils::Timer0_Cfg = NULL;
bool Utils::connectionState = false;
bool Utils::shortPressEvent = false;
bool Utils::longPressEvent = false;
bool Utils::timezoneValid = false;
bool Utils::tryReconnect = false;
String Utils::resetReason = "Not set";

bool Utils::pref_nightLight = false;
bool Utils::pref_motionActivated = false;

ParameterSwitch Utils::switch_nightLight("switch_nightLight", "Night Light");
ParameterSwitch Utils::switch_motionActivated("switch_motionActivated", "Motion Activated");


std::vector<const char*> Utils::menuItems = {"wifi", "param", "info", "sep", "update"};    // Don't display "Exit" in the menu

const char* Utils::resetReasons[] = {"Unknown",       "Power-on", "External",   "Software", "Panic", "Interrupt Watchdog",
                                     "Task Watchdog", "Watchdog", "Deep Sleep", "Brownout", "SDIO"};


CustomWiFiManagerParameter Utils::time_interval_slider(
  "time_interval",    // ID
  "Time Interval",    // Label
  "08:00-16:45",      // Default
  16,                 // Length
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
  "<input type='range' class='min' min='0' max='1425' value='480' step='15'>"
  "<input type='range' class='max' min='0' max='1425' value='1005' step='15'>"
  "</div>"
  "<div class='time-display'>"
  "<span id='startTime'>08:00</span><span id='endTime'>16:45</span>"
  "</div>"
  "<input type='hidden' id='time_interval' name='time_interval' value='08:00-16:45'>"
  "<script>"
  "const rangeFill = document.querySelector('.range-fill');"
  "const rangeInputs = document.querySelectorAll('.range-input input');"
  "const startTime = document.getElementById('startTime');"
  "const endTime = document.getElementById('endTime');"
  "const hiddenInput = document.getElementById('time_interval');"
  "// Set initial values from default"
  "function initializeValues() {"
  "  const defaultValues = hiddenInput.value.split('-');"
  "  const minValue = parseTimeToMinutes(defaultValues[0]);"
  "  const maxValue = parseTimeToMinutes(defaultValues[1]);"
  "  rangeInputs[0].value = minValue;"
  "  rangeInputs[1].value = maxValue;"
  "  updateSlider();"
  "}"
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
  "  hiddenInput.value = `${formatTime(minValue)}-${formatTime(maxValue)}`;"
  "}"
  "function formatTime(value) {"
  "  const hours = Math.floor(value / 60).toString().padStart(2, '0');"
  "  const minutes = (value % 60).toString().padStart(2, '0');"
  "  return `${hours}:${minutes}`;"
  "}"
  "function parseTimeToMinutes(time) {"
  "  const [hours, minutes] = time.split(':').map(Number);"
  "  return hours * 60 + minutes;"
  "}"
  "rangeInputs.forEach(input => input.addEventListener('input', updateSlider));"
  "initializeValues();"
  "</script>");


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
  resetReason = String(resetReasons[esp_reset_reason()]);
  console.log.printf("[UTILS] Reset reason: %s\n", resetReason);    // Panic, Watchdog is bad

  if(!preferences.begin("preferences", false))
  {
    console.error.println("[UTILS] Failed to open preferences");
    return false;
  }
  loadPreferences();

  connectionState = false;
  xTaskCreate(updateTask, "utils", 6000, NULL, 18, NULL);    // Stack Watermark: 4672

  Timer0_Cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &timerISR, true);
  timerAlarmWrite(Timer0_Cfg, 1000000 / BUTTON_TIMER_RATE, true);
  timerAlarmEnable(Timer0_Cfg);
  return true;
}

bool Utils::startWiFiManager()
{
  WiFi.begin();
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP

  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(0);
  wm.setCustomHeadElement(icon);
  wm.setMenu(menuItems);
  wm.setDarkMode(true);
  wm.setDebugOutput(true, WM_DEBUG_ERROR);    // For Debugging us: WM_DEBUG_VERBOSE
  wm.setConfigPortalSSID(Device::getDeviceName());
  wm.startWebPortal();

  // TODO: Label for Settings
  wm.addParameter(&switch_nightLight);
  wm.addParameter(&switch_motionActivated);

  // TODO: Slider Motion Activation Time

  // Color Hue for Idle Text
  // Color Hue for Night Light

  wm.setSaveParamsCallback(saveParamsCallback);
  reconnectWiFi(5, true);
  return true;
}

void Utils::saveParamsCallback()
{
  console.log.println("[UTILS] Saving parameters");

  pref_nightLight = switch_nightLight.getValue();
  if(pref_nightLight != preferences.getBool(SWITCH_NIGHT_LIGHT))
  {
    preferences.putBool(SWITCH_NIGHT_LIGHT, pref_nightLight);
    switch_nightLight.setValue(pref_nightLight);
    console.log.printf("  Night Light: %d\n", pref_nightLight);
  }

  pref_motionActivated = switch_motionActivated.getValue();
  if(pref_motionActivated != preferences.getBool(SWITCH_MOTION_ACTIVATED))
  {
    preferences.putBool(SWITCH_MOTION_ACTIVATED, pref_motionActivated);
    switch_motionActivated.setValue(pref_motionActivated);
    console.log.printf("  Motion Activated: %d\n", pref_motionActivated);
  }
}

void Utils::loadPreferences()
{
  pref_nightLight = preferences.getBool(SWITCH_NIGHT_LIGHT, false);
  switch_nightLight.setValue(pref_nightLight);

  pref_motionActivated = preferences.getBool(SWITCH_MOTION_ACTIVATED, false);
  switch_motionActivated.setValue(pref_motionActivated);
}


bool Utils::reconnectWiFi(int retries, bool verbose)
{
  if(WiFi.softAPgetStationNum() > 0)    // Abort if someone has already connected to the device (AP)
  {
    std::vector<IPAddress> clientIPs = getConnectedClientIPs(5);
    for(IPAddress ip : clientIPs)
    {
      const int retryCount = 10;
      for(int i = 0; i < retryCount; i++)
      {
        if(Ping.ping(ip))
        {
          console.warning.printf("[UTILS] Aborting reconnect: Client connected to portal: %s\n", ip.toString().c_str());
          return false;
        }
        console.warning.printf("[UTILS] Failed to ping client: %s, retrying...\n", ip.toString().c_str());
        delay(500);
      }
    }
  }
  bool res = false;
  wm.setConnectRetries(retries);
  wm.setWiFiAutoReconnect(false);    // We handle the reconnection ourselves
  if(wm.autoConnect(Device::getDeviceName().c_str()))
  {
    console.ok.printf("[UTILS] Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    if(!wm.getConfigPortalActive())
    {
      wm.startConfigPortal(Device::getDeviceName().c_str());
      console.log.println("[UTILS] Configportal started");
    }
    else if(verbose)
    {
      console.warning.println("[UTILS] Configportal already running");
    }
    res = true;
  }
  else if(verbose)
  {
    console.warning.println("[UTILS] Failed to connect to WiFi");
  }
  return res;
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
  if(!getConnectionState())    // Check if we are connected to the internet
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

  bool receivedTimeOffset = false;
  if(getOffsetFromWorldTimeAPI())
  {
    receivedTimeOffset = true;
  }
  else if(getOffsetFromIpapi())    // Try with alternative API (Ipapi), since WorldTimeAPI is currently not available
  {
    console.warning.println("[UTILS] Using Ipapi as fallback for time zone offset");
    receivedTimeOffset = true;
  }
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

std::vector<IPAddress> Utils::getConnectedClientIPs(int maxCount)
{
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
  std::vector<IPAddress> ipAddresses;
  esp_wifi_ap_get_sta_list(&wifi_sta_list);                         // Get the list of connected clients
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);    // Convert to adapter format for IP mapping
  int clientCount = adapter_sta_list.num;                           // Get the IP addresses of connected clients
  if(maxCount > 0 && clientCount > maxCount)
  {
    clientCount = maxCount;
  }
  for(int i = 0; i < clientCount; i++)
  {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
    ipAddresses.emplace_back(IPAddress(station.ip.addr));    // Add IPAddress to vector
  }
  return ipAddresses;
}

void Utils::updateTask(void* pvParameter)
{
  Utils* ref = (Utils*)pvParameter;
  static bool connectionStateOld = false;
  static uint32_t t = 0;

  ref->startWiFiManager();

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
        tryReconnect = false;
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
          else if(strncmp(myCountry.cc, "FR", 2) == 0)
          {
            country = Utils::France;
            console.log.println("[UTILS] Country: France");
          }
          else
          {
            country = Utils::Unknown;
            console.log.printf("[UTILS] Country: Unknown (%s)\n", myCountry.cc);
          }
        }
        getUnixTime();    // Get time offset from the internet
        static struct tm timeinfo;
        getCurrentTime(timeinfo);
      }
      else if(connectionStateOld && !connectionState)
      {
        tryReconnect = true;
        console.warning.println("[UTILS] Disconnected from WiFi");
      }

      if(millis() > 10000)    // Wait 10s after booting before checking for a disconnected IP
      {
        if(WiFi.localIP() == IPAddress(0, 0, 0, 0))    // Check if the IP was set to 0.0.0.0 (disconnected)
        {
          if((millis() - tConnected > 2000) && !forceWiFiOff)    // && tryReconnect)
          {
            console.warning.println("[UTILS] WiFi connected but IP is 0.0.0.0, disconnecting");
            connectionState = false;
            forceWiFiOff = true;
            tryReconnect = true;
            wm.disconnect();
            WiFi.disconnect();
            WiFi._setStatus(WL_DISCONNECTED);    // This is a bit hacky, but we need somehow to force WiFi API to show as disconnected
            reconnectWiFi(5);                    // Imeediate try to reconnect
          }
        }
        else
        {
          forceWiFiOff = false;
        }

        if(tryReconnect)    // Try to reconnect to WiFi if disconnected afer successful connection
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
