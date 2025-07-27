/******************************************************************************
 * file    fs_logger.cpp
 *******************************************************************************
 * brief   File System Logger
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2025-07-26
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

#include "fs_logger.h"
#include <console.h>

SemaphoreHandle_t FSLogger::logDoneSemaphore = nullptr;

bool FSLogger::begin()
{
  if(!SPIFFS.begin(true))
  {
    console.error.println("[FSLOGGER] Failed to mount SPIFFS");
    return false;
  }

  logfile = SPIFFS.open(logfilePath, FILE_APPEND);
  if(!logfile)
  {
    console.error.println("[FSLOGGER] Failed to open log file");
    return false;
  }

  console.ok.println("[FSLOGGER] SPIFFS mounted successfully");
  return true;
}

void FSLogger::writeToFS(const uint8_t* buffer, size_t size)
{
  if(!logfile)
    return;

  if(logfile.size() + size > maxLogSize)
  {
    logfile.close();
    SPIFFS.remove(logfilePath);
    logfile = SPIFFS.open(logfilePath, FILE_WRITE);
    if(!logfile)
      return;
  }

  logfile.write(buffer, size);
  logfile.flush();
}

void FSLogger::clearLog()
{
  logfile.close();
  SPIFFS.remove(logfilePath);
  logfile = SPIFFS.open(logfilePath, FILE_WRITE);
}

void FSLogger::printStoredLog()
{
  if(!SPIFFS.exists(logfilePath))
    return;

  if(logDoneSemaphore == nullptr)
    logDoneSemaphore = xSemaphoreCreateBinary();

  if(logDoneSemaphore == nullptr)
  {
    console.error.println("[FSLOGGER] Failed to create log semaphore");
    return;
  }

  BaseType_t ok = xTaskCreatePinnedToCore(FSLogger::LogPrintTask, "LogPrintTask", 4096, this, 1, nullptr, 0);
  if(ok != pdPASS)
  {
    console.error.println("[FSLOGGER] Failed to create print task");
    return;
  }

  // Wait for log task to finish
  xSemaphoreTake(logDoneSemaphore, portMAX_DELAY);
}

void FSLogger::LogPrintTask(void* parameter)
{
  FSLogger* logger = static_cast<FSLogger*>(parameter);
  File f = SPIFFS.open(logger->logfilePath, FILE_READ);
  if(!f)
  {
    xSemaphoreGive(logger->logDoneSemaphore);
    vTaskDelete(nullptr);
  }

  Serial.println("\n------------------ Boot Log Start ------------------");

  constexpr size_t bufferSize = 256;
  uint8_t buffer[bufferSize];

  while(f.available())
  {
    size_t len = f.read(buffer, bufferSize);
    Serial.write(buffer, len);
  }

  Serial.println("\n------------------- Boot Log End -------------------");

  f.close();
  xSemaphoreGive(logger->logDoneSemaphore);
  vTaskDelete(nullptr);
}
