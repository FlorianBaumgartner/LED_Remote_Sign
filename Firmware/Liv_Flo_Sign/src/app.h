/******************************************************************************
 * file    app.h
 *******************************************************************************
 * brief   Main Application
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-17
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

#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "discord.h"
#include "displayMatrix.h"
#include "displaySign.h"
#include "githubOTA.h"
#include "sensor.h"
#include "utils.h"

class App
{
 public:
  static constexpr const float APP_UPDATE_RATE = 10.0;        // [Hz]
  static constexpr const float LED_UPDATE_RATE = 30.0;        // [Hz]
  static constexpr const uint8_t NIGHT_LIGHT_MODE_MIN = 3;    // Below/Equal this value the night light is enabled

  static constexpr const float IP_ADDRESS_SHOW_TIME = 7.0;    // [s]

  App(Utils& utils, Sensor& sensor, Discord& discord, GithubOTA& githubOTA, DisplayMatrix& disp, DisplaySign& sign)
      : utils(utils), sensor(sensor), discord(discord), githubOTA(githubOTA), disp(disp), sign(sign)
  {}

  bool begin();

 private:
  Utils& utils;
  Sensor& sensor;
  Discord& discord;
  GithubOTA& githubOTA;
  DisplayMatrix& disp;
  DisplaySign& sign;

  Timer showIpAddressTimer;
  bool booting = true;

  static void appTask(void* pvParameter);
  static void ledTask(void* pvParameter);
};

#endif
