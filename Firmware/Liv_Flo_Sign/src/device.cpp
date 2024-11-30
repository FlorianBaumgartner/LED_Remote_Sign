/******************************************************************************
 * file    device.cpp
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

#include "device.h"

const Device Device::devices[8] = {
  Device(LIV_FLO_SIGN_0, PHONE_FLO, (const char*[]){LIV_FLO_SIGN_2, LIV_FLO_SIGN_3}, 2),                    // Liv's sign (ZHdK)
  Device(LIV_FLO_SIGN_1, PHONE_FLO, (const char*[]){LIV_FLO_SIGN_2, LIV_FLO_SIGN_3}, 2),                    // Liv's sign (bedroom)
  Device(LIV_FLO_SIGN_2, PHONE_LIV, (const char*[]){LIV_FLO_SIGN_0, LIV_FLO_SIGN_1, LIV_FLO_SIGN_3}, 3),    // Flo's sign (development unit)
  Device(LIV_FLO_SIGN_3, PHONE_LIV, (const char*[]){LIV_FLO_SIGN_0, LIV_FLO_SIGN_1, LIV_FLO_SIGN_2}, 3),    // Flo's sign (bedroom)
  Device(LIV_FLO_SIGN_4, PHONE_FLO, (const char*[]){LIV_FLO_SIGN_0, LIV_FLO_SIGN_1}, 2),                    // Backup
  Device(ESP32C3_DEV_BOARD_RGB_LED, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_LCD, ESP32S3_DEV_BOARD_BLING}, 2),
  Device(ESP32C3_DEV_BOARD_LCD, PHONE_LIV, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED, ESP32S3_DEV_BOARD_BLING}, 2),
  Device(ESP32S3_DEV_BOARD_BLING, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED, ESP32C3_DEV_BOARD_LCD}, 2)};
