/******************************************************************************
 * file    displaySign.h
 *******************************************************************************
 * brief   Handles the LED Sign Display
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-20
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

#include "DisplaySign.h"
#include "console.h"

void DisplaySign::begin(float updateRate)
{
  this->updateRate = updateRate;
  pixels.begin();
  pixels.clear();
  pixels.show();
}


void DisplaySign::updateTask(void)
{
  if(!enabled)
  {
    return;
  }
  static int pos = 0;
  for(int i = 0; i < pixels.numPixels(); i++)
  {
    if(i < pos)
    {
      pixels.setPixelColor(i, 0xFC5400);
      // uint8_t r = millis() / 10 % 255;
      // uint8_t g = ((millis() / 10) + (255 / 3)) % 255;
      // uint8_t b = ((millis() / 10) + (2 * 255 / 3)) % 255;
      // pixels.setPixelColor(i, r, g, b);
    }
    else
    {
      pixels.setPixelColor(i, 0, 0, 0);
    }
  }
  pos = (pos + 1) % pixels.numPixels();

  pixels.show();
}

void DisplaySign::enable(bool enable)
{
  enabled = enable;
  if(!enabled)
  {
    pixels.clear();
    pixels.show();
  }
}
