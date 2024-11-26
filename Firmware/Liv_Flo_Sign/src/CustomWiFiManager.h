/******************************************************************************
 * file    CustomWiFiManager.h
 *******************************************************************************
 * brief   Inherit from WiFiManager to add custom HTML elements
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-11-24
 *******************************************************************************
 * MIT License
 *
 * Copyright (c) 2024 Crelin - Florian Baumgartner
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

#ifndef CUSTOM_WIFI_MANAGER_H
#define CUSTOM_WIFI_MANAGER_H

#include <WiFiManager.h>


class CustomWiFiManagerParameter : public WiFiManagerParameter
{
 public:
  CustomWiFiManagerParameter(const char* customHTML) : WiFiManagerParameter(nullptr, nullptr, nullptr, 0, customHTML) {}

  // Override getCustomHTML to return the custom HTML
  virtual const char* getCustomHTML() const override { return _customHTML; }
};


class WiFiManagerCustom : public WiFiManager
{
 public:
  WiFiManagerCustom() : WiFiManager() {}
  WiFiManagerCustom(Print& consolePort) : WiFiManager(consolePort) {}
  void setConfigPortalSSID(String apName) { _apName = apName; }


  // Protected functions are now public, ungly I know, but it's the only way to access them
  void startWebPortal();
  boolean startConfigPortal();
  boolean startConfigPortal(char const* apName, char const* apPassword = NULL);
  void setupConfigPortal();
  void setupHTTPServer();
  void handleInfo();
  String getInfoData(String id);

 protected:
  bool _allowExit = false;         // Allow the user to exit the configuration portal
  bool _autoforcerescan = true;    // Automatically force a rescan if no networks are found
};


#endif