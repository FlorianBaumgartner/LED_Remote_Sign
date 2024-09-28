/******************************************************************************
* file    systemParser.cpp
*******************************************************************************
* brief   JSON-File Parser for system configuration
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

#include "systemParser.hpp"
#include "console.hpp"
#include "utils.hpp"
#include "USB.h"

SystemParser::SystemParser(void) {}

/**
 * @brief Load a system configuration file
 *
 * @param path is the path of the file
 * @return true on success
 * @return false on error
 */
bool SystemParser::loadFile(const char* path) {
  filePath = path;
  File file = fatfs.open(filePath);

  if(!file)
  {
    console.error.println("[PARSER] Open file failed");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if(error)
  {
    file.close();
    console.error.printf("[PARSER] Failed to read file, using default configuration: %d\n", error);
    return false;
  }

  file.close();
  return true;
}

/**
 * @brief Get the USB Vender Identifier
 * 
 * @param vid uint16_t reference to the USB VID
 * @return true on success
 * @return false on error
 */
bool SystemParser::getUsbVid(uint16_t& vid)
{
  if(doc.containsKey("usb_vid"))
  {
    vid = (uint16_t)strtol(doc["usb_vid"].as<const char*>(), NULL, 0);
    return true;
  }
  return false;
}

/**
 * @brief Get the USB Product Identifier
 * 
 * @param pid uint16_t reference to the USB PID
 * @return true on success
 * @return false on error
 */
bool SystemParser::getUsbPid(uint16_t& pid)
{
  if(doc.containsKey("usb_pid"))
  {
    pid = (uint16_t)strtol(doc["usb_pid"].as<const char*>(), NULL, 0);
    return true;
  }
  return false;
}

/**
 * @brief Get the USB Serial Number
 * 
 * @param usbSerial const char* with the serial number
 * @param size max number of characters
 * @return true on success
 * @return false on error
 */
bool SystemParser::getUsbSerial(char* usbSerial, size_t size)
{
  if(doc.containsKey("usb_serial"))
  {
    strncpy(usbSerial, doc["usb_serial"].as<const char*>(), size);
    return true;
  }
  return false;
}

/**
 * @brief Get the global LED brightness
 * 
 * @param value uint8_t reference to the brightness value
 * @return true on success
 * @return false on error
 */
bool SystemParser::getLedBrightness(uint8_t& value)
{
  if(doc.containsKey("led_brightness"))
  {
    value = (uint8_t)doc["led_brightness"].as<unsigned int>();
    return true;
  }
  return false; 
}

/**
 * @brief Get the total LED pixel count
 * 
 * @param value uint8_t reference to the pixel count value
 * @return true on success
 * @return false on error
 */
bool SystemParser::getLedCount(uint16_t& value)
{
  if(doc.containsKey("led_count"))
  {
    value = (uint16_t)doc["led_count"].as<unsigned int>();
    return true;
  }
  return false; 
}

/**
 * @brief Get the LED framerate in FPS (Hz)
 * 
 * @param value uint8_t reference to the framerate value
 * @return true on success
 * @return false on error
 */
bool SystemParser::getLedFramerate(float& value)
{
  if(doc.containsKey("led_framerate"))
  {
    value = doc["led_framerate"].as<float>();
    return true;
  }
  return false; 
}


/**
 * @brief Get the global LED brightness
 * 
 * @return true if LEDs are RGBW
 * @return false if not specified
 */
bool SystemParser::isLedRgbw(void)
{
  if(doc.containsKey("led_rgbw"))
  {
    return doc["led_rgbw"].as<bool>();
  }
  return false; 
}

/**
 * @brief Save the current loaded system config as a file
 *
 * @param path to location
 * @return true on success
 * @return false on error
 */
bool SystemParser::saveFile(const char* path) {
  if(path != NULL)
  {
    filePath = path;
  }
  if(fatfs.exists(filePath))
  {
    if(!fatfs.remove(filePath)) 
    {
      console.error.println("[PARSER] Could not remove file");
      return false;
    }
  }
  File file = fatfs.open(filePath, FILE_WRITE);
  if(!file)
  {
    console.error.println("[PARSER] Open file failed");
    return false;
  }
  if(serializeJson(doc, file) == 0)
  {
    file.close();
    console.error.println("[PARSER] Failed to write to file");
    return false;
  }
  file.close();
  return true;
}
