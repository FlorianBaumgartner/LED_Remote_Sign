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

class ParameterSwitch : public CustomWiFiManagerParameter
{
 public:
  ParameterSwitch(const char* id, const char* label, bool defaultValue = 0) : CustomWiFiManagerParameter(id, label, nullptr, 1, nullptr, WFM_NO_LABEL)
  {
    setValue(defaultValue);
  }

  bool getValue() { return *WiFiManagerParameter::getValue() == '1'; }
  void setValue(bool value)
  {
    html = String(htmlTemplate);
    html.replace("{label}", _label);                      // Replace {label} with the label
    html.replace("{id}", _id);                            // Replace {id} with the unique ID
    html.replace("{checked}", value ? "checked" : "");    // Replace argument of actual switch widget
    html.replace("{checked_bool}", value ? "1" : "0");    // Replace argument of hidden input
    setCustomHTML(html.c_str());
  }

 private:
  String html;    // Store the HTML to ensure it stays in scope

  static constexpr const char* htmlTemplate =
    "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css'>"
    "<style>"
    "label { font-family: Verdana, sans-serif; font-size: 1em; margin: 10px 0; padding: 0; }"
    ".switch-wrapper { display: flex; align-items: center; justify-content: space-between; margin: 0px 0; padding: 0; }"
    ".switch { position: relative; display: inline-block; width: 50px; height: 24px; }"
    ".switch input { opacity: 0; width: 0; height: 0; }"
    ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: 0.4s; border-radius: "
    "24px; }"
    ".slider:before { position: absolute; content: ''; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: 0.4s; "
    "border-radius: 50%; }"
    "input:checked + .slider { background-color: #1fa3ec; }"
    "input:checked + .slider:before { transform: translateX(26px); }"
    "</style>"
    "<div class='switch-wrapper'>"
    "<label for='{id}'>{label}</label>"    // Label is left-aligned
    "<label class='switch'>"
    "<input type='checkbox' id='{id}' value='1' {checked}>"
    "<span class='slider'></span>"
    "</label>"                                                                     // Switch is right-aligned
    "<input type='hidden' id='{id}_hidden' name='{id}' value='{checked_bool}'>"    // Hidden input to pass the correct value
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


class ParameterColorPicker : public CustomWiFiManagerParameter
{
 public:
  ParameterColorPicker(const char* id, const char* label, uint32_t defaultValue = 0xFF0000)
      : CustomWiFiManagerParameter(id, label, nullptr, 7, nullptr, WFM_NO_LABEL)
  {
    setValue(defaultValue);
  }

  uint32_t getValue()
  {
    const char* val = WiFiManagerParameter::getValue();
    return strtoul(val + 1, nullptr, 16);    // Convert hex string (skip '#') to uint32_t
  }

  void setValue(uint32_t value)
  {
    char hexColor[8];                                        // 6 digits + null terminator
    snprintf(hexColor, sizeof(hexColor), "#%06X", value);    // Convert uint32_t to hex string
    html = String(htmlTemplate);
    html.replace("{label}", _label);      // Replace {label} with the label
    html.replace("{id}", _id);            // Replace {id} with the unique ID
    html.replace("{value}", hexColor);    // Replace {value} with the current color value
    setCustomHTML(html.c_str());
  }

 private:
  String html;    // Store the HTML to ensure it stays in scope

  static constexpr const char* htmlTemplate =
    "<style>"
    "label { font-family: Verdana, sans-serif; font-size: 1em; margin: 10px 0; padding: 0; }"
    ".color-picker-wrapper { display: flex; align-items: center; justify-content: space-between; margin: 0; padding: 0; }"
    ".color-picker-container { position: relative; overflow: hidden; width: 50px; height: 24px; "
    "border: 2px solid #ddd; border-radius: 5px; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); }"    // Rectangular container with rounded corners
    ".color-picker { position: absolute; top: -16px; right: -8px; width: 70px; height: 40px; "
    "border: none; cursor: pointer; }"    // Larger input to ensure smooth clicking
    "</style>"
    "<div class='color-picker-wrapper'>"
    "<label for='{id}'>{label}</label>"    // Label for the color picker
    "<div class='color-picker-container'>"
    "<input type='color' class='color-picker' id='{id}' name='{id}' value='{value}'>"    // Color picker input
    "</div>"
    "</div>";
};

class ParameterSelect : public CustomWiFiManagerParameter
{
 public:
  ParameterSelect(const char* id, const char* label, const char* const options[], size_t optionCount, size_t defaultIndex = 0)
      : CustomWiFiManagerParameter(id, label, nullptr, 1, nullptr, WFM_LABEL_BEFORE), _options(options), _optionCount(optionCount)
  {
    setValue(defaultIndex);
  }

