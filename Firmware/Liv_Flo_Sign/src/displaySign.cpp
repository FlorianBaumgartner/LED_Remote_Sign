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

  if(booting)
  {
    animationBooting();
    return;
  }

  static bool eventOld = false;
  bool eventFlag = false;
  if(event && !eventOld)
  {
    eventFlag = true;
  }
  eventOld = event;
  event = false;

  static uint32_t framecount = 0;
  framecount++;

  uint32_t t = millis();
  animationSine(framecount, eventFlag);
  t = millis() - t;

  // static int u = 0;
  // if(millis() - u > 1000)
  // {
  //   console.log.printf("[DISP_SIG] Frame time: %d ms\n", t);
  //   u = millis();
  // }
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


void DisplaySign::animationBooting(void)
{
  static float pos = 0;
  static const float speed = 98;    // [pixel/s]

  pixels.clear();
  for(int i = 0; i < pixels.numPixels(); i++)
  {
    bool rampState = pos < pixels.numPixels();
    if((rampState && i < pos) || (!rampState && i >= pos - pixels.numPixels()))
    {
      pixels.setPixelColor(i, 0xFFFFFF);
    }
  }
  if(pos < pixels.numPixels() * 2)
  {
    pos += speed / updateRate;
  }
  else
  {
    booting = false;
  }
  pixels.show();
}

void DisplaySign::animationSine(uint32_t framecount, bool eventFlag)
{
  const int16_t speed = 5;             // Integer speed multiplier
  const int16_t wavelength_mm = 80;    // Integer wavelength
  const uint8_t high_color[3] = {0xFF, 0x00, 0x00};
  const uint8_t low_color[3] = {0xFF, 0x00, 0xFF};

  // Cosine lookup table for values between -1 and 1 scaled to an integer range [-1000, 1000]
  static const int16_t cos_lut[360] = {
    1000, 999,  999,  998,  997,  996,  994,  992,  990,  987,  984,  981,  978,   974,  970,  965,  961,  956,  951,  945,  939,  933,  927,  920,
    913,  906,  898,  891,  882,  874,  866,  857,  848,  838,  829,  819,  809,   798,  788,  777,  766,  754,  743,  731,  719,  707,  694,  681,
    669,  656,  642,  629,  615,  601,  587,  573,  559,  544,  529,  515,  500,   484,  469,  453,  438,  422,  406,  390,  374,  358,  342,  325,
    309,  292,  275,  258,  241,  224,  207,  190,  173,  156,  139,  121,  104,   87,   69,   52,   34,   17,   0,    -17,  -34,  -52,  -69,  -87,
    -104, -121, -139, -156, -173, -190, -207, -224, -241, -258, -275, -292, -309,  -325, -342, -358, -374, -390, -406, -422, -438, -453, -469, -484,
    -499, -515, -529, -544, -559, -573, -587, -601, -615, -629, -642, -656, -669,  -681, -694, -707, -719, -731, -743, -754, -766, -777, -788, -798,
    -809, -819, -829, -838, -848, -857, -866, -874, -882, -891, -898, -906, -913,  -920, -927, -933, -939, -945, -951, -956, -961, -965, -970, -974,
    -978, -981, -984, -987, -990, -992, -994, -996, -997, -998, -999, -999, -1000, -999, -999, -998, -997, -996, -994, -992, -990, -987, -984, -981,
    -978, -974, -970, -965, -961, -956, -951, -945, -939, -933, -927, -920, -913,  -906, -898, -891, -882, -874, -866, -857, -848, -838, -829, -819,
    -809, -798, -788, -777, -766, -754, -743, -731, -719, -707, -694, -681, -669,  -656, -642, -629, -615, -601, -587, -573, -559, -544, -529, -515,
    -500, -484, -469, -453, -438, -422, -406, -390, -374, -358, -342, -325, -309,  -292, -275, -258, -241, -224, -207, -190, -173, -156, -139, -121,
    -104, -87,  -69,  -52,  -34,  -17,  0,    17,   34,   52,   69,   87,   104,   121,  139,  156,  173,  190,  207,  224,  241,  258,  275,  292,
    309,  325,  342,  358,  374,  390,  406,  422,  438,  453,  469,  484,  500,   515,  529,  544,  559,  573,  587,  601,  615,  629,  642,  656,
    669,  681,  694,  707,  719,  731,  743,  754,  766,  777,  788,  798,  809,   819,  829,  838,  848,  857,  866,  874,  882,  891,  898,  906,
    913,  920,  927,  933,  939,  945,  951,  956,  961,  965,  970,  974,  978,   981,  984,  987,  990,  992,  994,  996,  997,  998,  999,  999};


  int16_t angle_offset = (framecount * -speed) % 360;    // Negative to reverse direction
  for(int i = 0; i < pixels.numPixels(); i++)
  {
    int16_t x = square_coordinates[i][0] - canvas_center[0];
    int16_t angle = ((abs(x) * 360) / wavelength_mm + angle_offset) % 360;
    int16_t val = cos_lut[angle];    // Lookup cosine value
    uint8_t red = low_color[0] + (val + 1000) * (high_color[0] - low_color[0]) / 2000;
    uint8_t green = low_color[1] + (val + 1000) * (high_color[1] - low_color[1]) / 2000;
    uint8_t blue = low_color[2] + (val + 1000) * (high_color[2] - low_color[2]) / 2000;
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}
