/*************************************************************************************************

 ________    ______    _______    ____  ____                                 
|_   __  | .' ____ \  |_   __ \  |_   ||   _|                                
  | |_ \_| | (___ \_|   | |__) |   | |__| |     .--.    _ .--..--.    .---.  
  |  _| _   _.____`.    |  ___/    |  __  |   / .'`\ \ [ `.-. .-. |  / /__\\ 
 _| |__/ | | \____) |  _| |_      _| |  | |_  | \__. |  | | | | | |  | \__., 
|________|  \______.' |_____|    |____||____|  '.__.'  [___||__||__]  '.__.' 
                                                                             
              _    ______                                _                   
             (_)  |_   _ \                              / |_                 
             __     | |_) |    .--.     .--.    .--.   `| |-'                
            [  |    |  __'.  / .'`\ \ / .'`\ \ ( (`\]   | |                  
             | |   _| |__) | | \__. | | \__. |  `'.'.   | |,                 
            [___] |_______/   '.__.'   '.__.'  [\__) )  \__/                 
                                                                             
                                                                             

MIT License

Copyright (c) 2023 JNSwanson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*************************************************************************************************


    ESP32 DevKit V1 PIN connections (VSPI default)
   CC1101         ESP32

    VCC           3V3
    GND           GND
    SCK           GPIO18
    MOSI          GPIO23
    MISO          GPIO19 + GPIO4 (IMPORTANT: wire MISO to BOTH pins)
    CS            GPIO5

    WARNING: The CC1101 MISO pin must be connected to TWO ESP32 pins:
    - GPIO19 (hardware SPI MISO)
    - GPIO4  (for digitalRead while SPI is active)
    
    This mirrors the ESP8266 workaround where the MCU cannot digitalRead()
    the hardware MISO pin during SPI activity. Avoid strapping pins (0,2,12,15)
    for CS or the extra MISO-read pin.

    Original ESP8266 (NodeMCU) connections were:
    CSN=D8, CLK=D5, MISO=D6+D2, MOSI=D7

	
******************************************************************************************************/

#ifndef ESPHOME_COMPONENTS_IBOOST_IBOOST_H
#define ESPHOME_COMPONENTS_IBOOST_IBOOST_H

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/cc1101/cc1101.h"

#ifdef USE_ESP32
#include "driver/gpio.h"
#endif

namespace esphome {
namespace iboost {

static const char *const TAG = "iboost";

// Timing constants (milliseconds)
static const uint32_t PING_INTERVAL_MS = 10000;      // Time between pings
static const uint32_t LED_BLINK_DURATION_MS = 200;   // LED on time after packet
static const uint32_t TX_WINDOW_START_MS = 1000;     // Start of transmit window after RX
static const uint32_t TX_WINDOW_END_MS = 2000;       // End of transmit window after RX

extern sensor::Sensor *heating_import;
extern sensor::Sensor *heating_power;
extern sensor::Sensor *heating_today;
extern sensor::Sensor *heating_yesterday;
extern sensor::Sensor *heating_last_7;
extern sensor::Sensor *heating_last_28;
extern sensor::Sensor *heating_last_gt;
extern sensor::Sensor *heating_boost_time;
extern text_sensor::TextSensor *heating_mode;
extern text_sensor::TextSensor *heating_warn;
extern binary_sensor::BinarySensor *tank_hot;
extern binary_sensor::BinarySensor *sender_battery_low;

extern long today, yesterday, last7, last28, total;

class iBoost : public PollingComponent {
 public:
 
  // Constructor
  //iBoost();
  iBoost() : PollingComponent(15000){}  // Poll every 15 seconds


  // Override setup() from PollingComponent
  void setup() override;
  void loop() override;
  void update() override;
  void boost(uint8_t boost_time);
  

    void set_heating_import(sensor::Sensor *sensor) { heating_import = sensor; }
    void set_heating_power(sensor::Sensor *sensor) { heating_power = sensor; }
    void set_heating_today(sensor::Sensor *sensor) { heating_today = sensor; }
    void set_heating_yesterday(sensor::Sensor *sensor) { heating_yesterday = sensor; }
    void set_heating_last_7(sensor::Sensor *sensor) { heating_last_7 = sensor; }
    void set_heating_last_28(sensor::Sensor *sensor) { heating_last_28 = sensor; }
    void set_heating_last_gt(sensor::Sensor *sensor) { heating_last_gt = sensor; }
    void set_heating_boost_time(sensor::Sensor *sensor) { heating_boost_time = sensor; }
    void set_heating_mode(text_sensor::TextSensor *sensor) { heating_mode = sensor; }
    void set_heating_warn(text_sensor::TextSensor *sensor) { heating_warn = sensor; }
    void set_tank_hot(binary_sensor::BinarySensor *sensor) { tank_hot = sensor; }
    void set_sender_battery_low(binary_sensor::BinarySensor *sensor) { sender_battery_low = sensor; }

private:
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }



};
extern iBoost *global_iboost;  // Declare global instance of iBoost
}  // namespace iboost
}  // namespace esphome
#endif
