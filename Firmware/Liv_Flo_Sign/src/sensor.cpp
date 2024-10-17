/******************************************************************************
 * file    sensor.cpp
 *******************************************************************************
 * brief   Handles the sensor data acquisition
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-16
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

#include "sensor.h"
#include "console.h"


bool Sensor::begin(void)
{
  if(!vcnl4020.begin(&Wire))
  {
    console.error.println("[SENSOR] VCNL4020 not found");
    return false;
  }
  console.ok.printf("[SENSOR] VCNL4020 found (Product ID & Revision: 0x%02X)\n", vcnl4020.getProdRevision());
  vcnl4020.enable(false, false, false);    // Turn off all features for configuration
  vcnl4020.setOnDemand(false, false);
  vcnl4020.setProxRate(PROX_RATE_250_PER_S);
  vcnl4020.setProxLEDmA(200);
  vcnl4020.setAmbientRate(AMBIENT_RATE_10_SPS);
  vcnl4020.setAmbientAveraging(AVG_8_SAMPLES);
  vcnl4020.setProxFrequency(PROX_FREQ_390_625_KHZ);
  //   vcnl4020.setInterruptConfig(true, false, false, false , INT_COUNT_1);
  vcnl4020.enable(true, true, true);
  //   vcnl4020.setOnDemand(false, false);

  xTaskCreate(updateTask, "sensor", 4096, this, 8, NULL);
  return true;
}

void Sensor::updateTask(void* pvParameter)
{
  Sensor* sensor = (Sensor*)pvParameter;

  while(true)
  {
    TickType_t task_last_tick = xTaskGetTickCount();
    if(sensor->vcnl4020.isProxReady())
    {
      sensor->proxValue = sensor->vcnl4020.readProximity();
      sensor->vcnl4020.clearInterrupts(true, false, false, false);    // Clear Interrupts
    }
    if(sensor->vcnl4020.isAmbientReady())
    {
      sensor->ambientValue = sensor->vcnl4020.readAmbient();
      sensor->vcnl4020.clearInterrupts(false, true, false, false);    // Clear Interrupts
    }

    static uint32_t t = 0;
    if(millis() - t > 1000)
    {
      t = millis();
      console.ok.printf("[SENSOR] Proximity: %d, Ambient: %d\n", sensor->proxValue, sensor->ambientValue);
    }

    vTaskDelayUntil(&task_last_tick, (const TickType_t)1000 / SENSOR_UPDATE_RATE);
  }
}
