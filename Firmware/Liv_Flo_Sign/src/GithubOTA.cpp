/******************************************************************************
 * file    GithubOTA.cpp
 *******************************************************************************
 * brief   Firmware Update via Github Releases
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-04
 *******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022 Crelin - Florian Baumgartner
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

#include "GithubOTA.h"
#include "HTTPUpdate.h"
#include "console.h"
#include "utils.h"

bool GithubOTA::_serverAvailable = false;
bool GithubOTA::_updateAvailable = false;
bool GithubOTA::_startUpdate = false;
bool GithubOTA::_updateStarted = false;
bool GithubOTA::_updateAborted = false;
bool GithubOTA::_updateInProgress = false;
char GithubOTA::firmwareUrl[256];
uint16_t GithubOTA::_progress = 0;
HTTPClient GithubOTA::http;
WiFiClient GithubOTA::base_client;
ESP_SSLClient GithubOTA::client;
WiFiClientSecure GithubOTA::otaClient;

GithubOTA::GithubOTA() {}

void GithubOTA::begin(const char* currentFwVersion)
{
  _currentFwVersion = decodeFirmwareString(currentFwVersion);

  xTaskCreate(updateTask, "github", 8192, this, 5, NULL);
  console.log.println("[GITHUB_OTA] Started");
  console.log.printf("[GITHUB_OTA] Booting %s\n", _currentFwVersion.toString().c_str());
}

Firmware GithubOTA::decodeFirmwareString(const char* version)
{
  Firmware fw;
  sscanf(version, "%d.%d.%d", &fw.major, &fw.minor, &fw.patch);
  return fw;
}

int GithubOTA::compareFirmware(Firmware a, Firmware b)
{
  if(a.major > b.major)
    return 1;
  if(a.major < b.major)
    return -1;

  if(a.minor > b.minor)
    return 1;
  if(a.minor < b.minor)
    return -1;

  if(a.patch > b.patch)
    return 1;
  if(a.patch < b.patch)
    return -1;

  return 0;
}

bool GithubOTA::checkForUpdates()
{
  if(!Utils::getConnectionState())
  {
    _serverAvailable = false;
    _updateAvailable = false;
    _startUpdate = false;
    return false;
  }

  client.setTimeout(5000);
  client.setBufferSizes(1024 /* rx */, 1024 /* tx */);
  client.setDebugLevel(1);    // none = 0, error = 1, warn = 2, info = 3, dump = 4
  client.setClient(&base_client);
  client.setInsecure();    // Using insecure connection for testing

  snprintf(firmwareUrl, sizeof(firmwareUrl), "https://github.com/" REPO_URL "/releases/latest/download/firmware.bin?t=%lu", millis());
  if(!http.begin(client, firmwareUrl))
  {
    console.error.printf("[GITHUB_OTA] Server not available\n");
    _serverAvailable = false;
    _updateAvailable = false;
    _startUpdate = false;
    return false;    // Server not available
  }

  http.addHeader("Cache-Control", "no-cache");    // no cache
  http.addHeader("Connection", "keep-alive");     // Ensure persistent connection
  int httpCode = http.sendRequest("HEAD");
  if(httpCode < 200 || httpCode > 302)
  {
    console.warning.printf("[GITHUB_OTA] Error code: %d\n", httpCode);
    http.end();
    client.stop();
    _serverAvailable = false;
    _updateAvailable = false;
    _startUpdate = false;
    return false;
  }
  _serverAvailable = true;

  String location = http.getLocation();
  int start = location.indexOf("download/v") + 10;
  String onlineFirmware = location.substring(start, location.indexOf("/", start));
  _latestFwVersion = decodeFirmwareString(onlineFirmware.c_str());
  _updateAvailable = compareFirmware(_latestFwVersion, _currentFwVersion) > 0;    // Check if update is available
  // console.log.printf("[GITHUB_OTA] Online: %s, Current: %s, Update: %s\n", _latestFwVersion.toString().c_str(), _currentFwVersion.toString().c_str(), _updateAvailable ? "Yes" : "No");
  http.end();
  client.stop();

  if(_startUpdate && _updateAvailable)
  {
    _startUpdate = false;
    _updateAborted = false;
    _updateInProgress = true;
    _updateStarted = false;
    _progress = 0;

    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    console.log.printf("[GITHUB_OTA] Update started: %s -> %s\n", _currentFwVersion.toString().c_str(), _latestFwVersion.toString().c_str());

    httpUpdate.onStart([]() {
      _updateInProgress = true;
      console.log.printf("[GITHUB_OTA] Update Start\n");
    });
    httpUpdate.onEnd([]() { console.log.printf("[GITHUB_OTA] Update End\n"); });
    httpUpdate.onError([](int error) {
      console.error.printf("[GITHUB_OTA] Update Error: %d\n", error);
      _updateAborted = true;
      _updateInProgress = false;
    });
    httpUpdate.onProgress([](int current, int total) {
      _progress = (current * 100) / total;
      console.log.printf("[GITHUB_OTA] Update Progress: %d%%\n", (current * 100) / total);
    });

    otaClient.setInsecure();
    t_httpUpdate_return ret = httpUpdate.update(otaClient, firmwareUrl);
    switch(ret)
    {
      case HTTP_UPDATE_FAILED:
        console.error.printf("[GITHUB_OTA] HTTP Update Failed (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        http.end();
        client.stop();
        return false;

      case HTTP_UPDATE_NO_UPDATES:
        console.warning.printf("[GITHUB_OTA] No Update!\n");
        http.end();
        client.stop();
        return false;

      case HTTP_UPDATE_OK:
        console.ok.printf("[GITHUB_OTA] Update OK!\n");
        http.end();
        client.stop();
        return true;
    }
  }
  http.end();
  client.stop();
  return true;
}

void GithubOTA::updateTask(void* pvParameter)
{
  GithubOTA* ref = (GithubOTA*)pvParameter;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    if(!ref->_updateInProgress)
    {
      ref->checkForUpdates();    // Check is server is available and if an update is available
    }
    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 * FIRMWARE_UPDATE_INTERVAL);
  }
  vTaskDelete(NULL);
}
