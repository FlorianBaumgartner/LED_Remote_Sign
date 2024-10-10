/******************************************************************************
 * file    Discord.h
 *******************************************************************************
 * brief   Discord Communication
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-04
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

#ifndef DISCORD_H
#define DISCORD_H

#include <Arduino.h>

#define ESP32C3_DEV_BOARD_RGB_LED "AC6EBB03F784"
#define ESP32C3_DEV_BOARD_LCD     "542C474E76A0"

#define PHONE_LIV                 "PHONE_LIV"
#define PHONE_FLO                 "PHONE_FLO"


#define DISCORD_UPDATE_INTERVAL   0.25    // [s]  Interval to check for new messages
#define EVENT_VALIDITY_TIME       10      // [s]  Time within an event is seen as new and therefore valid

class Devices
{
 public:
  constexpr static const int maxMessageCountPerRequest = 15;

  Devices(const char* name, const char* receiveMessagesFrom, const char** receiveEventsFrom, const int receiveEventsFromCount)
      : myName(name), receiveMessagesFrom(receiveMessagesFrom), receiveEventsFrom(receiveEventsFrom), receiveEventsFromCount(receiveEventsFromCount)
  {}
  const char* myName;
  const char* receiveMessagesFrom;
  const char** receiveEventsFrom;
  const int receiveEventsFromCount;
};

class Discord
{
 public:
  Discord();
  bool begin();
  String& getLatestMessage() { return latestMessage; }
  bool newMessageAvailable(bool clearFlag = true)
  {
    bool flag = newMessageFlag;
    if(clearFlag)
      newMessageFlag = false;
    return flag;
  }


 private:
  String apiToken;
  String apiUrl;

  char myName[20];
  String latestDiscordPayload = "";
  String latestMessage = "";
  String latestEvent = "";
  int myDeviceIndex = -1;
  bool newMessageFlag = false;


  const Devices devices[2] = {Devices(ESP32C3_DEV_BOARD_RGB_LED, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_LCD}, 1),
                              Devices(ESP32C3_DEV_BOARD_LCD, PHONE_LIV, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED}, 1)};

  constexpr static const int httpsPort = 443;
  constexpr static const char* discordHost = "discord.com";

  void getDeviceName(char* deviceName);
  bool checkForMessages();
  static void updateTask(void* pvParameter);
};


#endif
