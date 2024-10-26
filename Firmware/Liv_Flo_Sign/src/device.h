/******************************************************************************
 * file    device.h
 *******************************************************************************
 * brief   Contains all device and message routing information
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-26
 *******************************************************************************
 * MIT License
 *
 * Copyright (c) 2024 Crelin - Florian Baumgartner
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

#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>

#define LIV_FLO_SIGN_0            "D4E2D49E9EF0"    // Flo's sign (based in the appartment)
#define LIV_FLO_SIGN_1            "CCD0D49E9EF0"    // Flo's sign (based in the lab)
#define LIV_FLO_SIGN_2            "3C17D29E9EF0"    // Liv's sign ()
#define LIV_FLO_SIGN_3            "50FCD49E9EF0"    // Liv's sign ()
#define LIV_FLO_SIGN_4            "309ED59E9EF0"    // Backup

#define ESP32C3_DEV_BOARD_RGB_LED "AC6EBB03F784"
#define ESP32C3_DEV_BOARD_LCD     "542C474E76A0"
#define ESP32S3_DEV_BOARD_BLING   "A869D87554DC"

#define PHONE_LIV                 "PHONE_LIV"
#define PHONE_FLO                 "PHONE_FLO"


class Device
{
 public:
  Device(const char* name, const char* receiveMessagesFrom, const char** receiveEventsFrom, const int receiveEventsFromCount)
      : myName(name), receiveMessagesFrom(receiveMessagesFrom), receiveEventsFrom(receiveEventsFrom), receiveEventsFromCount(receiveEventsFromCount)
  {}

  const char* myName;
  const char* receiveMessagesFrom;
  const char** receiveEventsFrom;
  const int receiveEventsFromCount;

  static void getDeviceSerial(char* deviceSerial)
  {
    uint64_t chipId = ESP.getEfuseMac();
    sprintf(deviceSerial, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  }

  static int getDeviceIndex()
  {
    char myName[13];
    getDeviceSerial(myName);
    for(int i = 0; i < sizeof(devices) / sizeof(devices[0]); i++)
    {
      if(strcmp(devices[i].myName, myName) == 0)
      {
        return i;
      }
    }
    return -1;
  }

  static String getDeviceName()
  {
    char myName[13];
    getDeviceSerial(myName);
    int i = getDeviceIndex();
    String name = "Liv-Flo Sign ";
    if(i >= 0)
    {
      return name + String(i);
    }
    return name + String(myName);
  }

  static const Device devices[8];
};

#endif