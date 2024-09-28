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

#define CHECK_FOR_UPDATES_INTERVAL 5
#ifndef VERSION
#define VERSION "0.0.7"
#endif

#ifndef REPO_URL
#define REPO_URL "Florianbaumgartner/led_remote_sign"
#endif

unsigned long getUptimeSeconds();
void firmwareUpdate();
void checkForUpdates(void* parameter);

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

  matrix.begin();
  matrix.setRotation(2);
  matrix.setTextWrap(false);
  matrix.setBrightness(10);
  matrix.setTextColor(matrix.Color(255, 0, 255));

  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP


  //wm.resetSettings();     // reset settings - wipe credentials for testing

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  // automatically connect using saved credentials if they exist
  // If connection fails it starts an access point with the specified name
  if(wm.autoConnect("AutoConnectAP"))
  {
    console.log.println("connected...yeey :)");
  }
  else
  {
    console.log.println("Configportal running");
  }


  console.log.print("Booting: ");
#ifdef VERSION
  console.log.println(VERSION);
#endif

  console.log.println("Ready");
  console.log.print("IP address: ");
  console.log.println(WiFi.localIP());

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
  const char* msg = "This is Version " VERSION;
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
}


void checkForUpdates(void* parameter)
{
  for(;;)
  {
    firmwareUpdate();
    vTaskDelay((CHECK_FOR_UPDATES_INTERVAL * 1000) / portTICK_PERIOD_MS);
  }
}

void firmwareUpdate()
{
#ifdef VERSION
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  String firmwareUrl = String("https://github.com/") + REPO_URL + String("/releases/latest/download/firmware.bin");
  console.log.println(firmwareUrl);

  if(!http.begin(client, firmwareUrl))
    return;

  int httpCode = http.sendRequest("HEAD");
  if(httpCode < 300 || httpCode > 400 || http.getLocation().indexOf(String(VERSION)) > 0)
  {
    console.log.printf("Not updating from (sc=%d): %s\n", httpCode, http.getLocation().c_str());
    http.end();
    return;
  }
  else
  {
    console.log.printf("Updating from (sc=%d): %s\n", httpCode, http.getLocation().c_str());
  }

  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch(ret)
  {
    case HTTP_UPDATE_FAILED:
      console.log.printf("Http Update Failed (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      console.log.printf("No Update!\n");
      break;

    case HTTP_UPDATE_OK:
      console.log.printf("Update OK!\n");
      break;
  }
#endif
}

unsigned long getUptimeSeconds()
{
  return (unsigned long)(esp_timer_get_time() / 1000000ULL);
}