  size_t getValue()
  {
    const char* val = WiFiManagerParameter::getValue();
    return atoi(val);    // Convert the stored value to an integer index
  }

  void setValue(size_t index)
  {
    if(index >= _optionCount)
    {
      index = 0;    // Ensure the index is within bounds
    }

    html = String(htmlTemplate);
    html.replace("{id}", _id);    // Replace {id} with the unique ID

    // Generate options HTML dynamically
    String optionsHTML;
    for(size_t i = 0; i < _optionCount; ++i)
    {
      optionsHTML += "<option value='" + String(i) + "'";
      if(i == index)
      {
        optionsHTML += " selected";
      }
      optionsHTML += ">" + String(_options[i]) + "</option>";
    }

    html.replace("{options}", optionsHTML);    // Replace {options} with the generated HTML
    setCustomHTML(html.c_str());
  }

 private:
  const char* const* _options;    // Pointer to an array of constant strings
  size_t _optionCount;            // Number of options
  String html;                    // Store the HTML to ensure it stays in scope

  static constexpr const char* htmlTemplate =
    "<style>"
    ".select-wrapper { display: flex; align-items: center; justify-content: center; margin: 0; padding: 0; width: 100%; }"
    ".select-container { position: relative; width: 100%; }"
    ".custom-select { font-family: Verdana, sans-serif; font-size: 1em; padding: 4px 8px; border: 1px solid #ddd; border-radius: 5px; "
    "box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); background-color: #f9f9f9; width: 100%; box-sizing: border-box; }"
    "</style>"
    "<div class='select-wrapper'>"
    "<div class='select-container'>"
    "<select class='custom-select' id='{id}' name='{id}'>"
    "{options}"    // Dynamic options insertion
    "</select>"
    "</div>"
    "</div>";
};


class ParameterSlider : public CustomWiFiManagerParameter
{
 public:
  ParameterSlider(const char* id, const char* label, int minValue, int maxValue, int defaultValue = 50, const char* postfix = "")
      : CustomWiFiManagerParameter(id, label, nullptr, 5, nullptr, WFM_LABEL_BEFORE),
        _minValue(minValue),
        _maxValue(maxValue),
        _defaultValue(defaultValue),
        _postfix(postfix)
  {
    setValue(defaultValue);
  }

  int getValue() { return atoi(WiFiManagerParameter::getValue()); }

  void setValue(int value)
  {
    value = constrain(value, _minValue, _maxValue);    // Ensure value is within bounds

    html = String(htmlTemplate);
    html.replace("{id}", _id);
    html.replace("{min}", String(_minValue));
    html.replace("{max}", String(_maxValue));
    html.replace("{value}", String(value));
    html.replace("{postfix}", _postfix);

    setCustomHTML(html.c_str());
  }

 private:
  int _minValue, _maxValue, _defaultValue;
  const char* _postfix;
  String html;

  static constexpr const char* htmlTemplate =
    "<style>"
    ".slidecontainer { width: 100%; margin: 0; }"
    ".basic-slider { -webkit-appearance: none; width: 100%; height: 6px; background: #ddd; border: none; border-radius: 3px; outline: none; padding: "
    "0; }"
    ".basic-slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 16px; height: 16px; background: #1fa3ec; "
    "border-radius: 50%; cursor: pointer; }"
    ".slider-value { font-family: Verdana, sans-serif; font-size: 0.85em; font-weight: 300; margin-top: 5px; text-align: left; color: #ddd; }"
    "</style>"
    "<div class='slidecontainer'>"
    "<input type='range' class='basic-slider' id='{id}' name='{id}' min='{min}' max='{max}' value='{value}' />"
    "<div class='slider-value'>Value: <span id='{id}_value'>{value}</span> {postfix}</div>"
    "<script>"
    "document.getElementById('{id}').addEventListener('input', function() {"
    "  document.getElementById('{id}_value').innerText = this.value;"
    "});"
    "</script>"
    "</div>";
};


#endif
