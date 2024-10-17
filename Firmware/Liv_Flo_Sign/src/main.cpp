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
#include "Discord.h"
#include "GithubOTA.h"
#include "console.h"
#include "displayMatrix.h"
#include "sensor.h"
#include "utils.h"

#define LED_MATRIX_PIN   7
#define LED_SIGNAL_PIN   8
#define BTN_PIN          9

#define LED_SIGN_COUNT   268
#define LED_MATRIX_H     7
#define LED_MATRIX_W     40
#define LED_MATRIX_COUNT (LED_MATRIX_H * LED_MATRIX_W)

Utils utils;
Sensor sensor;
Discord discord;
GithubOTA githubOTA;
DisplayMatrix disp(LED_MATRIX_PIN, LED_MATRIX_H, LED_MATRIX_W);


Adafruit_NeoPixel ledSign(LED_SIGN_COUNT, LED_SIGNAL_PIN, NEO_GRB + NEO_KHZ800);

static void updateTask(void* param);

void setup()
{
  pinMode(BTN_PIN, INPUT_PULLUP);
  console.begin();
  utils.begin();
  discord.begin();
  githubOTA.begin(REPO_URL);
  sensor.begin();
  disp.begin();

  ledSign.begin();
  ledSign.setBrightness(3);
  for(int i = 0; i < LED_SIGN_COUNT; i++)
  {
    ledSign.setPixelColor(i, 255, 0, 200);
  }
  // ledSign.show();

  xTaskCreate(updateTask, "main_task", 4096, NULL, 12, NULL);
}

void loop()
{
  vTaskDelay(100);
}


static void updateTask(void* param)
{
  while(true)
  {
    static bool btnOld = false, btnNew = false;
    btnOld = btnNew;
    btnNew = !digitalRead(BTN_PIN);


    if(utils.getConnectionState())
    {
      if(githubOTA.updateAvailable())
      {
        githubOTA.startUpdate();
      }
      if(githubOTA.updateInProgress())
      {
        disp.setState(DisplayMatrix::UPDATING);
        disp.setUpdatePercentage(githubOTA.getProgress());
      }
      else
      {
        disp.setState(DisplayMatrix::IDLE);
        disp.setMessage(discord.getLatestMessage());
      }
    }
    else
    {
      disp.setState(DisplayMatrix::DISCONNECTED);
    }

    if(sensor.getProxEvent())
    {
      console.log.println("[MAIN ]Proximity Event");
      discord.sendEvent("PROXIMITY");
    }

    uint8_t brightness = map(sensor.getAmbientBrightness(), 0, 255, 0, disp.MAX_BRIGHTNESS);
    brightness = brightness < 3? 0 : brightness;
    disp.setBrightness(brightness);   // Turn off display for very low brightness (colors get distorted)

    vTaskDelay(100);
    utils.resetWatchdog();
  }
}
