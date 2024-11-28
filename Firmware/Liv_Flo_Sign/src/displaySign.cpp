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

constexpr const float DisplaySign::canvas_center[2] = {64.35, 70.5};
constexpr const float DisplaySign::canvas_min_max_x[2] = {9.875, 132.65};
constexpr const float DisplaySign::square_coordinates[268][2] = {
  {11.7, 66.075},    {12.7, 67.475},    {13.6, 69.025},    {14.325, 70.55},   {15.0, 72.2},      {15.575, 73.825},  {16.0, 75.525},
  {16.25, 77.25},    {16.3, 78.975},    {16.175, 80.65},   {15.75, 82.275},   {14.475, 83.65},   {12.85, 82.7},     {12.05, 81.025},
  {11.55, 79.4},     {11.2, 77.75},     {10.925, 76.075},  {10.675, 74.35},   {10.45, 72.625},   {10.325, 70.925},  {10.15, 69.25},
  {10.0, 67.5},      {9.9, 65.875},     {9.875, 64.225},   {9.875, 62.575},   {10.0, 60.9},      {10.25, 59.175},   {10.675, 57.55},
  {11.3, 55.95},     {12.3, 54.45},     {13.85, 53.45},    {15.7, 53.25},     {17.425, 54.125},  {18.625, 55.525},  {19.55, 57.0},
  {20.725, 58.425},  {20.75, 60.125},   {20.75, 61.9},     {20.65, 63.65},    {20.6, 65.4},      {20.575, 67.1},    {20.575, 68.875},
  {20.575, 70.65},   {20.575, 72.4},    {21.625, 79.7},    {19.975, 79.7},    {20.7, 81.35},     {21.2, 56.0},      {21.55, 54.275},
  {22.775, 52.675},  {24.775, 53.275},  {25.975, 54.6},    {26.925, 56.05},   {27.7, 57.6},      {28.775, 59.2},    {28.825, 60.95},
  {28.775, 62.725},  {28.725, 64.55},   {28.725, 66.35},   {28.725, 68.1},    {28.75, 69.9},     {28.825, 71.625},  {28.925, 73.35},
  {29.475, 57.025},  {29.9, 55.3},      {30.65, 53.575},   {32.325, 52.375},  {34.025, 53.65},   {34.975, 55.3},    {35.6, 56.95},
  {36.125, 58.725},  {36.55, 60.425},   {36.9, 62.175},    {37.1, 63.925},    {37.4, 65.85},     {37.525, 67.525},  {37.575, 69.2},
  {37.525, 70.9},    {37.3, 72.675},    {36.2, 74.25},     {34.925, 72.625},  {34.75, 70.725},   {34.85, 68.925},   {35.125, 67.175},
  {35.6, 65.375},    {38.375, 60.1},    {39.5, 58.8},      {40.8, 57.575},    {42.25, 56.5},     {43.825, 55.575},  {45.4, 54.775},
  {47.1, 54.15},     {48.875, 53.7},    {50.725, 53.4},    {52.525, 53.3},    {54.325, 53.325},  {56.1, 53.5},      {57.85, 53.8},
  {59.55, 54.325},   {61.25, 55.075},   {62.825, 55.875},  {64.35, 56.8},     {65.825, 57.825},  {67.225, 58.875},  {68.575, 59.975},
  {69.875, 61.2},    {71.075, 62.5},    {72.175, 63.875},  {73.2, 65.325},    {74.1, 66.85},     {74.9, 68.375},    {75.575, 69.975},
  {76.1, 71.6},      {76.5, 73.25},     {76.75, 75.025},   {76.8, 76.775},    {76.575, 78.525},  {76.05, 80.25},    {75.025, 81.8},
  {73.5, 82.925},    {71.75, 83.5},     {69.9, 83.45},     {68.15, 82.875},   {66.625, 81.9},    {65.425, 80.575},  {64.475, 79.0},
  {64.475, 77.275},  {63.275, 80.375},  {61.975, 81.675},  {60.35, 82.6},     {58.525, 82.925},  {56.7, 82.7},      {55.05, 81.95},
  {53.65, 80.7},     {52.75, 79.075},   {52.3, 77.3},      {52.225, 75.55},   {52.425, 73.775},  {52.875, 72.025},  {53.525, 70.375},
  {54.275, 68.75},   {55.125, 67.25},   {56.1, 65.775},    {57.075, 64.375},  {58.1, 63.025},    {59.225, 61.675},  {60.425, 60.35},
  {61.65, 59.125},   {62.925, 57.95},   {66.05, 55.775},   {67.65, 55.025},   {69.25, 54.375},   {70.95, 53.85},    {72.675, 53.525},
  {74.4, 53.325},    {76.175, 53.275},  {77.925, 53.25},   {79.675, 53.375},  {81.375, 53.6},    {83.075, 53.95},   {84.75, 54.45},
  {86.375, 55.025},  {87.95, 55.75},    {89.4, 56.525},    {90.85, 57.4},     {92.225, 58.4},    {93.85, 50.325},   {93.85, 52.05},
  {93.85, 53.8},     {93.85, 55.575},   {93.9, 57.375},    {93.95, 59.175},   {93.975, 60.85},   {94.0, 62.525},    {94.0, 64.3},
  {94.0, 66.1},      {94.075, 67.875},  {94.15, 69.625},   {94.225, 71.375},  {94.325, 73.1},    {94.425, 74.825},  {94.475, 76.575},
  {91.175, 78.1},    {92.8, 78.15},     {94.45, 78.3},     {96.15, 78.475},   {97.875, 78.7},    {99.575, 78.975},  {101.275, 79.2},
  {102.975, 79.35},  {104.675, 79.45},  {95.725, 64.225},  {97.425, 64.175},  {99.125, 64.175},  {100.875, 64.25},  {102.5, 64.4},
  {104.2, 64.675},   {105.85, 65.175},  {107.475, 66.05},  {108.8, 67.275},   {109.9, 68.675},   {110.825, 70.2},   {111.6, 71.775},
  {112.225, 73.4},   {112.725, 75.1},   {113.075, 76.825}, {113.175, 78.55},  {112.8, 80.375},   {111.325, 81.7},   {109.65, 80.65},
  {108.65, 79.125},  {107.9, 77.525},   {107.25, 75.875},  {106.725, 74.125}, {106.35, 72.375},  {106.075, 70.6},   {105.85, 68.85},
  {105.7, 67.05},    {105.875, 63.325}, {106.1, 61.6},     {106.45, 59.9},    {106.925, 58.225}, {107.525, 56.55},  {108.35, 54.925},
  {109.675, 53.525}, {111.475, 52.775}, {113.25, 53.725},  {114.275, 55.275}, {114.875, 56.875}, {115.45, 58.5},    {116.125, 60.15},
  {116.875, 61.725}, {117.75, 63.175},  {118.85, 64.475},  {119.6, 66.3},     {119.75, 68.05},   {120.075, 69.775}, {120.575, 71.475},
  {121.275, 73.075}, {122.175, 74.625}, {123.325, 76.075}, {124.8, 77.275},   {126.675, 77.825}, {128.5, 76.925},   {129.625, 75.525},
  {130.5, 74.0},     {131.25, 72.425},  {131.825, 70.8},   {132.275, 69.125}, {132.55, 67.375},  {132.65, 65.6},    {132.55, 63.85},
  {132.3, 62.175},   {132.025, 60.5},   {131.625, 58.825}, {131.075, 57.2},   {130.375, 55.65},  {129.35, 54.125},  {128.0, 52.95},
  {126.375, 52.125}, {124.5, 51.975},   {122.675, 52.7},   {121.375, 54.025}, {120.5, 55.6},     {120.025, 57.3},   {119.7, 59.0},
  {119.55, 60.675},  {119.475, 62.325},
};

