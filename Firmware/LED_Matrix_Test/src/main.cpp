/******************************************************************************
 * file    main.cpp
 *******************************************************************************
 * brief   Main Program
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2023-02-14
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

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "freertos/FreeRTOS.h"

#include "console.hpp"
#include "credentials.h"
#include "utils.hpp"

#define LED_PIN 18


const int httpsPort = 443;
const char* discordHost = "discord.com";
String currentMessage = "";    // The message to display
int textWidth = 0;             // Width of the text


WiFiClientSecure client;
Utils utils;
Preferences preferences;
Adafruit_NeoMatrix matrix =
  Adafruit_NeoMatrix(40, 8, LED_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);


void checkDiscordForMessages();
void scrollText(void* pvParameters);

void setup()
{
  console.begin();
  if(!utils.begin(0, "DRIVE"))
  {
    console.error.println("[MAIN] Could not initialize utilities");
  }

  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(4);
  matrix.setRotation(2);
  matrix.fillScreen(0);
  matrix.setTextColor(matrix.Color(255, 0, 255));
  matrix.setTextSize(1);
  matrix.setCursor(0, 0);
  matrix.print("Booting");
  matrix.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    console.log.println("Connecting to WiFi...");
  }

  console.log.println(WiFi.localIP());
  xTaskCreate(scrollText, "task_led", 4096, nullptr, 1, NULL);
}

void loop()
{
  static uint32_t lastDiscordCheck = 0;
  if(millis() - lastDiscordCheck > 1000)
  {
    lastDiscordCheck = millis();
    checkDiscordForMessages();
  }

  delay(10);
}

void checkDiscordForMessages()
{
  client.setInsecure();    // Using insecure connection for testing
  String apiUrlWithLimit = String(apiUrl) + "&limit=20";
  String lastMessageId = "";
  bool foundMessage = false;

  while(!foundMessage)
  {
    // If there's a last message ID, use it to fetch older messages
    if(lastMessageId.length() > 0)
    {
      apiUrlWithLimit += "&before=" + lastMessageId;
    }

    if(!client.connect(discordHost, httpsPort))
    {
      console.error.println("Connection to Discord failed!");
      return;
    }

    client.print(String("GET ") + apiUrlWithLimit + " HTTP/1.1\r\n" + "Host: " + discordHost + "\r\n" + "Authorization: Bot " + botToken + "\r\n" +
                 "User-Agent: ESP32\r\n" + "Connection: close\r\n\r\n");

    while(client.connected())
    {
      String line = client.readStringUntil('\n');
      if(line == "\r")
      {
        break;
      }
    }

    String payload;
    while(client.available())
    {
      payload += client.readString();
    }

    // Trim to get valid JSON content
    int start = payload.indexOf('[');
    payload = payload.substring(start);
    int end = payload.lastIndexOf(']');
    payload = payload.substring(0, end + 1);

    static JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if(error)
    {
      console.error.println("Failed to parse JSON: ");
      console.error.println(error.c_str());
      client.stop();
      return;
    }

    if(!doc.isNull() && doc.size() > 0)
    {
      for(int i = 0; i < doc.size(); i++)
      {
        String message = doc[i]["content"].as<String>();
        console.log.println("Received message: " + message);

        // Check for the last message that starts with "PHONE_LIV:"
        if(message.startsWith("PHONE_LIV:"))
        {
          currentMessage = message.substring(strlen("PHONE_LIV:"));
          textWidth = matrix.width() + 6 * strlen(currentMessage.c_str());    // Calculate total text width
          foundMessage = true;                                                // Break out of loop when we find the message
          break;
        }
        // Track the ID of the last message in this chunk for pagination
        lastMessageId = doc[i]["id"].as<String>();
      }
    }

    client.stop();

    // If we processed all messages and didn't find the desired message, load the next chunk
    if(lastMessageId == "")
    {
      console.error.println("No more messages to check.");
      break;
    }
  }

  if(!foundMessage)
  {
    console.log.println("No message containing 'PHONE_LIV:' found.");
  }
}

void scrollText(void* pvParameters)
{
  static int scrollPosition = 0;    // Position for scrolling text

  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    matrix.fillScreen(0);                              // Clear the display
    matrix.setTextColor(matrix.Color(255, 0, 255));    // Set text color
    matrix.setTextSize(1);                             // Set text size
    matrix.setCursor(scrollPosition, 0);               // Set cursor position

    // Draw the current message
    matrix.print(currentMessage.c_str());
    matrix.show();

    // Update the scroll position
    scrollPosition--;

    // If the text has completely scrolled off screen, reset position
    if(scrollPosition < -textWidth)
    {
      scrollPosition = matrix.width();
    }

    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 / 30);
  }
}
