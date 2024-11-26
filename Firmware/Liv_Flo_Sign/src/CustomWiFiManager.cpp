
/******************************************************************************
 * file    CustomWiFiManager.cpp
 *******************************************************************************
 * brief   Inherit from WiFiManager to add custom HTML elements
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-11-24
 *******************************************************************************
 * MIT License
 *
 * Copyright (c) 2024 Crelin - Florian Baumgartner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "CustomWiFiManager.h"
#include <console.h>


void WiFiManagerCustom::startWebPortal()    // Override works
{
  if(configPortalActive || webPortalActive)
    return;
  connect = abort = false;
  setupConfigPortal();
  webPortalActive = true;
}

boolean WiFiManagerCustom::startConfigPortal()
{
  String ssid = getDefaultAPName();
  return startConfigPortal(ssid.c_str(), NULL);
}

boolean WiFiManagerCustom::startConfigPortal(char const* apName, char const* apPassword)
{
  _begin();

  if(configPortalActive)
  {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("Starting Config Portal FAILED, is already running"));
#endif
    return false;
  }

  //setup AP
  _apName = apName;    // @todo check valid apname ?
  _apPassword = apPassword;

#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE, F("Starting Config Portal"));
#endif

  if(_apName == "")
    _apName = getDefaultAPName();

  if(!validApPassword())
    return false;

  // HANDLE issues with STA connections, shutdown sta if not connected, or else this will hang channel scanning and softap will not respond
  if(_disableSTA || (!WiFi.isConnected() && _disableSTAConn))
  {
// this fixes most ap problems, however, simply doing mode(WIFI_AP) does not work if sta connection is hanging, must `wifi_station_disconnect`
#ifdef WM_DISCONWORKAROUND
    WiFi.mode(WIFI_AP_STA);
#endif
    WiFi_Disconnect();
    WiFi_enableSTA(false);
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("Disabling STA"));
#endif
  }
  else
  {
    // WiFi_enableSTA(true);
  }

  // init configportal globals to known states
  configPortalActive = true;
  bool result = connect = abort = false;    // loop flags, connect true success, abort true break
  uint8_t state;

  _configPortalStart = millis();

// start access point
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE, F("Enabling AP"));
#endif
  startAP();
  WiFiSetCountry();

  // do AP callback if set
  if(_apcallback != NULL)
  {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("[CB] _apcallback calling"));
#endif
    _apcallback(this);
  }

// init configportal
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV, F("setupConfigPortal"));
#endif
  setupConfigPortal();

#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV, F("setupDNSD"));
#endif
  setupDNSD();


  if(!_configPortalIsBlocking)
  {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("Config Portal Running, non blocking (processing)"));
    if(_configPortalTimeout > 0)
      DEBUG_WM(WM_DEBUG_VERBOSE, F("Portal Timeout In"), (String)(_configPortalTimeout / 1000) + (String)F(" seconds"));
#endif
    return result;    // skip blocking loop
  }

  // enter blocking loop, waiting for config

#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE, F("Config Portal Running, blocking, waiting for clients..."));
  if(_configPortalTimeout > 0)
    DEBUG_WM(WM_DEBUG_VERBOSE, F("Portal Timeout In"), (String)(_configPortalTimeout / 1000) + (String)F(" seconds"));
#endif

  while(1)
  {

    // if timed out or abort, break
    if(configPortalHasTimeout() || abort)
    {
#ifdef WM_DEBUG_LEVEL
      DEBUG_WM(WM_DEBUG_DEV, F("configportal loop abort"));
#endif
      shutdownConfigPortal();
      result = abort ? portalAbortResult : portalTimeoutResult;    // false, false
      if(_configportaltimeoutcallback != NULL)
      {
#ifdef WM_DEBUG_LEVEL
        DEBUG_WM(WM_DEBUG_VERBOSE, F("[CB] config portal timeout callback"));
#endif
        _configportaltimeoutcallback();    // @CALLBACK
      }
      break;
    }

    state = processConfigPortal();

    // status change, break
    // @todo what is this for, should be moved inside the processor
    // I think.. this is to detect autoconnect by esp in background, there are also many open issues about autoreconnect not working
    if(state != WL_IDLE_STATUS)
    {
      result = (state == WL_CONNECTED);    // true if connected
      DEBUG_WM(WM_DEBUG_DEV, F("configportal loop break"));
      break;
    }

    if(!configPortalActive)
      break;

    yield();    // watchdog
  }

#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_NOTIFY, F("config portal exiting"));
#endif
  return result;
}

void WiFiManagerCustom::setupConfigPortal()    // Override works
{
  setupHTTPServer();
  _lastscan = 0;    // reset network scan cache
  if(_preloadwifiscan)
    WiFi_scanNetworks(true, true);    // preload wifiscan , async
}

void WiFiManagerCustom::setupHTTPServer()    // Override works
{
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(F("Starting Web Portal"));
#endif

  if(_httpPort != 80)
  {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("http server started with custom port: "), _httpPort);    // @todo not showing ip
#endif
  }

  server.reset(new WM_WebServer(_httpPort));
  // This is not the safest way to reset the webserver, it can cause crashes on callbacks initilized before this and since its a shared pointer...

  if(_webservercallback != NULL)
  {
#ifdef WM_DEBUG_LEVEL
    DEBUG_WM(WM_DEBUG_VERBOSE, F("[CB] _webservercallback calling"));
#endif
    _webservercallback();    // @CALLBACK
  }
  // @todo add a new callback maybe, after webserver started, callback cannot override handlers, but can grab them first

  /* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

  // G macro workaround for Uri() bug https://github.com/esp8266/Arduino/issues/7102
  server->on(WM_G(R_root), std::bind(&WiFiManager::handleRoot, this));
  server->on(WM_G(R_wifi), std::bind(&WiFiManager::handleWifi, this, true));
  server->on(WM_G(R_wifinoscan), std::bind(&WiFiManager::handleWifi, this, false));
  server->on(WM_G(R_wifisave), std::bind(&WiFiManager::handleWifiSave, this));
  server->on(WM_G(R_info), std::bind(&WiFiManagerCustom::handleInfo, this));    // Reroute to custom handler
  server->on(WM_G(R_param), std::bind(&WiFiManager::handleParam, this));
  server->on(WM_G(R_paramsave), std::bind(&WiFiManager::handleParamSave, this));
  server->on(WM_G(R_restart), std::bind(&WiFiManager::handleReset, this));
  server->on(WM_G(R_exit), std::bind(&WiFiManager::handleExit, this));
  server->on(WM_G(R_close), std::bind(&WiFiManager::handleClose, this));
  server->on(WM_G(R_erase), std::bind(&WiFiManager::handleErase, this, false));
  server->on(WM_G(R_status), std::bind(&WiFiManager::handleWiFiStatus, this));
  server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));

  server->on(WM_G(R_update), std::bind(&WiFiManager::handleUpdate, this));
  server->on(WM_G(R_updatedone), HTTP_POST, std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));

  server->begin();    // Web server start
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE, F("HTTP server started"));
#endif
}

void WiFiManagerCustom::handleInfo()
{
#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Info"));
#endif
  handleRequest();
  String page = getHTTPHead(FPSTR(S_titleinfo));    // @token titleinfo
  reportStatus(page);

  uint16_t infos = 0;

//@todo convert to enum or refactor to strings
//@todo wrap in build flag to remove all info code for memory saving
#ifdef ESP8266
  infos = 28;
  String infoids[] = {F("esphead"), F("uptime"),   F("chipid"),     F("fchipid"),   F("idesize"),   F("flashsize"), F("corever"),
                      F("bootver"), F("cpufreq"),  F("freeheap"),   F("memsketch"), F("memsmeter"), F("lastreset"), F("wifihead"),
                      F("conx"),    F("stassid"),  F("staip"),      F("stagw"),     F("stasub"),    F("dnss"),      F("host"),
                      F("stamac"),  F("autoconx"), F("wifiaphead"), F("apssid"),    F("apip"),      F("apbssid"),   F("apmac")};

#elif defined(ESP32)
  // add esp_chip_info ?
  infos = 27;
  String infoids[] = {F("esphead"), F("uptime"), F("chipid"), F("chiprev"), F("idesize"), F("flashsize"), F("cpufreq"), F("freeheap"), F("memsketch"),
                      F("memsmeter"), F("lastreset"), F("temp"),
                      // F("hall"),
                      F("wifihead"), F("conx"), F("stassid"), F("staip"), F("stagw"), F("stasub"), F("dnss"), F("host"), F("stamac"), F("apssid"),
                      F("wifiaphead"), F("apip"), F("apmac"), F("aphost"), F("apbssid")};
#endif

  for(size_t i = 0; i < infos; i++)
  {
    if(infoids[i] != NULL)
    {
      page += getInfoData(infoids[i]);
    }
  }
  page += F("</dl>");

  page += F("<h3>About</h3><hr><dl>");
  page += getInfoData("aboutver");
  page += getInfoData("aboutarduinover");
  page += getInfoData("aboutidfver");
  page += getInfoData("firmwareversion");    // Added firmware version
  page += getInfoData("aboutdate");
  page += F("</dl>");

  if(_showInfoUpdate)
  {
    page += HTTP_PORTAL_MENU[8];
    page += HTTP_PORTAL_MENU[9];
  }
  if(_showInfoErase)
    page += FPSTR(HTTP_ERASEBTN);
  if(_showBack)
    page += FPSTR(HTTP_BACKBTN);
  page += FPSTR(HTTP_HELP);
  page += FPSTR(HTTP_END);

  HTTPSend(page);

#ifdef WM_DEBUG_LEVEL
  DEBUG_WM(WM_DEBUG_DEV, F("Sent info page"));
#endif
}

String WiFiManagerCustom::getInfoData(String id)
{

  String p;
  if(id == F("esphead"))
  {
    p = FPSTR(HTTP_INFO_esphead);
#ifdef ESP32
    p.replace(FPSTR(T_1), (String)ESP.getChipModel());
#endif
  }
  else if(id == F("wifihead"))
  {
    p = FPSTR(HTTP_INFO_wifihead);
    p.replace(FPSTR(T_1), getModeString(WiFi.getMode()));
  }
  else if(id == F("uptime"))
  {
    // subject to rollover!
    p = FPSTR(HTTP_INFO_uptime);
    p.replace(FPSTR(T_1), (String)(millis() / 1000 / 60));
    p.replace(FPSTR(T_2), (String)((millis() / 1000) % 60));
  }
  else if(id == F("chipid"))
  {
    p = FPSTR(HTTP_INFO_chipid);
    p.replace(FPSTR(T_1), String(WIFI_getChipId(), HEX));
  }
#ifdef ESP32
  else if(id == F("chiprev"))
  {
    p = FPSTR(HTTP_INFO_chiprev);
    String rev = (String)ESP.getChipRevision();
#ifdef _SOC_EFUSE_REG_H_
    String revb = (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S) && EFUSE_RD_CHIP_VER_RESERVE_V);
    p.replace(FPSTR(T_1), rev + "<br/>" + revb);
#else
    p.replace(FPSTR(T_1), rev);
#endif
  }
#endif
#ifdef ESP8266
  else if(id == F("fchipid"))
  {
    p = FPSTR(HTTP_INFO_fchipid);
    p.replace(FPSTR(T_1), (String)ESP.getFlashChipId());
  }
#endif
  else if(id == F("idesize"))
  {
    p = FPSTR(HTTP_INFO_idesize);
    p.replace(FPSTR(T_1), (String)ESP.getFlashChipSize());
  }
  else if(id == F("flashsize"))
  {
#ifdef ESP8266
    p = FPSTR(HTTP_INFO_flashsize);
    p.replace(FPSTR(T_1), (String)ESP.getFlashChipRealSize());
#elif defined ESP32
    p = FPSTR(HTTP_INFO_psrsize);
    p.replace(FPSTR(T_1), (String)ESP.getPsramSize());
#endif
  }
  else if(id == F("corever"))
  {
#ifdef ESP8266
    p = FPSTR(HTTP_INFO_corever);
    p.replace(FPSTR(T_1), (String)ESP.getCoreVersion());
#endif
  }
#ifdef ESP8266
  else if(id == F("bootver"))
  {
    p = FPSTR(HTTP_INFO_bootver);
    p.replace(FPSTR(T_1), (String)system_get_boot_version());
  }
#endif
  else if(id == F("cpufreq"))
  {
    p = FPSTR(HTTP_INFO_cpufreq);
    p.replace(FPSTR(T_1), (String)ESP.getCpuFreqMHz());
  }
  else if(id == F("freeheap"))
  {
    p = FPSTR(HTTP_INFO_freeheap);
    p.replace(FPSTR(T_1), (String)ESP.getFreeHeap());
  }
  else if(id == F("memsketch"))
  {
    p = FPSTR(HTTP_INFO_memsketch);
    p.replace(FPSTR(T_1), (String)(ESP.getSketchSize()));
    p.replace(FPSTR(T_2), (String)(ESP.getSketchSize() + ESP.getFreeSketchSpace()));
  }
  else if(id == F("memsmeter"))
  {
    p = FPSTR(HTTP_INFO_memsmeter);
    p.replace(FPSTR(T_1), (String)(ESP.getSketchSize()));
    p.replace(FPSTR(T_2), (String)(ESP.getSketchSize() + ESP.getFreeSketchSpace()));
  }
  else if(id == F("lastreset"))
  {
#ifdef ESP8266
    p = FPSTR(HTTP_INFO_lastreset);
    p.replace(FPSTR(T_1), (String)ESP.getResetReason());
#elif defined(ESP32) && defined(_ROM_RTC_H_)
    // requires #include <rom/rtc.h>
    p = FPSTR(HTTP_INFO_lastreset);
    for(int i = 0; i < 2; i++)
    {
      int reason = rtc_get_reset_reason(i);
      String tok = (String)T_ss + (String)(i + 1) + (String)T_es;
      switch(reason)
      {
        //@todo move to array
        case 1:
          p.replace(tok, F("Vbat power on reset"));
          break;
        case 3:
          p.replace(tok, F("Software reset digital core"));
          break;
        case 4:
          p.replace(tok, F("Legacy watch dog reset digital core"));
          break;
        case 5:
          p.replace(tok, F("Deep Sleep reset digital core"));
          break;
        case 6:
          p.replace(tok, F("Reset by SLC module, reset digital core"));
          break;
        case 7:
          p.replace(tok, F("Timer Group0 Watch dog reset digital core"));
          break;
        case 8:
          p.replace(tok, F("Timer Group1 Watch dog reset digital core"));
          break;
        case 9:
          p.replace(tok, F("RTC Watch dog Reset digital core"));
          break;
        case 10:
          p.replace(tok, F("Instrusion tested to reset CPU"));
          break;
        case 11:
          p.replace(tok, F("Time Group reset CPU"));
          break;
        case 12:
          p.replace(tok, F("Software reset CPU"));
          break;
        case 13:
          p.replace(tok, F("RTC Watch dog Reset CPU"));
          break;
        case 14:
          p.replace(tok, F("for APP CPU, reseted by PRO CPU"));
          break;
        case 15:
          p.replace(tok, F("Reset when the vdd voltage is not stable"));
          break;
        case 16:
          p.replace(tok, F("RTC Watch dog reset digital core and rtc module"));
          break;
        default:
          p.replace(tok, F("NO_MEAN"));
      }
    }
#endif
  }
  else if(id == F("apip"))
  {
    p = FPSTR(HTTP_INFO_apip);
    p.replace(FPSTR(T_1), WiFi.softAPIP().toString());
  }
  else if(id == F("apmac"))
  {
    p = FPSTR(HTTP_INFO_apmac);
    p.replace(FPSTR(T_1), (String)WiFi.softAPmacAddress());
  }
#ifdef ESP32
  else if(id == F("aphost"))
  {
    p = FPSTR(HTTP_INFO_aphost);
    p.replace(FPSTR(T_1), WiFi.softAPgetHostname());
  }
#endif
#ifndef WM_NOSOFTAPSSID
#ifdef ESP8266
  else if(id == F("apssid"))
  {
    p = FPSTR(HTTP_INFO_apssid);
    p.replace(FPSTR(T_1), htmlEntities(WiFi.softAPSSID()));
  }
#endif
#endif
  else if(id == F("apbssid"))
  {
    p = FPSTR(HTTP_INFO_apbssid);
    p.replace(FPSTR(T_1), (String)WiFi.BSSIDstr());
  }
  // softAPgetHostname // esp32
  // softAPSubnetCIDR
  // softAPNetworkID
  // softAPBroadcastIP

  else if(id == F("stassid"))
  {
    p = FPSTR(HTTP_INFO_stassid);
    p.replace(FPSTR(T_1), htmlEntities((String)WiFi_SSID()));
  }
  else if(id == F("staip"))
  {
    p = FPSTR(HTTP_INFO_staip);
    p.replace(FPSTR(T_1), WiFi.localIP().toString());
  }
  else if(id == F("stagw"))
  {
    p = FPSTR(HTTP_INFO_stagw);
    p.replace(FPSTR(T_1), WiFi.gatewayIP().toString());
  }
  else if(id == F("stasub"))
  {
    p = FPSTR(HTTP_INFO_stasub);
    p.replace(FPSTR(T_1), WiFi.subnetMask().toString());
  }
  else if(id == F("dnss"))
  {
    p = FPSTR(HTTP_INFO_dnss);
    p.replace(FPSTR(T_1), WiFi.dnsIP().toString());
  }
  else if(id == F("host"))
  {
    p = FPSTR(HTTP_INFO_host);
#ifdef ESP32
    p.replace(FPSTR(T_1), WiFi.getHostname());
#else
    p.replace(FPSTR(T_1), WiFi.hostname());
#endif
  }
  else if(id == F("stamac"))
  {
    p = FPSTR(HTTP_INFO_stamac);
    p.replace(FPSTR(T_1), WiFi.macAddress());
  }
  else if(id == F("conx"))
  {
    p = FPSTR(HTTP_INFO_conx);
    p.replace(FPSTR(T_1), WiFi.isConnected() ? FPSTR(S_y) : FPSTR(S_n));
  }
#ifdef ESP8266
  else if(id == F("autoconx"))
  {
    p = FPSTR(HTTP_INFO_autoconx);
    p.replace(FPSTR(T_1), WiFi.getAutoConnect() ? FPSTR(S_enable) : FPSTR(S_disable));
  }
#endif
#if defined(ESP32) && !defined(WM_NOTEMP)
  else if(id == F("temp"))
  {
    // temperature is not calibrated, varying large offsets are present, use for relative temp changes only
    p = FPSTR(HTTP_INFO_temp);
    p.replace(FPSTR(T_1), (String)temperatureRead());
    p.replace(FPSTR(T_2), (String)((temperatureRead() + 32) * 1.8f));
  }
// else if(id==F("hall")){
//   p = FPSTR(HTTP_INFO_hall);
//   p.replace(FPSTR(T_1),(String)hallRead()); // hall sensor reads can cause issues with adcs
// }
#endif
  else if(id == F("aboutver"))
  {
    p = FPSTR(HTTP_INFO_aboutver);
    p.replace(FPSTR(T_1), FPSTR(WM_VERSION_STR));
  }
  else if(id == F("aboutarduinover"))
  {
#ifdef VER_ARDUINO_STR
    p = FPSTR(HTTP_INFO_aboutarduino);
    p.replace(FPSTR(T_1), String(VER_ARDUINO_STR));
#endif
  }
  // else if(id==F("aboutidfver")){
  //   #ifdef VER_IDF_STR
  //   p = FPSTR(HTTP_INFO_aboutidf);
  //   p.replace(FPSTR(T_1),String(VER_IDF_STR));
  //   #endif
  // }
  else if(id == F("aboutsdkver"))
  {
    p = FPSTR(HTTP_INFO_sdkver);
#ifdef ESP32
    p.replace(FPSTR(T_1), (String)esp_get_idf_version());
    // p.replace(FPSTR(T_1),(String)system_get_sdk_version()); // deprecated
#else
    p.replace(FPSTR(T_1), (String)system_get_sdk_version());
#endif
  }
  else if(id == F("aboutdate"))
  {
    p = FPSTR(HTTP_INFO_aboutdate);
    p.replace(FPSTR(T_1), String(__DATE__ " " __TIME__));
  }
  else if(id == F("firmwareversion"))
  {
    p = FPSTR(HTTP_INFO_firmwareversion);
    p.replace(FPSTR(T_1), String("v") + FIRMWARE_VERSION);
  }
  return p;
}