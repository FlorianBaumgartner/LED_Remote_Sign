/******************************************************************************
 * file    main.cpp
 *******************************************************************************
 * brief   Main Program
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2022-08-02
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

#include <Arduino.h>
#include "Discord.h"
#include "GithubOTA.h"
#include "console.h"
#include "utils.h"
#include "displayMatrix.h"


#define LED_RGB_PIN  8
#define BTN_PIN      9
#define LED_PIN      10

#define TOTAL_LEDS   480
#define LED_MATRIX_H 5
#define LED_MATRIX_W TOTAL_LEDS / LED_MATRIX_H


Utils utils;
Discord discord;
GithubOTA githubOTA;
DisplayMatrix disp(LED_RGB_PIN, LED_MATRIX_H, LED_MATRIX_W);


// void scrollTextNonBlocking(const char* text, int speed);
static void updateTask(void* param);

void setup()
{
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  
  console.begin();
  utils.begin();
  discord.begin();
  githubOTA.begin(REPO_URL);

  disp.begin();

  xTaskCreate(updateTask, "main_task", 4096, NULL, 12, NULL);
}

void loop()
{
  vTaskDelay(100);
}

// void scrollTextNonBlocking(const char* text, int speed)
// {
//   static int x = 5;
//   const int len = strlen(text) * 6;

//   disp.fillScreen(0);
//   disp.setCursor(x, 4);
//   disp.print(text);
//   static uint32_t tShift = 0;
//   if(millis() - tShift >= speed)
//   {
//     tShift = millis();
//     if(--x < -len)
//     {
//       x = 5;
//     }
//   }
// }

static void updateTask(void* param)
{
  while(true)
  {
    static bool btnOld = false, btnNew = false;
    btnOld = btnNew;
    btnNew = !digitalRead(BTN_PIN);
    if(!btnOld && btnNew)
    {
      static uint16_t counter = 0;
      char buffer[60];
      sprintf(buffer, "Button Pressed: %d", counter++);
      console.log.printf("%s\n", buffer);
      discord.sendEvent(buffer);
    }

    if(githubOTA.updateAvailable())
    {
      githubOTA.startUpdate();
    }
    if(githubOTA.updateInProgress())
    {
      disp.setState(DisplayMatrix::UPDATING);
      disp.setUpdatePercentage(githubOTA.getProgress());
    }
    else
    {
      disp.setState(DisplayMatrix::IDLE);
      disp.setMessage(discord.getLatestMessage());
    }

    vTaskDelay(50);
    utils.resetWatchdog();
  }
}