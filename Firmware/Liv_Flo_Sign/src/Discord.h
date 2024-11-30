/******************************************************************************
 * file    discord.h
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
#include <ESP_SSLClient.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "utils.h"

// Message strcuture: "<Sender>:<Message>"
// Example message: "PHONE_LIV:Hello World! ✨"

// Event structure: "<Sender>_<UnixTimestamp>:<Event>"
// Example event: "AC6EBB03F784_1633363200:ButtonTrigger"


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
  constexpr static const int MAX_MESSAGE_COUNT_PER_REQUEST = 15;
  constexpr static const float DISCORD_UPDATE_INTERVAL = 5.0;    // [s]  Interval to check for new messages
  constexpr static const float SERVER_SLOW_DOWN_TIME = 3.0;      // [s]  Time to wait after server asked to slow down
  constexpr static const int EVENT_VALIDITY_TIME = 20;           // [s]  Time within an event is seen as new and therefore valid

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
  bool newEventAvailable(bool clearFlag = true)
  {
    bool flag = newEventFlag;
    if(clearFlag)
      newEventFlag = false;
    return flag;
  }
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
  bool enabled = false;

  constexpr static const int httpsPort = 443;    // Not used, since we connect by URL (https://)
  constexpr static const char* discordHost = "discord.com";

  HTTPClient http;
  WiFiClient base_client;
  ESP_SSLClient client;

  bool checkForMessages();
  bool checkForOutgoingEvents();
  static void updateTask(void* pvParameter);
};


#endif
