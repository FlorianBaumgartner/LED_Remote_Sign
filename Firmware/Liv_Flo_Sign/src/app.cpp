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
#include "device.h"
#include "utils.h"

bool App::begin()
{
  disp.setTextColor(Utils::getTextColor());
  sign.setBootColor(Utils::getTextColor());
  sign.setNightLightColor(Utils::getNightLightColor());

  discord.begin();
  githubOTA.begin();
  sensor.begin();
  disp.begin(LED_UPDATE_RATE);
  sign.begin(LED_UPDATE_RATE);

  static String bootMessage = "BOOT " + Device::getDeviceName() + ": v" + String(FIRMWARE_VERSION) + " (" + utils.getResetReason() + ")";
  discord.sendEvent(bootMessage.c_str());

  xTaskCreate(appTask, "app_task", 4096, this, 17, NULL);         // Stack Watermark: 2476
  xTaskCreate(ledTask, "led_sign_task", 4096, this, 20, NULL);    // Stack Watermark: 2492
  return true;
}

void App::appTask(void* pvParameter)
{
  App* app = (App*)pvParameter;
  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    if(!app->booting)
    {
      if(app->utils.getConnectionState())
      {
        if(app->githubOTA.updateAvailable())
        {
          console.log.println("[APP] Update available, shut down services");
          app->discord.enable(false);
          app->sign.enable(false);
          app->disp.setUpdatePercentage(-1);    // Show update message
          app->disp.setState(DisplayMatrix::UPDATING);
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
          app->disp.setUpdatePercentage(app->githubOTA.getProgress());
        }
        else if(app->showIpAddressTimer.expired())    // Show IP address instead of message while timer is running
        {
          app->discord.enable(true);
          app->disp.setState(DisplayMatrix::IDLE);
          app->disp.setMessage(app->discord.getLatestMessage());
          app->sign.enable(true);
        }
      }
      else
      {
        app->sign.enable(false);
        app->disp.setState(Utils::isClientConnectedToPortal() ? DisplayMatrix::PORTAL_ACTIVE : DisplayMatrix::DISCONNECTED);
      }
    }

    // Check for boot status
    static bool once = true;
    if(!app->sign.getBootStatus() && once)
    {
      once = false;
      app->booting = false;
    }

    // Check for activation triggers
    bool motionTrigger = false;            // Trigger is set only once and gets cleared otherswise
    bool eventTrigger = false;             // Trigger is set only once and gets cleared otherswise
    static bool newMessageFlag = false;    // Flag stays active until the user triggers motion event
    static bool initialMessageReceived = false;
    if(app->sensor.getProxEvent())
    {
      console.log.println("[APP] Proximity Event");
      motionTrigger = true;
      if(!newMessageFlag)    // When the user wakes up the sign to see the message, don't send event
      {
        app->discord.sendEvent("PROXIMITY");
      }
      if(!Utils::getMotionActivated())    // Only trigger animation when not in motion activation mode
      {
        eventTrigger = true;
      }
      newMessageFlag = false;    // Reset new message flag when the user activates the sign
    }
    if(app->discord.newMessageAvailable())
    {
      if(initialMessageReceived)    // Don't trigger event on first message
      {
        newMessageFlag = true;
      }
      else
      {
        motionTrigger = true;    // When initial message is received, trigger motion event to display message
      }
      initialMessageReceived = true;
    }
    if(app->discord.newEventAvailable())
    {
      String event;
      if(app->discord.getLatestEvent(event))
      {
        if(event == "PROXIMITY")
        {
          eventTrigger = true;    // When event is received, only trigger event animation (has no effect when motion activation is enabled)
        }
      }
    }
    app->sign.setEvent(eventTrigger);
    app->sign.setNewMessage(newMessageFlag);
    app->sign.setMotionEvent(motionTrigger);    // Trigger to activate the sign
    app->sign.setMotionActivation(Utils::getMotionActivated());
    app->sign.setMotionEventTime(Utils::getMotionActivationTime());
    app->disp.setMotionEvent(motionTrigger);
    app->disp.setMotionActivation(Utils::getMotionActivated());
    app->disp.setMotionEventTime(Utils::getMotionActivationTime());

    // Set brightness and night mode
    uint8_t brightness = map(app->sensor.getAmbientBrightness(), 0, 255, 0, app->disp.MAX_BRIGHTNESS);
    if(brightness < NIGHT_LIGHT_MODE_MIN)
    {
      bool nightMode = Utils::getNightLight();
      app->sign.setNightMode(nightMode);    // Enable night mode if user has set it
      app->sign.setBrightness(nightMode ? NIGHT_LIGHT_MODE_MIN : 0);
      app->disp.setBrightness(0);    // Turn off display for low brightness environments
    }
    else
    {
      app->sign.setNightMode(false);
      app->sign.setBrightness(brightness);
      app->disp.setBrightness(brightness);    // Turn off display for very low brightness (colors get distorted)
    }

    // Allways apply current settings to modules
    app->disp.setTextColor(Utils::getTextColor());
    app->sign.setNightLightColor(Utils::getNightLightColor());
    app->sign.setAnimationType(Utils::getAnimationType());
    app->sign.setAnimationPrimaryColor(Utils::getAnimationPrimaryColor());
    app->sign.setAnimationSecondaryColor(Utils::getAnimationSecondaryColor());

    // TODO: Remove after debugging
    static uint32_t tUpdateCheck = 0;
    static const uint32_t tUpdateCheckInterval = 8 * 60 * 60 * 1000;    // 8 hours
    if(millis() - tUpdateCheck > tUpdateCheckInterval)
    {
      tUpdateCheck = millis();
      console.log.println("[APP] Sending update report");
      String message = String("UPDATE_CHECK: ") + app->githubOTA.getFailedUpdateChecks() + " (" + millis() / (1000 * 60) + "min)";
      app->discord.sendEvent(message.c_str());
    }

    app->utils.resetWatchdog();
    vTaskDelayUntil(&task_last_tick, pdMS_TO_TICKS(1000 / APP_UPDATE_RATE));
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
