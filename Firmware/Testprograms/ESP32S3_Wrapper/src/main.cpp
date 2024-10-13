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

#include <Arduino.h>
#include "../../../Liv_Flo_Sign/src/Discord.h"
#include "../../../Liv_Flo_Sign/src/GithubOTA.h"
#include "../../../Liv_Flo_Sign/src/displayMatrix.h"
#include "../../../Liv_Flo_Sign/src/utils.h"
#include "console.h"
#include "system.h"


#define BTN_PIN      0
#define LED_PWR_PIN  6
#define LED_RGB_PIN  18
#define LED_MATRIX_H 8
#define LED_MATRIX_W 40


Utils utils;
System sys;
Discord discord;
GithubOTA githubOTA;
DisplayMatrix disp(LED_RGB_PIN, LED_MATRIX_H, LED_MATRIX_W);

static void updateTask(void* param);


void setup()
{
  console.begin();
  if(!sys.begin(0, "DRIVE"))
  {
    console.error.println("[MAIN] Could not initialize utilities");
  }

  pinMode(LED_PWR_PIN, OUTPUT);
  digitalWrite(LED_PWR_PIN, HIGH);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
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
