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

#define LED            10
#define BLINK_INTERVAL 1000
#define LED_RGB_PIN    8

#define TOTAL_LEDS     480
#define LED_MATRIX_H   5
#define LED_MATRIX_W   TOTAL_LEDS / LED_MATRIX_H

Adafruit_NeoMatrix matrix =
  Adafruit_NeoMatrix(LED_MATRIX_W, LED_MATRIX_H, LED_RGB_PIN, NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

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

  // iluminate one pixel and iterate over all columns and rows
  // for (int i = 0; i < matrix.width(); i++)
  // {
  //   for (int j = 0; j < matrix.height(); j++)
  //   {
  //     matrix.fillScreen(0);
  //     matrix.drawPixel(i, j, matrix.Color(255, 0, 255));
  //     matrix.show();
  //     delay(100);
  //   }
  // }
}

void loop()
{
  static uint32_t cycles = 0;

  static int t = 0;
  if(millis() - t >= 1000)
  {
    t = millis();
    console.log.printf("FPS: %d\n", cycles);
    cycles = 0;
  }

  digitalWrite(LED, Serial);

  static int x = 5;
  const char* msg = "Love you!";
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
}
