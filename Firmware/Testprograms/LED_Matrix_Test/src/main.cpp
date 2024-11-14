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

#define LED_PIN 7


const int httpsPort = 443;
const char* discordHost = "discord.com";
String currentMessage = "";    // The message to display
int textWidth = 0;             // Width of the text


WiFiClientSecure client;
Utils utils;
Preferences preferences;
Adafruit_NeoMatrix matrix =
  Adafruit_NeoMatrix(40, 7, LED_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

const uint8_t image_0[] = {
  0,   0,   0,   255, 159, 0,   253, 215, 99,  253, 234, 142, 253, 217, 104, 246, 158, 8,   0,   0,   0,  255, 148, 10, 255, 213, 85,  255, 255, 164,
  255, 255, 186, 255, 255, 167, 255, 217, 91,  246, 148, 16,  249, 161, 20,  233, 189, 59,  241, 224, 79, 255, 246, 88, 243, 226, 80,  232, 191, 61,
  248, 163, 25,  226, 149, 27,  201, 156, 47,  206, 162, 30,  243, 210, 42,  207, 164, 31,  199, 154, 47, 228, 153, 29, 111, 195, 223, 168, 169, 135,
  200, 171, 118, 208, 181, 127, 198, 167, 114, 169, 168, 131, 112, 196, 220, 47,  182, 245, 193, 137, 40, 198, 129, 22, 181, 120, 29,  198, 129, 21,
  196, 139, 39,  49,  178, 241, 0,   0,   0,   234, 112, 0,   223, 134, 9,   223, 137, 11,  222, 135, 10, 229, 110, 0,  0,   0,   0};
const uint8_t image_1[] = {
  0,   0,   0,   248, 159, 25,  253, 213, 112, 253, 231, 154, 253, 217, 117, 249, 160, 33, 0,   0,   0,  247, 135, 15, 253, 206, 87, 255, 255, 161,
  255, 253, 177, 255, 255, 163, 255, 210, 94,  248, 139, 18,  247, 150, 24,  255, 208, 68, 200, 161, 49, 236, 209, 70, 200, 160, 48, 255, 209, 69,
  248, 154, 27,  239, 136, 16,  255, 158, 33,  237, 174, 38,  247, 203, 47,  237, 175, 39, 254, 158, 33, 241, 138, 18, 234, 130, 18, 249, 145, 27,
  218, 148, 36,  219, 164, 43,  218, 149, 37,  250, 147, 28,  235, 133, 19,  227, 141, 30, 242, 178, 47, 241, 158, 30, 235, 149, 24, 243, 161, 31,
  244, 184, 50,  224, 137, 28,  225, 135, 26,  240, 179, 50,  224, 138, 19,  223, 120, 8,  223, 136, 19, 238, 175, 47, 219, 122, 20};
const uint8_t image_2[] = {
  213, 41, 35, 237, 121, 111, 241, 117, 106, 201, 10, 0,  236, 118, 109, 237, 129, 120, 210, 44, 31, 226, 73, 64, 255, 143, 134, 255, 122, 112,
  237, 67, 56, 255, 129, 120, 255, 151, 143, 229, 78, 67, 222, 53,  41,  255, 73,  60,  253, 67, 53, 255, 68, 54, 254, 69,  55,  255, 72,  59,
  235, 56, 43, 195, 20,  10,  240, 41,  28,  255, 58, 44, 255, 64,  49,  255, 60,  45,  251, 44, 30, 224, 24, 13, 85,  0,   0,   197, 17,  7,
  251, 41, 29, 255, 64,  50,  255, 51,  38,  217, 22, 12, 127, 0,   0,   0,   0,   0,   148, 0,  0,  203, 18, 9,  243, 32,  21,  215, 24,  15,
  139, 0,  0,  0,   0,   0,   0,   0,   0,   0,   0,  0,  162, 11,  0,   182, 11,  3,   153, 0,  0,  0,   0,  0,  0,   0,   0};


void checkDiscordForMessages();
void scrollText(void* pvParameters);
void drawRGB24Bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);

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
  matrix.setBrightness(10);
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
    // matrix.print(currentMessage.c_str());


    // Draw the 3 images (emojis) the are 7x7 pixels
    drawRGB24Bitmap(0, 0, image_0, 7, 7);
    drawRGB24Bitmap(12, 0, image_1, 7, 7);
    drawRGB24Bitmap(24, 0, image_2, 7, 7);



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

void drawRGB24Bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h)
{
  uint8_t r, g, b;
  for(int16_t j = 0; j < h; j++)
  {
    for(int16_t i = 0; i < w; i++)
    {
      r = pgm_read_byte(bitmap + 3 * (j * w + i));
      g = pgm_read_byte(bitmap + 3 * (j * w + i) + 1);
      b = pgm_read_byte(bitmap + 3 * (j * w + i) + 2);
      matrix.drawPixel(x + i, y + j, matrix.Color(r, g, b));
    }
  }
}
