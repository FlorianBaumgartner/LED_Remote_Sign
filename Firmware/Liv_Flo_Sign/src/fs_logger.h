/******************************************************************************
 * file    fs_logger.h
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

#ifndef FS_LOGGER_H
#define FS_LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

class FSLogger
{
 public:
  bool begin();
  void writeToFS(const uint8_t* buffer, size_t size);
  void printStoredLog();
  void clearLog();

 private:
  const char* logfilePath = "/log.txt";
  const size_t maxLogSize = 40 * 1024;
  File logfile;

  static void LogPrintTask(void* parameter);
  static SemaphoreHandle_t logDoneSemaphore;
};


#endif
