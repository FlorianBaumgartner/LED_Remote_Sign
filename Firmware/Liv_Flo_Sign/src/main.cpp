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
#include "app.h"
#include "console.h"
#include "displayMatrix.h"
#include "displaySign.h"
#include "sensor.h"
#include "utils.h"

#define LED_MATRIX_PIN 7
#define LED_SIGNAL_PIN 8
#define BTN_PIN        9

#define LED_SIGN_COUNT 268
#define LED_MATRIX_H   7
#define LED_MATRIX_W   40


static Utils utils(BTN_PIN);
static Sensor sensor;
static Discord discord;
static GithubOTA githubOTA;
static DisplayMatrix disp(LED_MATRIX_PIN, LED_MATRIX_H, LED_MATRIX_W);
static DisplaySign sign(LED_SIGNAL_PIN, LED_SIGN_COUNT);
static App app(utils, sensor, discord, githubOTA, disp, sign);


void setup()
{
  console.begin();
  utils.begin();
  app.begin();
}

void loop()
{
  vTaskDelay(100);
}
