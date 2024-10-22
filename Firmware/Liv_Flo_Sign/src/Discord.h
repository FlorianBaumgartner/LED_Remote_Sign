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


// Message strcuture: "Sender:Message"
// Example message: "PHONE_LIV:Hello World! ✨"

// Event structure: "Sender_UnixTimestamp:Event"
// Example event: "AC6EBB03F784_1633363200:ButtonTrigger"


#define DISCORD_UPDATE_INTERVAL   1.0    // [s]  Interval to check for new messages
#define EVENT_VALIDITY_TIME       20     // [s]  Time within an event is seen as new and therefore valid

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

class Event
{
 public:
  Event(String type, uint32_t timestamp) : type(type), timestamp(timestamp) {}
  String type;
  uint32_t timestamp;
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
  bool getLatestEvent(String& event)
  {
    if(latestEvent.type.length() == 0)
      return false;
    event = latestEvent.type;
    return true;
  }
  bool newEventAvailable() { return newEventFlag; }
  void sendEvent(const char* event);
  void enable(bool enable) { enabled = enable; }


 private:
  String apiToken;
  String apiUrl;

  char myName[20];
  String latestDiscordPayload = "";
  String latestMessage = "";
  Event latestEvent = Event("", 0);
  String eventMessageToSend = "";
  int myDeviceIndex = -1;
  bool newMessageFlag = false;
  bool newEventFlag = false;
  bool outgoingEventFlag = false;
  bool enabled = true;


  const Devices devices[5] = {Devices(LIV_FLO_SIGN_0, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_LCD, ESP32S3_DEV_BOARD_BLING}, 2),
                              Devices(LIV_FLO_SIGN_1, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED, ESP32C3_DEV_BOARD_LCD}, 2),

                              Devices(ESP32C3_DEV_BOARD_RGB_LED, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_LCD, ESP32S3_DEV_BOARD_BLING}, 2),
                              Devices(ESP32C3_DEV_BOARD_LCD, PHONE_LIV, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED, ESP32S3_DEV_BOARD_BLING}, 2),
                              Devices(ESP32S3_DEV_BOARD_BLING, PHONE_FLO, (const char*[]){ESP32C3_DEV_BOARD_RGB_LED, ESP32C3_DEV_BOARD_LCD}, 2)};

  constexpr static const int httpsPort = 443;
  constexpr static const char* discordHost = "discord.com";

  void getDeviceName(char* deviceName);
  bool checkForMessages();
  bool checkForOutgoingEvents();
  static void updateTask(void* pvParameter);
};


#endif
