/******************************************************************************
 * file    app.cpp
 *******************************************************************************
 * brief   Main Application
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-17
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

#include "app.h"
#include "console.h"

bool App::begin()
{
  discord.begin();
  githubOTA.begin(REPO_URL);
  sensor.begin();
  disp.begin();
  sign.begin();

  xTaskCreate(appTask, "app_task", 4096, this, 17, NULL);
  xTaskCreate(ledTask, "led_sign_task", 4096, this, 20, NULL);
  return true;
}

void App::appTask(void* pvParameter)
{
  App* app = (App*)pvParameter;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();

    // static int t = 0;
    // if(millis() - t > 500)
    // {
    //   t = millis();
    //   console.log.printf("Free Heap: %d, Max Heap: %d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    // }

    if(!app->booting)
    {
      if(app->utils.getConnectionState())
      {
        if(app->githubOTA.updateAvailable() && !app->githubOTA.updateInProgress())
        {
          console.log.println("[APP] Update available, shut down services");
          app->discord.enable(false);
          app->sign.enable(false);
          app->sensor.enable(false);
          app->githubOTA.startUpdate();
        }
        if(app->githubOTA.updateAborted())
        {
          console.error.println("[APP] Update aborted");
          app->discord.enable(true);
          app->sign.enable(true);
          app->sensor.enable(true);
          app->disp.setState(DisplayMatrix::IDLE);
          app->disp.setMessage("Update aborted");
        }

        if(app->utils.getButtonShortPressEvent())
        {
          app->showIpAddressTimer.start(IP_ADDRESS_SHOW_TIME * 1000);
          IPAddress ipAddr = WiFi.localIP();
          static char ipStr[16];
          sprintf(ipStr, "%d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
          app->disp.setIpAdress(String(ipStr));
          app->disp.setState(DisplayMatrix::SHOW_IP);
        }
        if(app->utils.getButtonLongPressEvent())
        {
          console.log.println("[APP] Long press event, reset settings");
          app->utils.resetSettings();
        }

        if(app->githubOTA.updateInProgress())
        {
          app->disp.setState(DisplayMatrix::UPDATING);
          app->disp.setUpdatePercentage(app->githubOTA.getProgress());
        }
        else if(app->showIpAddressTimer.expired())    // Show IP address instead of message while timer is running
        {
          app->disp.setState(DisplayMatrix::IDLE);
          app->disp.setMessage(app->discord.getLatestMessage());
          app->sign.enable(true);
        }
      }
      else
      {
        app->disp.setState(DisplayMatrix::DISCONNECTED);
        app->sign.enable(false);
      }
    }

    if(app->sensor.getProxEvent())
    {
      console.log.println("[APP] Proximity Event");
      app->discord.sendEvent("PROXIMITY");
    }

    if(app->discord.newEventAvailable())
    {
      String event;
      if(app->discord.getLatestEvent(event))
      {
        app->sign.setEvent(event == "PROXIMITY");
      }
    }
    else
    {
      app->sign.setEvent(false);
    }

    static bool once = true;
    if(!app->sign.getBootStatus() && once)
    {
      once = false;
      app->booting = false;
    }

    uint8_t brightness = map(app->sensor.getAmbientBrightness(), 0, 255, 0, app->disp.MAX_BRIGHTNESS);
    brightness = brightness < 3 ? 0 : brightness;
    app->disp.setBrightness(brightness);    // Turn off display for very low brightness (colors get distorted)
    app->sign.setBrightness(brightness);

    app->utils.resetWatchdog();
    vTaskDelayUntil(&task_last_tick, pdMS_TO_TICKS(1000 / app->APP_UPDATE_RATE));
  }
}


void App::ledTask(void* pvParameter)
{
  App* app = (App*)pvParameter;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    app->sign.updateTask();
    app->disp.updateTask();
    vTaskDelayUntil(&task_last_tick, pdMS_TO_TICKS(1000 / app->LED_UPDATE_RATE));
  }
}
