/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Ha Thach (tinyusb.org) for Adafruit Industries
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef ADAFRUIT_USBH_CDC_H_
#define ADAFRUIT_USBH_CDC_H_

#include "Stream.h"

class Adafruit_USBH_CDC : public Stream {
public:
  Adafruit_USBH_CDC(void);

  // Init/Bind to an specific cdc interface
  void begin(uint8_t idx = 0);

  // unbind cdc interface
  void end(void);

  // Get index of cdc interface
  uint8_t getIndex(void) { return _idx; }

  // If cdc is mounted
  bool mounted(void);
  operator bool() { return mounted(); }

  // if cdc's DTR is asserted
  bool connected(void);

  //------------- Stream API -------------//
  virtual int available(void);
  virtual int peek(void);

  virtual int read(void);
  size_t read(uint8_t *buffer, size_t size);

  virtual void flush(void);
  virtual size_t write(uint8_t ch);

  virtual size_t write(const uint8_t *buffer, size_t size);
  size_t write(const char *buffer, size_t size) {
    return write((const uint8_t *)buffer, size);
  }

  virtual int availableForWrite(void);
  using Print::write; // pull in write(str) from Print

private:
  uint8_t _idx; // TinyUSB CDC Interface Index
};

#endif
