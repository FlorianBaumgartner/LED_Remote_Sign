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
#include "../tools/Emoji/emoji_bitmaps.h"
#include "console.h"

void DisplayMatrix::begin(void)
{
  matrix.begin();
  matrix.setTextSize(1);
  matrix.setFont(&Grand9K_Pixel8pt7bModified);
  matrix.setTextWrap(false);
  matrix.setBrightness(DEFAULT_BRIGHNESS);
  matrix.setTextColor(matrix.Color(0, 0, 255));
  matrix.fillScreen(0);
  matrix.show();

  xTaskCreate(updateTask, "matrix", 4096, this, 15, NULL);
}


size_t DisplayMatrix::printMessage(const String& msg, int offset)
{
  size_t textWidth = 0;
  int utf8_code_length = 0;
  int utf8_code_index = 0;
  for(int i = 0; i < msg.length(); i++)
  {
    matrix.setCursor(textWidth + offset, 6);
    int emojiWidth = 0;

    if((msg[i] & 0x80) == 0x00)    // Check if the character is ASCII
    {
      utf8_code_index = 0;
      utf8_code_length = 0;
      matrix.write(msg[i]);
    }
    else if((msg[i] & 0xC0) == 0x80)    // Check if the character is a continuation of a UTF-8 character
    {
      utf8_code_index++;
      if(utf8_code_index == 1 && utf8_code_length == 1)
      {
        console.warning.printf("[DISP_MAT] Invalid UTF-8 continuation: %02X\n", msg[i]);
      }
      else if(utf8_code_index == 2 && utf8_code_length == 2)
      {
        if(msg[i - 1] == 0xC2)
        {
          matrix.write(msg[i]);
        }
        else if(msg[i - 1] == 0xC3)
        {
          matrix.write(msg[i] + 0x40);
        }
        else
        {
          console.warning.printf("[DISP_MAT] Invalid UTF-8 continuation: %02X\n", msg[i]);
        }
      }
      else if(utf8_code_index == 3 && utf8_code_length == 3)
      {
        uint32_t unicode_index = (msg[i - 2] & 0x0F) << 12 | (msg[i - 1] & 0x3F) << 6 | (msg[i] & 0x3F);
        // console.log.printf("[DISP_MAT] Loading 3-byte UTF-8 character: emoji_%x\n", unicode_index);
        emojiWidth = drawEmoji(textWidth + offset, 0, unicode_index)? 8 : 0;
      }
      else if(utf8_code_index == 4 && utf8_code_length == 4)
      {
        uint32_t unicode_index = (msg[i - 3] & 0x07) << 18 | (msg[i - 2] & 0x3F) << 12 | (msg[i - 1] & 0x3F) << 6 | (msg[i] & 0x3F);
        // console.log.printf("[DISP_MAT] Loading 4-byte UTF-8 character: emoji_%x\n", unicode_index);
        emojiWidth = drawEmoji(textWidth + offset, 0, unicode_index)? 8 : 0;
      }
    }
    else if((msg[i] & 0xE0) == 0xC0)    // Check if the character is a 2-byte UTF-8 character
    {
      utf8_code_length = 2;
      utf8_code_index = 1;
    }
    else if((msg[i] & 0xF0) == 0xE0)    // Check if the character is a 3-byte UTF-8 character
    {
      utf8_code_length = 3;
      utf8_code_index = 1;
    }
    else if((msg[i] & 0xF8) == 0xF0)    // Check if the character is a 4-byte UTF-8 character
    {
      utf8_code_length = 4;
      utf8_code_index = 1;
    }
    else
    {
      utf8_code_index = 0;
      utf8_code_length = 0;
      console.warning.printf("[DISP_MAT] Invalid UTF-8 character: %02X\n", msg[i]);
    }
    textWidth = matrix.getCursorX() + emojiWidth;
  }
  return textWidth;
}


bool DisplayMatrix::drawEmoji(uint8_t x, uint8_t y, uint32_t unicode_index)
{
  bool found = false;
  for(int i = 0; i < emoji_count; i++)   // Search for emoji based on unicode index
  {
    if(emojis[i].unicode == unicode_index)
    {
      found = true;
      for(int j = 0; j < 7; j++)
      {
        for(int k = 0; k < 7; k++)
        {
          uint32_t color = emojis[i].data[j][k][0] << 16 | emojis[i].data[j][k][1] << 8 | emojis[i].data[j][k][2];
          matrix.setPassThruColor(color);
          matrix.drawPixel(x + k, y + j, color);
        }
      }
    }
  }
  if(!found)
  {
    if(unicode_index != 0xFE0F)   // Ignore the "⬢" character of the Heart Rate Emoji (U+1F497)
    {
      char unicode_char[4];
      unicode_char[0] = (unicode_index >> 16) & 0xFF;
      unicode_char[1] = (unicode_index >> 8) & 0xFF;
      unicode_char[2] = unicode_index & 0xFF;
      unicode_char[3] = '\0';
      console.warning.printf("[DISP_MAT] Emoji not found: emoji_%x (%s)\n", unicode_index, unicode_char);
    }
  }
  matrix.setPassThruColor();
  return found;
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
        display->matrix.setPassThruColor(display->textColor);
        display->printMessage(display->message, 0);
        display->matrix.setPassThruColor();
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
