/******************************************************************************
 * file    GithubOTA.h
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

#ifndef GITHUBOTA_H
#define GITHUBOTA_H

#include <Arduino.h>

#define FIRMWARE_UPDATE_INTERVAL 10    // [s]  Interval to check for updates

class Firmware
{
 public:
  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;
  String toString() { return "v" + String(major) + "." + String(minor) + "." + String(patch); }
};

class GithubOTA
{
 public:
  GithubOTA();
  void begin(const char* repo, const char* currentFwVersion = FIRMWARE_VERSION);
  bool isServerAvailable() { return _serverAvailable; }
  bool updateAvailable() { return _updateAvailable && !_updateInProgress; }
  void startUpdate() { _startUpdate = _updateStarted = true; }
  uint16_t getProgress() { return _progress; }
  bool updateStarted() { return _updateStarted; }
  bool updateInProgress() { return _updateInProgress || _updateStarted; }
  bool updateAborted() { return _updateAborted; }
  Firmware getCurrentFirmwareVersion() { return _currentFwVersion; }
  Firmware getLatestFirmwareVersion() { return _latestFwVersion; }

 private:
  const char* _repo;
  String firmwareUrl;
  Firmware _latestFwVersion;
  Firmware _currentFwVersion;
  static bool _serverAvailable;
  static bool _updateAvailable;
  static bool _startUpdate;
  static bool _updateStarted;
  static bool _updateAborted;
  static bool _updateInProgress;
  static uint16_t _progress;

  Firmware decodeFirmwareString(const char* version);
  int compareFirmware(Firmware a, Firmware b);    // Returns 1 if a > b, -1 if a < b, 0 if a == b
  bool checkForUpdates();

  static void updateTask(void* pvParameter);
};


#endif
