/******************************************************************************
 * file    main.cpp
 *******************************************************************************
 * brief   Main Program
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2022-08-02
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

#include <Arduino.h>
#include "console.h"

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#include <WiFiManager.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define CHECK_FOR_UPDATES_INTERVAL 1
#ifndef VERSION
#define VERSION "0.0.12"
#endif

#ifndef REPO_URL
#define REPO_URL "Florianbaumgartner/led_remote_sign"
#endif

void firmwareUpdate();
void checkForUpdates(void* parameter);
int compareVersion(const char* online, const char* local);

TaskHandle_t checkForUpdatesTask = NULL;


#define LED            10
#define BLINK_INTERVAL 1000
#define LED_RGB_PIN    8

#define TOTAL_LEDS     480
#define LED_MATRIX_H   5
#define LED_MATRIX_W   TOTAL_LEDS / LED_MATRIX_H

WiFiManager wm;
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(LED_MATRIX_W, LED_MATRIX_H, LED_RGB_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  console.begin();
  console.log.println("OK, Let's go");

  //wm.resetSettings();     // reset settings - wipe credentials for testing

  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  if(wm.autoConnect("Liv Flo Sign"))
  {
    console.log.println("[MAIN] Connected...yeey :)");
  }
  else
  {
    console.log.println("[MAIN] Configportal running");
  }

  console.log.println("[MAIN] Booting: v" VERSION);
  console.log.print("[MAIN] IP address: ");
  console.log.println(WiFi.localIP());
  console.log.print("[MAIN] SSID: ");
  console.log.println(WiFi.SSID());

  matrix.begin();
  matrix.setRotation(2);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);
  matrix.setBrightness(3);
  matrix.setTextColor(matrix.Color(255, 255, 0));

  xTaskCreate(checkForUpdates,        // Function that should be called
              "Check For Updates",    // Name of the task (for debugging)
              6000,                   // Stack size (bytes)
              NULL,                   // Parameter to pass
              0,                      // Task priority
              &checkForUpdatesTask    // Task handle
  );
}

void loop()
{
  static uint32_t cycles = 0;

  static int t = 0;
  if(millis() - t >= 1000)
  {
    t = millis();
    // console.log.printf("FPS: %d\n", cycles);
    cycles = 0;
  }

  static int x = 5;
  char msg[50] = VERSION;
  const int len = strlen(msg) * 6;

  matrix.fillScreen(0);
  matrix.setCursor(x, -2);
  matrix.print(msg);
  static uint32_t tShift = 0;
  if(millis() - tShift >= 50)
  {
    tShift = millis();
    if(--x < -len)
    {
      x = 5;
    }
  }
  uint32_t tp = micros();
  matrix.show();
  // console.log.printf("Time: %d\n", micros() - tp);
  cycles++;

  wm.process();

  vTaskDelay(10);
}

void firmwareUpdate()
{
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  static const String firmwareUrl = String("https://github.com/") + REPO_URL + String("/releases/latest/download/firmware.bin");
  if(!http.begin(client, firmwareUrl))
    return;

  int httpCode = http.sendRequest("HEAD");
  if(httpCode < 300 || httpCode > 400)
  {
    console.warning.printf("[MAIN] Error code: %d\n", httpCode);
    http.end();
    return;
  }

  int start = http.getLocation().indexOf("download/v") + 10;
  String onlineFirmware = http.getLocation().substring(start, http.getLocation().indexOf("/", start));
  if(compareVersion(onlineFirmware.c_str(), VERSION) <= 0)
  {
    console.log.printf("[MAIN] No updates available (%s -> %s)\n", VERSION, onlineFirmware.c_str());
    http.end();
    return;
  }
  console.log.printf("[MAIN] Update available: %s -> %s\n", VERSION, onlineFirmware.c_str());
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  httpUpdate.onStart([]() { console.log.printf("[MAIN] Update Start\n"); });
  httpUpdate.onEnd([]() { console.log.printf("[MAIN] Update End\n"); });
  httpUpdate.onError([](int error) { console.error.printf("[MAIN] Update Error: %d\n", error); });
  httpUpdate.onProgress([](int current, int total) { console.log.printf("[MAIN] Update Progress: %d%%\n", (current * 100) / total); });

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch(ret)
  {
    case HTTP_UPDATE_FAILED:
      console.error.printf("[MAIN] HTTP Update Failed (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      console.warning.printf("[MAIN] No Update!\n");
      break;

    case HTTP_UPDATE_OK:
      console.ok.printf("[MAIN] Update OK!\n");
      break;
  }
}

void checkForUpdates(void* parameter)
{
  while(true)
  {
    firmwareUpdate();
    vTaskDelay((CHECK_FOR_UPDATES_INTERVAL * 1000) / portTICK_PERIOD_MS);
  }
}

int compareVersion(const char* online, const char* local)
{
  int onlineMajor, onlineMinor, onlinePatch;
  int localMajor, localMinor, localPatch;

  sscanf(online, "%d.%d.%d", &onlineMajor, &onlineMinor, &onlinePatch);
  sscanf(local, "%d.%d.%d", &localMajor, &localMinor, &localPatch);

  if(onlineMajor > localMajor)
    return 1;
  if(onlineMajor < localMajor)
    return -1;

  if(onlineMinor > localMinor)
    return 1;
  if(onlineMinor < localMinor)
    return -1;

  if(onlinePatch > localPatch)
    return 1;
  if(onlinePatch < localPatch)
    return -1;

  return 0;
}