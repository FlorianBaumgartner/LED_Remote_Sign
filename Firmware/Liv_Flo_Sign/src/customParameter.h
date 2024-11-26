/******************************************************************************
 * file    customParameter.h
 *******************************************************************************
 * brief   HTML/CSS/JS custom parameters
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-11-25
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

#ifndef CUSTOM_PARAMETER_H
#define CUSTOM_PARAMETER_H

#include <Arduino.h>
#include <CustomWiFiManager.h>

class ParameterSwitch
{
 public:
  ParameterSwitch(const char* id, const char* label, bool defaultValue) : param(id, nullptr, defaultValue ? "1" : "0", 10, nullptr)
  {
    html = String(htmlPart1) + String(label) + String(htmlPart2);
    html.replace("{id}", id);  // Replace {id} with the unique ID
    // html.replace("{checked}", defaultValue ? "1" : "0");  // Add checked attribute if default value is true
    param.setCustomHTML(html.c_str());  // Assign the final HTML to the parameter
  }

  CustomWiFiManagerParameter param;

 private:
  String html;    // Store the HTML to ensure it stays in scope

  static constexpr const char* htmlPart1 =
    "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css'>"
    "<style>"
    "label { font-family: Verdana, sans-serif; font-size: 1em; margin: 5px 0; }"
    ".switch-wrapper { display: flex; align-items: center; justify-content: space-between; margin: 10px 0; }"
    ".switch { position: relative; display: inline-block; width: 50px; height: 24px; }"
    ".switch input { opacity: 0; width: 0; height: 0; }"
    ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: 0.4s; border-radius: 24px; }"
    ".slider:before { position: absolute; content: ''; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: 0.4s; border-radius: 50%; }"
    "input:checked + .slider { background-color: #007bff; }"
    "input:checked + .slider:before { transform: translateX(26px); }"
    "</style>"
    "<div class='switch-wrapper'>"
    "<label for='{id}'>";

  static constexpr const char* htmlPart2 =
    "</label>"    // Label is left-aligned
    "<label class='switch'>"
    "<input type='checkbox' id='{id}' value='1' {checked}>"
    "<span class='slider'></span>"
    "</label>"    // Switch is right-aligned
    "<input type='hidden' id='{id}_hidden' name='{id}' value='{checked}'>"    // Hidden input to pass the correct value
    "</div>"
    "<script>"
    "document.addEventListener('DOMContentLoaded', function() {"
    "  const checkbox = document.getElementById('{id}');"
    "  const hiddenInput = document.getElementById('{id}_hidden');"
    "  checkbox.addEventListener('change', function() {"
    "    hiddenInput.value = this.checked ? '1' : '0';"    // Dynamically update hidden input based on checkbox state
    "  });"
    "});"
    "</script>";
};



#endif
