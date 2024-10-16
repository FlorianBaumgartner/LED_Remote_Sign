/******************************************************************************
 * file    main.cpp
 *******************************************************************************
 * brief   Main Program
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

#include <Arduino.h>
#include "Discord.h"
#include "GithubOTA.h"
#include "console.h"
#include "displayMatrix.h"
#include "utils.h"


#include "Adafruit_VCNL4020.h"

Adafruit_VCNL4020 vcnl4020;


#define LED_MATRIX_PIN   7
#define LED_SIGNAL_PIN   8
#define BTN_PIN          0

#define LED_SIGN_COUNT   268
#define LED_MATRIX_H     7
#define LED_MATRIX_W     40
#define LED_MATRIX_COUNT (LED_MATRIX_H * LED_MATRIX_W)


Utils utils;
Discord discord;
GithubOTA githubOTA;
DisplayMatrix disp(LED_MATRIX_PIN, LED_MATRIX_H, LED_MATRIX_W);


Adafruit_NeoPixel ledSign(LED_SIGN_COUNT, LED_SIGNAL_PIN, NEO_GRB + NEO_KHZ800);


// void scrollTextNonBlocking(const char* text, int speed);
static void updateTask(void* param);

void setup()
{
  pinMode(BTN_PIN, INPUT_PULLUP);

  console.begin();
  utils.begin();
  discord.begin();
  githubOTA.begin(REPO_URL);
  disp.begin();

  ledSign.begin();
  ledSign.setBrightness(10);
  // Set color to white
  for(int i = 0; i < LED_SIGN_COUNT; i++)
  {
    ledSign.setPixelColor(i, 255, 0, 200);
  }
  // ledSign.show();

  xTaskCreate(updateTask, "main_task", 4096, NULL, 12, NULL);


  console.log.println("Adafruit VCNL4020 Library Test Sketch");

  // Initialize sensor
  if(!vcnl4020.begin(&Wire))
  {
    console.log.println("Failed to initialize VCNL4020!");
    while(1)
      ;
  }
  console.log.println("VCNL4020 initialized.");

  // Verify product ID & revision
  console.log.print("VCNL4020 Product ID & Revision (should be 0x21): ");
  console.log.println(vcnl4020.getProdRevision(), HEX);

  // To set up all the configuration, first disable everything
  vcnl4020.enable(false /* ALS Enable */, false /* Proximity Enable */, false /* Self-Timed Enable */);
  vcnl4020.setOnDemand(false /* ALS on demand read */, false /* Prox on demand read */);

  // Try different proximity rates, faster measurements use more power of course!
  // Can also try: PROX_RATE_1_95_PER_S, PROX_RATE_3_9_PER_S, PROX_RATE_7_8_PER_S,
  // PROX_RATE_16_6_PER_S, PROX_RATE_31_2_PER_S, PROX_RATE_62_5_PER_S,
  // PROX_RATE_125_PER_S, PROX_RATE_250_PER_S
  vcnl4020.setProxRate(PROX_RATE_250_PER_S);
  console.log.print("Proximity Rate: ");
  switch(vcnl4020.getProxRate())
  {
    case PROX_RATE_1_95_PER_S:
      console.log.println("1.95 measurements/s");
      break;
    case PROX_RATE_3_9_PER_S:
      console.log.println("3.9 measurements/s");
      break;
    case PROX_RATE_7_8_PER_S:
      console.log.println("7.8 measurements/s");
      break;
    case PROX_RATE_16_6_PER_S:
      console.log.println("16.6 measurements/s");
      break;
    case PROX_RATE_31_2_PER_S:
      console.log.println("31.2 measurements/s");
      break;
    case PROX_RATE_62_5_PER_S:
      console.log.println("62.5 measurements/s");
      break;
    case PROX_RATE_125_PER_S:
      console.log.println("125 measurements/s");
      break;
    case PROX_RATE_250_PER_S:
      console.log.println("250 measurements/s");
      break;
  }

  // Test LED Current (Valid range: 0-200 mA), higher currents require
  // more power but will let you detect farther.
  vcnl4020.setProxLEDmA(200);
  console.log.print("Proximity LED Current (mA): ");
  console.log.println(vcnl4020.getProxLEDmA());


  // Test Ambient Rate
  // Can also try: AMBIENT_RATE_1_SPS, AMBIENT_RATE_2_SPS, AMBIENT_RATE_3_SPS,
  // AMBIENT_RATE_4_SPS, AMBIENT_RATE_5_SPS, AMBIENT_RATE_6_SPS, AMBIENT_RATE_8_SPS,
  // AMBIENT_RATE_10_SPS
  vcnl4020.setAmbientRate(AMBIENT_RATE_10_SPS);
  console.log.print("Ambient Rate: ");
  switch(vcnl4020.getAmbientRate())
  {
    case AMBIENT_RATE_1_SPS:
      console.log.println("1 samples/s");
      break;
    case AMBIENT_RATE_2_SPS:
      console.log.println("2 samples/s");
      break;
    case AMBIENT_RATE_3_SPS:
      console.log.println("3 samples/s");
      break;
    case AMBIENT_RATE_4_SPS:
      console.log.println("4 samples/s");
      break;
    case AMBIENT_RATE_5_SPS:
      console.log.println("5 samples/s");
      break;
    case AMBIENT_RATE_6_SPS:
      console.log.println("6 samples/s");
      break;
    case AMBIENT_RATE_8_SPS:
      console.log.println("8 samples/s");
      break;
    case AMBIENT_RATE_10_SPS:
      console.log.println("10 samples/s");
      break;
  }

  // Test Ambient Averaging
  // Can also try: AVG_1_SAMPLES, AVG_2_SAMPLES, AVG_4_SAMPLES,
  // AVG_8_SAMPLES, AVG_16_SAMPLES, AVG_32_SAMPLES, AVG_64_SAMPLES,
  // AVG_128_SAMPLES

  vcnl4020.setAmbientAveraging(AVG_1_SAMPLES);
  console.log.print("Ambient Averaging: ");
  switch(vcnl4020.getAmbientAveraging())
  {
    case AVG_1_SAMPLES:
      console.log.println("1 sample");
      break;
    case AVG_2_SAMPLES:
      console.log.println("2 samples");
      break;
    case AVG_4_SAMPLES:
      console.log.println("4 samples");
      break;
    case AVG_8_SAMPLES:
      console.log.println("8 samples");
      break;
    case AVG_16_SAMPLES:
      console.log.println("16 samples");
      break;
    case AVG_32_SAMPLES:
      console.log.println("32 samples");
      break;
    case AVG_64_SAMPLES:
      console.log.println("64 samples");
      break;
    case AVG_128_SAMPLES:
      console.log.println("128 samples");
      break;
  }

  // Setup IRQ pin output when proximity data is ready
  vcnl4020.setInterruptConfig(true /* Proximity Ready */, false /* ALS Ready */, false /* Threshold */,
                              false /* true = Threshold ALS, false = Threshold Proximity */, INT_COUNT_1 /* how many values before the INT fires */
  );

  // The proximity measurement is using a square IR signal as
  // measurement signal. Four different values are
  // possible: PROX_FREQ_390_625_KHZ, PROX_FREQ_781_25_KHZ,
  // PROX_FREQ_1_5625_MHZ, PROX_FREQ_3_125_MHZ
  vcnl4020.setProxFrequency(PROX_FREQ_390_625_KHZ);
  console.log.print("Proximity Frequency: ");
  switch(vcnl4020.getProxFrequency())
  {
    case PROX_FREQ_390_625_KHZ:
      console.log.println("390.625 KHz");
      break;
    case PROX_FREQ_781_25_KHZ:
      console.log.println("781.25 KHz");
      break;
    case PROX_FREQ_1_5625_MHZ:
      console.log.println("1.5625 MHz");
      break;
    case PROX_FREQ_3_125_MHZ:
      console.log.println("3.125 MHz");
      break;
  }


  // finally, we can set up all the sensors we want to use. for example:
  // enable light sensor and proximity sensor, we will also use 'self timed' mode, which means
  // measurements are taken repeatedly for us and we dont have to do an 'on demand' measurement
  // request (simpler for getting started)
  vcnl4020.enable(true /* ALS Enable */, true /* Proximity Enable */, true /* Self-Timed Enable */);
  // dont use on-demand, we will have continuous reads.
  vcnl4020.setOnDemand(false /* ALS on demand read */, false /* Prox on demand read */);
}

