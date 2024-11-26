/******************************************************************************
 * file    discord.cpp
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

#include "discord.h"
#include <WiFi.h>
#include <esp_system.h>
#include "console.h"
#include "device.h"
#include "secrets.h"
#include "utils.h"

// #include <static_malloc.h>

// constexpr size_t myHeapSize = 1024 * 16;
// static uint8_t myHeap[myHeapSize];


// struct CutsomAllocator : ArduinoJson::Allocator
// {
//  public:
//   void* allocate(size_t size) { return sm_malloc(size); }
//   void deallocate(void* pointer) { sm_free(pointer); }
//   void* reallocate(void* pointer, size_t new_size) { return sm_realloc(pointer, new_size); }
// };

// CutsomAllocator allocator;


Discord::Discord() {}

bool Discord::begin()
{
  // sm_set_default_pool(myHeap, myHeapSize, 0, nullptr);

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

  xTaskCreate(updateTask, "discord", 8192, this, 5, NULL);    // Stack Watermark: 3560
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
  String lastMessageId = "";
  bool foundMessage = false;
  bool foundEvent = false;
  bool firstRun = true;
  int chuckCount = 0;

  client.setTimeout(5000);
  client.setBufferSizes(8192 /* rx */, 512 /* tx */);
  client.setDebugLevel(1);    // none = 0, error = 1, warn = 2, info = 3, dump = 4
  client.setClient(&base_client);
  client.setInsecure();    // Using insecure connection for testing

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

    String url = String("https://") + discordHost + apiUrl + "&limit=" + String(MAX_MESSAGE_COUNT_PER_REQUEST);
    if(lastMessageId.length() > 0)    // If there's a last message ID, use it to fetch older messages
    {
      url += "&before=" + lastMessageId;
    }
    if(!http.begin(client, url))
    {
      console.error.printf("[DISCORD] Server not available\n");
      return false;    // Server not available
    }
    http.addHeader("Authorization", "Bot " + apiToken, true);    // 'true' ensures overwriting default headers
    http.addHeader("User-Agent", "ESP32");
    http.addHeader("Connection", "keep-alive");
    int httpCode = http.GET();
    if(httpCode <= 0)
    {
      console.error.printf("[DISCORD] HTTP GET failed! Error code: %d, reason: %s\n", httpCode, http.errorToString(httpCode).c_str());
      http.end();
      client.stop();
      return false;
    }
    if(httpCode < 200 || httpCode >= 300)
    {
      if(httpCode == 429)
      {
        console.warning.printf("[DISCORD] Rate limited, waiting %.1f seconds.\n", SERVER_SLOW_DOWN_TIME);
        delay(SERVER_SLOW_DOWN_TIME * 1000);
        http.end();
        client.stop();
        continue;
      }
      console.warning.printf("[DISCORD] Unexpected HTTP response code: %d\n", httpCode);
      http.end();
      client.stop();
      return false;
    }
    String payload = http.getString();
    http.end();
    client.stop();

    if(firstRun)    // Check if the latest discord payload is the same as the last one, if so we don't need to process the messages
    {
      String payloadStart = payload.substring(0, 100);
      if(payloadStart == latestDiscordPayload)
      {
        return false;
      }
      latestDiscordPayload = payloadStart;
      firstRun = false;
    }

    // Trim to get valid JSON content
    int start = payload.indexOf('[');
    if(start == -1)
    {
      console.error.println("[DISCORD] No messages available.");
      break;
    }
    int end = payload.lastIndexOf(']');
    if(end == -1)
    {
      console.error.println("[DISCORD] No messages available.");
      break;
    }
    payload = payload.substring(0, end + 1);

    if(outgoingEventFlag)    // Early exit before JSON deserialization
    {
      console.warning.printf("[DISCORD] Abort message search, sending event: %s\n", eventMessageToSend.c_str());
      return false;
    }

    static StaticJsonDocument<12000> doc;
    // DynamicJsonDocument doc(12000);
    DeserializationError error = deserializeJson(doc, payload);
    if(error)
    {
      console.error.printf("[DISCORD] Failed to parse JSON: %s\n", error.c_str());
      break;
    }
    if(doc.isNull() || doc.size() == 0)
    {
      console.error.println("[DISCORD] No messages available.");
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
          return false;
        }
        latestMessage = discordEntry;
        newMessageFlag = true;
        console[COLOR_MAGENTA].printf("[DISCORD] New Message received from [%s]: %s\n", sender.c_str(), latestMessage.c_str());
        console[COLOR_DEFAULT].print("");
        foundMessage = true;
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
    return false;    // No event to send
  }

  client.setTimeout(5000);
  client.setBufferSizes(8192 /* rx */, 1024 /* tx */);
  client.setDebugLevel(1);    // none = 0, error = 1, warn = 2, info = 3, dump = 4
  client.setClient(&base_client);
  client.setInsecure();                                      // Using insecure connection for testing
  String url = String("https://") + discordHost + apiUrl;    // Ensure `apiPath` points to the correct endpoint
  if(!http.begin(client, url))
  {
    console.error.printf("[DISCORD] Failed to initialize connection to: %s\n", url.c_str());
    return false;    // Server not available
  }

  // Prepare the event payload
  String eventString = String(myName) + "_" + String(Utils::getUnixTime()) + ":" + eventMessageToSend;
  String payload = "{\"content\":\"" + eventString + "\"}";

  // Add headers
  http.addHeader("Authorization", "Bot " + apiToken, true);    // Ensure the token is sent
  http.addHeader("User-Agent", "ESP32");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "keep-alive");

  // Send the POST request
  int httpCode = http.POST(payload);

  if(httpCode <= 0)
  {
    console.error.printf("[DISCORD] HTTP POST failed! Error code: %d, reason: %s\n", httpCode, http.errorToString(httpCode).c_str());
    http.end();
    client.stop();
    return false;
  }

  if(httpCode < 200 || httpCode >= 300)
  {
    console.warning.printf("[DISCORD] Unexpected HTTP response code: %d\n", httpCode);
    http.end();
    client.stop();
    return false;
  }

  // String response = http.getString();
  // Clear the event message since it has been sent successfully
  console.ok.printf("[DISCORD] Event sent: %s\n", eventMessageToSend.c_str());
  eventMessageToSend = "";
  outgoingEventFlag = false;

  http.end();    // Always end the HTTPClient session
  client.stop();

  return true;    // Event sent successfully
}


void Discord::updateTask(void* param)
{
  Discord* ref = (Discord*)param;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    if(Utils::getConnectionState() && ref->enabled)
    {
      ref->checkForOutgoingEvents();    // Check if there are events to send
      ref->checkForMessages();          // Check is server is available and if an update is available
    }
    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 * DISCORD_UPDATE_INTERVAL);
  }
  vTaskDelete(NULL);
}
