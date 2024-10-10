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
#include "GithubOTA.h"
#include "console.h"

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

#include "Discord.h"
#include "utils.h"


#define REPO_URL       "florianbaumgartner/led_remote_sign"


#define LED            10
#define BLINK_INTERVAL 1000
#define LED_RGB_PIN    8

#define TOTAL_LEDS     480
#define LED_MATRIX_H   5
#define LED_MATRIX_W   TOTAL_LEDS / LED_MATRIX_H


WiFiManager wm;
Discord discord;
GithubOTA githubOTA;
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(LED_MATRIX_W, LED_MATRIX_H, LED_RGB_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);


void scrollTextNonBlocking(const char* text, int speed);
static void updateTask(void* param);

void setup()
{
  console.begin();

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Get reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();
  const char* resetReasons[] = {"Unknown",       "Power-on", "External",   "Software", "Panic", "Interrupt Watchdog",
                                "Task Watchdog", "Watchdog", "Deep Sleep", "Brownout", "SDIO"};

  //wm.resetSettings();     // reset settings - wipe credentials for testing

  WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  if(wm.autoConnect("Liv Flo Sign"))
  {
    console.ok.println("[MAIN] Connected...yeey :)");
  }
  else
  {
    console.warning.println("[MAIN] Configportal running");
  }

  console.log.printf("[MAIN] Reset reason: %s\n", resetReasons[resetReason]);
  console.log.println("[MAIN] Booting: v" FIRMWARE_VERSION);

  Utils::begin();
  console.log.printf("[MAIN] Unix time: %d\n", Utils::getUnixTime());
  struct tm timeinfo;
  Utils::getCurrentTime(timeinfo);
  console.log.printf("[MAIN] Current time: %02d.%02d.%04d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);


  matrix.begin();
  matrix.setRotation(2);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);
  matrix.setBrightness(3);
  matrix.setTextColor(matrix.Color(255, 255, 0));

  discord.begin();

  githubOTA.begin(REPO_URL);
  xTaskCreate(updateTask, "main_task", 4096, NULL, 20, NULL);
}

void loop()
{
  wm.process();
  vTaskDelay(10);
}

void scrollTextNonBlocking(const char* text, int speed)
{
  static int x = 5;
  const int len = strlen(text) * 6;

  matrix.fillScreen(0);
  matrix.setCursor(x, -2);
  matrix.print(text);
  static uint32_t tShift = 0;
  if(millis() - tShift >= speed)
  {
    tShift = millis();
    if(--x < -len)
    {
      x = 5;
    }
  }
}

static void updateTask(void* param)
{
  while(true)
  {
    if(githubOTA.updateInProgress())
    {
      matrix.fillScreen(0);
      uint8_t progress = githubOTA.getProgress();
      matrix.fillRect(0, 0, (progress / 25) + 1, 5, matrix.Color(255, 0, 0));
    }
    else if(githubOTA.updateAvailable())
    {
      githubOTA.startUpdate();
    }
    else
    {
      char fwVersion[16];
      sprintf(fwVersion, "v%d.%d.%d", githubOTA.getCurrentFirmwareVersion().major, githubOTA.getCurrentFirmwareVersion().minor,
              githubOTA.getCurrentFirmwareVersion().patch);
      scrollTextNonBlocking(fwVersion, 50);
    }
    matrix.show();
    vTaskDelay(30);
  }
}