// Cosine lookup table for values between -1 and 1 scaled to an integer range [-1000, 1000]
constexpr const int16_t DisplaySign::cos_lut[360] = {
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

void DisplaySign::begin(float updateRate)
{
  this->updateRate = updateRate;
  pixels.begin();
  pixels.clear();
  pixels.show();
}

void DisplaySign::setBrightness(uint8_t brightness)
{
  pixels.setBrightness(brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness);
}

void DisplaySign::updateTask(void)
{
  if(booting)
  {
    animationBooting();
    return;
  }
  if(!enabled)
  {
    pixels.clear();
    pixels.show();
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

  if(nightMode)
  {
    animationNightMode(framecount, eventFlag);
  }
  else
  {
    // TODO: Add Switch for different animations
    animationSine(framecount, eventFlag);
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
      pixels.setPixelColor(i, bootColor);
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

void DisplaySign::animationNightMode(uint32_t framecount, bool eventFlag)
{
  pixels.clear();
  pixels.fill(nightLightColor, 101, 48);    // Only heart is lit
  pixels.show();
}

void DisplaySign::animationSine(uint32_t framecount, bool eventFlag)
{
  const float speed = 0.65;        // [Hz]
  const float wavelength = 2.5;    // Wavelength of the sine wave
  const uint8_t high_color[3] = {0xFF, 0x08, 0x08};
  const uint8_t low_color[3] = {0xFF, 0x54, 0x00};    // Match Python colors
  const float total_range_left = canvas_center[0] - canvas_min_max_x[0];
  const float total_range_right = canvas_min_max_x[1] - canvas_center[0];

  int16_t angle_offset = -fmodf(framecount * speed, 360);
  if(angle_offset < 0)
    angle_offset += 360;

  for(int i = 0; i < pixels.numPixels(); i++)
  {
    int16_t x = square_coordinates[i][0] - canvas_center[0];
    float relative_x, total_range, normalized_position;
    if(x < 0)
    {
      relative_x = canvas_center[0] - square_coordinates[i][0];    // Distance from left side of center
      total_range = total_range_left * wavelength;
    }
    else
    {
      relative_x = square_coordinates[i][0] - canvas_center[0];    // Distance from right side of center
      total_range = total_range_right * wavelength;
    }
    normalized_position = relative_x / total_range;
    int16_t angle = (static_cast<int16_t>(normalized_position * 360) + angle_offset) % 360;
    if(angle < 0)
      angle += 360;                  // Ensure angle is positive
    int16_t val = cos_lut[angle];    // LUT contains cosine values scaled to [-1000, 1000]
    uint8_t red = low_color[0] + (val + 1000) * (high_color[0] - low_color[0]) / 2000;
    uint8_t green = low_color[1] + (val + 1000) * (high_color[1] - low_color[1]) / 2000;
    uint8_t blue = low_color[2] + (val + 1000) * (high_color[2] - low_color[2]) / 2000;
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}
