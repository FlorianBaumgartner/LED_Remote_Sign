/******************************************************************************
 * file    Discord.cpp
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

#include "Discord.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include "ArduinoJson.h"
#include "console.h"
#include "device.h"
#include "secrets.h"
#include "utils.h"


Discord::Discord() {}

bool Discord::begin()
{
  Device::getDeviceSerial(myName);
  console.log.printf("[DISCORD] ESP32 Serial Number: %s\n", myName);
  myDeviceIndex = Device::getDeviceIndex();
  if(myDeviceIndex == -1)
  {
    console.error.println("[DISCORD] Device not found in devices list.");
    return false;
  }
  console.log.printf("[DISCORD] I'm the device: %s (List Index: %d)\n", Device::devices[myDeviceIndex].myName, myDeviceIndex);
  console.log.printf("[DISCORD] Listening for messages from: %s\n", Device::devices[myDeviceIndex].receiveMessagesFrom);
  console.log.print("[DISCORD] Listening for events from: ");
  for(int i = 0; i < Device::devices[myDeviceIndex].receiveEventsFromCount; i++)
  {
    console.log.printf("%s", Device::devices[myDeviceIndex].receiveEventsFrom[i]);
    if(i < Device::devices[myDeviceIndex].receiveEventsFromCount - 1)
    {
      console.log.print(", ");
    }
  }
  console.log.println();

  // Unscramble the Discord API URL and the Discord Bot Token
  apiUrl = unscrambleKey(DISCORD_API_URL, sizeof(DISCORD_API_URL) - 1);
  apiToken = unscrambleKey(DISCORD_BOT_TOKEN, sizeof(DISCORD_BOT_TOKEN) - 1);

  xTaskCreate(updateTask, "discord", 8096, this, 5, NULL);
  console.ok.println("[DISCORD] Started");
  return true;
}

void Discord::sendEvent(const char* event)
{
  eventMessageToSend = String(event);
  outgoingEventFlag = true;
}

bool Discord::checkForMessages()
{
  WiFiClientSecure client;
  client.setInsecure();    // Using insecure connection for testing

  String lastMessageId = "";
  bool foundMessage = false;
  bool foundEvent = false;
  bool firstRun = true;
  int chuckCount = 0;

  while(!foundMessage)
  {
    // if(chuckCount > 0)    // If we are in the second chunk, we need to load the next chunk
    // {
    //   console.log.println("[DISCORD] Message not found in this chunk, loading next chunk.");
    // }

    if(outgoingEventFlag)    // Check if there's an event to send
    {
      console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
      return false;
    }

    String apiUrlWithLimit = apiUrl + "&limit=" + String(maxMessageCountPerRequest);
    if(lastMessageId.length() > 0)    // If there's a last message ID, use it to fetch older messages
    {
      apiUrlWithLimit += "&before=" + lastMessageId;
    }

    if(outgoingEventFlag)    // Early exit before connection setup
    {
      console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
      return false;
    }

    if(!client.connect(discordHost, httpsPort))
    {
      console.error.println("[DISCORD] Connection to Discord failed!");
      return false;
    }

    if(outgoingEventFlag)    // Early exit before sending request
    {
      console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
      client.stop();
      return false;
    }

    client.print(String("GET ") + apiUrlWithLimit + " HTTP/1.1\r\n" + "Host: " + discordHost + "\r\n" + "Authorization: Bot " + apiToken + "\r\n" +
                 "User-Agent: ESP32\r\n" + "Connection: close\r\n\r\n");

    while(client.connected())    // Read the response header
    {
      if(outgoingEventFlag)    // Early exit during response header reading
      {
        console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
        client.stop();
        return false;
      }

      String line = client.readStringUntil('\n');
      if(line == "\r")
      {
        break;
      }
    }

    String payload;
    payload.reserve(1024);
    while(client.connected() || client.available())
    {
      if(outgoingEventFlag)    // Early exit during payload reading
      {
        console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
        client.stop();
        return false;
      }
      payload += client.readString();
    }

    if(firstRun)    // Check if the latest discord payload is the same as the last one, if so we don't need to process the messages
    {
      if(payload == latestDiscordPayload)
      {
        client.stop();
        return false;
      }
      latestDiscordPayload = payload;
      firstRun = false;
    }

    // Trim to get valid JSON content
    int start = payload.indexOf('[');
    payload = payload.substring(start);
    int end = payload.lastIndexOf(']');
    payload = payload.substring(0, end + 1);

    if(outgoingEventFlag)    // Early exit before JSON deserialization
    {
      console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
      return false;
    }

    static JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if(error)
    {
      console.error.printf("[DISCORD] Failed to parse JSON: %s\n", error.c_str());
      client.stop();
      break;
    }
    if(doc.isNull() || doc.size() == 0)
    {
      console.error.println("[DISCORD] No messages available.");
      client.stop();
      break;
    }

    for(int i = 0; i < doc.size(); i++)
    {
      String discordEntry = doc[i]["content"].as<String>();
      if(discordEntry.startsWith(
           (String(Device::devices[myDeviceIndex].receiveMessagesFrom) + ":").c_str()))    // Search for the last message entry that is meant
      {
        String sender = discordEntry.substring(0, discordEntry.indexOf(":"));
        discordEntry.remove(0, strlen(Device::devices[myDeviceIndex].receiveMessagesFrom) + 1);    // Remove the sender from the message
        if(discordEntry == latestMessage)    // If the message is the same as the last one, we don't need to process it
        {
          client.stop();
          return false;
        }
        latestMessage = discordEntry;
        newMessageFlag = true;
        console[COLOR_MAGENTA].printf("[DISCORD] New Message received from [%s]: %s\n", sender.c_str(), latestMessage.c_str());
        console[COLOR_DEFAULT].print("");
        foundMessage = true;
        client.stop();
        return true;
      }
      if(!foundEvent)    // While we are searching the latest message, we can also check for events
      {
        for(int j = 0; j < Device::devices[myDeviceIndex].receiveEventsFromCount; j++)
        {
          if(discordEntry.startsWith((String(Device::devices[myDeviceIndex].receiveEventsFrom[j]) + "_").c_str()))
          {
            uint32_t timestamp = discordEntry.substring(discordEntry.indexOf("_") + 1, discordEntry.indexOf(":")).toInt();
            String event = discordEntry.substring(discordEntry.indexOf(":") + 1);

            if(Utils::getUnixTime() - EVENT_VALIDITY_TIME <= timestamp)    // Check if the event is still valid
            {
              if(latestEvent.type == event && latestEvent.timestamp == timestamp)    // Ignore the event if it's the same as the last one
              {
                break;
              }
              latestEvent = Event(event, timestamp);
              newEventFlag = true;
              foundEvent = true;
              console[COLOR_CYAN].printf("[DISCORD] New Event received from [%s]: %s\n", Device::devices[myDeviceIndex].receiveEventsFrom[j],
                                         event.c_str());
              console[COLOR_DEFAULT].print("");
              break;
            }
          }
        }
      }
      lastMessageId = doc[i]["id"].as<String>();    // Track the ID of the last message in this chunk for pagination
    }
    client.stop();
    chuckCount++;
  }
  if(!foundMessage)
  {
    console.log.printf("[DISCORD] No message containing '%s' found.\n", Device::devices[myDeviceIndex].receiveMessagesFrom);
  }
  return true;
}


bool Discord::checkForOutgoingEvents()
{
  if(eventMessageToSend.length() == 0)
  {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();    // Using insecure connection for testing
  if(!client.connect(discordHost, httpsPort))
  {
    console.error.println("[DISCORD] Connection to Discord failed!");
    return false;
  }
  String eventString = String(myName) + "_" + String(Utils::getUnixTime()) + ":" + eventMessageToSend;
  String payload = "{\"content\":\"" + eventString + "\"}";
  String apiUrlWithLimit = apiUrl + "&limit=" + String(maxMessageCountPerRequest);

  client.print(String("POST ") + apiUrlWithLimit + " HTTP/1.1\r\n" + "Host: " + discordHost + "\r\n" + "Authorization: Bot " + apiToken + "\r\n" +
               "User-Agent: ESP32\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + payload.length() + "\r\n" +
               "Connection: close\r\n\r\n" + payload);

  while(client.connected())    // Read the response header
  {
    String line = client.readStringUntil('\n');
    if(line == "\r")
    {
      break;
    }
  }

  String response;
  while(client.connected() || client.available())
  {
    response += client.readString();
  }
  // if(!response.startsWith("24d"))    // Not really sure
  // {
  //   console.error.printf("[DISCORD] Failed to send event: %s\n", response.c_str());
  //   client.stop();
  //   return false;
  // }
  console.ok.printf("[DISCORD] Event sent: %s\n", eventMessageToSend.c_str());
  client.stop();
  eventMessageToSend = "";
  outgoingEventFlag = false;
  return true;
}


void Discord::updateTask(void* param)
{
  Discord* ref = (Discord*)param;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    if(WiFi.status() == WL_CONNECTED && ref->enabled)
    {
      ref->checkForOutgoingEvents();    // Check if there are events to send
      ref->checkForMessages();          // Check is server is available and if an update is available
    }
    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 * DISCORD_UPDATE_INTERVAL);
  }
  vTaskDelete(NULL);
}