void loop()
{
  vTaskDelay(100);

  // if(vcnl4020.isProxReady())
  // {
  //   console.log.print("Prox: ");
  //   console.log.println(vcnl4020.readProximity());
  //   vcnl4020.clearInterrupts(                                                             // Clear Interrupts
  //     true /* Proximity Ready */, false /* ALS Ready */, false /* Threshold Low */, false /* Threshold High */
  //   );
  // }

  // if(vcnl4020.isAmbientReady())
  // {
  //   console.log.print("Ambient: ");
  //   console.log.print(vcnl4020.readAmbient());
  //   console.log.print(", ");
  //   vcnl4020.clearInterrupts(                                                             // Clear Interrupts
  //     false /* Proximity Ready */, true /* ALS Ready */, false /* Threshold Low */, false /* Threshold High */
  //   );
  // }
}

// void scrollTextNonBlocking(const char* text, int speed)
// {
//   static int x = 5;
//   const int len = strlen(text) * 6;

//   disp.fillScreen(0);
//   disp.setCursor(x, 4);
//   disp.print(text);
//   static uint32_t tShift = 0;
//   if(millis() - tShift >= speed)
//   {
//     tShift = millis();
//     if(--x < -len)
//     {
//       x = 5;
//     }
//   }
// }

static void updateTask(void* param)
{
  while(true)
  {
    static bool btnOld = false, btnNew = false;
    btnOld = btnNew;
    btnNew = !digitalRead(BTN_PIN);
    if(!btnOld && btnNew)
    {
      static uint16_t counter = 0;
      char buffer[60];
      sprintf(buffer, "Button Pressed: %d", counter++);
      console.log.printf("%s\n", buffer);
      discord.sendEvent(buffer);
    }

    if(utils.getConnectionState())
    {
      if(githubOTA.updateAvailable())
      {
        githubOTA.startUpdate();
      }
      if(githubOTA.updateInProgress())
      {
        disp.setState(DisplayMatrix::UPDATING);
        disp.setUpdatePercentage(githubOTA.getProgress());
      }
      else
      {
        disp.setState(DisplayMatrix::IDLE);
        disp.setMessage(discord.getLatestMessage());
      }
    }
    else
    {
      disp.setState(DisplayMatrix::DISCONNECTED);
    }

    vTaskDelay(50);
    utils.resetWatchdog();
  }
}
