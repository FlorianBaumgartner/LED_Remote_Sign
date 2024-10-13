/******************************************************************************
 * file    displayMatrix.cpp
 *******************************************************************************
 * brief   Handles the LED Matrix Display
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-12
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

#include "displayMatrix.h"
#include "../fonts/Grand9K_Pixel8_Modified.h"

void DisplayMatrix::begin(void)
{
  matrix.begin();
  matrix.setRotation(2);
  matrix.setTextSize(1);
  matrix.setFont(&Grand9K_Pixel8pt7bModified);
  matrix.setTextWrap(false);
  matrix.setBrightness(DEFAULT_BRIGHNESS);
  matrix.setTextColor(matrix.Color(0, 0, 255));
  matrix.fillScreen(0);
  matrix.show();

  xTaskCreate(updateTask, "matrix", 4096, this, 15, NULL);
}


void DisplayMatrix::updateTask(void* pvParameter)
{
  DisplayMatrix* display = (DisplayMatrix*)pvParameter;
  static State lastState = (State)-1;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();

    // Handle Events
    if(display->state != lastState)
    {
      lastState = display->state;
      switch(display->state)
      {
        case DisplayMatrix::BOOTING:
          display->matrix.fillScreen(0);
          display->matrix.setCursor(0, 6);
          display->matrix.setTextColor(display->matrix.Color(255, 255, 255));
          display->matrix.print("Booting...");
          display->matrix.show();
          break;
        case DisplayMatrix::IDLE:
          //   display->matrix.fillScreen(0);
          //   display->matrix.setCursor(0, 6);
          //   display->matrix.print("IDLE");
          //   display->matrix.show();
          break;
        case DisplayMatrix::DISCONNECTED:
          display->matrix.fillScreen(0);
          display->matrix.setCursor(0, 6);
          display->matrix.print("DISCONNECTED");
          display->matrix.show();
          break;
        case DisplayMatrix::UPDATING:
          display->matrix.fillScreen(0);
          display->matrix.setCursor(0, 6);
          display->matrix.print("UPDATING");
          display->matrix.show();
          break;
      }
    }

    // Handle Live Updates
    static uint8_t oldPercentage = 0;
    switch(display->state)
    {
      case DisplayMatrix::BOOTING:
        break;
      case DisplayMatrix::IDLE:
        // Udate the display with the message
        display->matrix.fillScreen(0);
        display->matrix.setCursor(0, 6);
        display->matrix.setTextColor(display->matrix.Color(0, 0, 255));
        display->matrix.print(display->message);
        display->matrix.show();
        break;
      case DisplayMatrix::DISCONNECTED:
        break;
      case DisplayMatrix::UPDATING:
        if(display->updatePercentage != oldPercentage)
        {
          oldPercentage = display->updatePercentage;
          display->matrix.fillScreen(0);
          display->matrix.setCursor(0, 6);
          display->matrix.print("UPDATING");
          display->matrix.setCursor(0, 8);
          display->matrix.print(display->updatePercentage);
          display->matrix.print("%");
          display->matrix.show();
        }
        break;
    }

    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 / MATRIX_UPDATE_RATE);
  }
  vTaskDelete(NULL);
}
