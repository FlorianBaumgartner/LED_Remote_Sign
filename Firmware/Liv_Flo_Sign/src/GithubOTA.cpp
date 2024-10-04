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
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <console.h>

bool GithubOTA::_serverAvailable = false;
bool GithubOTA::_updateAvailable = false;
bool GithubOTA::_startUpdate = false;
bool GithubOTA::_updateAborted = false;
bool GithubOTA::_updateInProgress = false;
uint16_t GithubOTA::_progress = 0;

GithubOTA::GithubOTA() {}

void GithubOTA::begin(const char* repo, const char* currentFwVersion)
{
  _repo = repo;
  _currentFwVersion = decodeFirmwareString(currentFwVersion);

  xTaskCreate(updateTask, "github", 8096, this, 0, NULL);
  console.log.printf("[GITHUB_OTA] Begin\n");
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
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  firmwareUrl = String("https://github.com/") + _repo + String("/releases/latest/download/firmware.bin") + "?t=" + String(millis());
  if(!http.begin(client, firmwareUrl))
  {
    console.error.printf("[GITHUB_OTA] Server not available\n");
    _serverAvailable = false;
    _updateAvailable = false;
    _startUpdate = false;
    return false;    // Server not available
  }

  http.addHeader("Cache-Control", "no-cache");    // no cache
  int httpCode = http.sendRequest("HEAD");
  if(httpCode < 300 || httpCode > 400)
  {
    console.warning.printf("[GITHUB_OTA] Error code: %d\n", httpCode);
    http.end();
    _serverAvailable = false;
    _updateAvailable = false;
    _startUpdate = false;
    return false;
  }

  int start = http.getLocation().indexOf("download/v") + 10;
  String onlineFirmware = http.getLocation().substring(start, http.getLocation().indexOf("/", start));
  _latestFwVersion = decodeFirmwareString(onlineFirmware.c_str());
  _updateAvailable = compareFirmware(_latestFwVersion, _currentFwVersion) > 0;    // Check if update is available

  if(_startUpdate && _updateAvailable)
  {
    _startUpdate = false;
    _updateAborted = false;
    _updateInProgress = false;
    _progress = 0;

    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    console.log.printf("[GITHUB_OTA] Update started: %s -> %s\n", FimrwareVersionToString(_currentFwVersion),
                       FimrwareVersionToString(_latestFwVersion));

    httpUpdate.onStart([]() {
      _updateInProgress = true;
      console.log.printf("[GITHUB_OTA] Update Start\n");
    });
    httpUpdate.onEnd([]() {
      _updateInProgress = false;
      _updateAvailable = false;
      console.log.printf("[GITHUB_OTA] Update End\n");
    });
    httpUpdate.onError([](int error) {
      console.error.printf("[GITHUB_OTA] Update Error: %d\n", error);
      _updateAborted = true;
    });
    httpUpdate.onProgress([](int current, int total) {
      _progress = (current * 100) / total;
      console.log.printf("[GITHUB_OTA] Update Progress: %d%%\n", (current * 100) / total);
    });

    t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);
    switch(ret)
    {
      case HTTP_UPDATE_FAILED:
        console.error.printf("[GITHUB_OTA] HTTP Update Failed (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        return false;

      case HTTP_UPDATE_NO_UPDATES:
        console.warning.printf("[GITHUB_OTA] No Update!\n");
        return false;

      case HTTP_UPDATE_OK:
        console.ok.printf("[GITHUB_OTA] Update OK!\n");
        return true;
    }
  }
  return true;
}

const char* GithubOTA::FimrwareVersionToString(Firmware& fw)
{
  static char version[16];
  sprintf(version, "%d.%d.%d", fw.major, fw.minor, fw.patch);
  return version;
}

void GithubOTA::updateTask(void* pvParameter)
{
  GithubOTA* ref = (GithubOTA*)pvParameter;

  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();

    ref->checkForUpdates();    // Check is server is available and if an update is available

    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 / FIRMWARE_UPDATE_INTERVAL);
  }
  vTaskDelete(NULL);
}
