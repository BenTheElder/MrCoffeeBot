/*
Copyright 2016 Benjamin Elder (BenTheElder)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <limits>

#include "mbed.h"

#include "DS1820.h"
#include "HCSR04.h"
#include "RateLimiter.h"


// Hardware pinout constants
// ds1820 temperature probe
#define TEMPERATURE_PROBE_PIN p8
// SSR in-line with coffee pot power switch
#define HEATER_PIN p21
// hc-sr04 ultrasonic ranger for water level
#define WATER_HCSR04_TRIG_PIN p22
#define WATER_HCSR04_ECHO_PIN p23
// pin wired to nR reset pin on the NXP LPC1768
#define RESET_PIN p6


// Watchdog class based on
// https://developer.mbed.org/cookbook/WatchDog-Timer
// https://developer.mbed.org/forum/mbed/topic/508/
// And the NXP LPC1768 User Manual:
// http://www.nxp.com/documents/user_manual/UM10360.pdf
class Watchdog {
public:
    // Load timeout value in watchdog timer and enable
    void setTimeout(float s) {
        // Set CLK src to PCLK
        LPC_WDT->WDCLKSEL = 0x1;
        // WD has a fixed /4 prescaler, PCLK default is /4
        uint32_t clk = SystemCoreClock / 16;
        LPC_WDT->WDTC = s * (float)clk;
        // Enabled and Reset
        LPC_WDT->WDMOD = 0x3;
        feed();
    }
    // "feed" the dog - reset the watchdog timer by writing
    // this required bit pattern (0xAA55)
    void feed() {
        LPC_WDT->WDFEED = 0xAA;
        LPC_WDT->WDFEED = 0x55;
    }
};


// helper method
bool starts_with(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

// Globals
Watchdog wdt;

DigitalOut heater(HEATER_PIN);

DS1820 temp_probe(TEMPERATURE_PROBE_PIN);
double temperature;

HCSR04 water_level_sensor(WATER_HCSR04_TRIG_PIN, WATER_HCSR04_ECHO_PIN);
double water_distance_inches;

DigitalInOut reset_pin(RESET_PIN);

Serial pc(USBTX, USBRX);
// serial receive buffer
#define RECEIVE_BUFF_SIZE 7*3
char recv_buff[RECEIVE_BUFF_SIZE];

// For debug only, this is an LED on the mbed device.
DigitalOut led1(LED1);


void update_temperature() {
    temp_probe.convertTemperature(true, DS1820::this_device);
    temperature = temp_probe.temperature();
}

void update_water_level() {
    water_distance_inches = water_level_sensor.read_inches();
}

bool probably_has_water() {
    double d = water_distance_inches;
    // 7.5 inches is roughly the real maximum
    // after ~9.4 we can guess that the steam has skewed the measurement
    // after 20 (most arbitrary) the steam is intense and we should back off
    return (d < 7.5) || (d > 9.4 && d < 20);
}


/*
Methods for replying with statuses for varioius sensors
*/
void send_water() {
    pc.printf("W+%.2f\n", water_distance_inches);
}

void send_temp() {
    pc.printf("T+%.1f\n", temperature);
}

void send_brew() {
    if (heater) {
        pc.printf("B+1\n");
    } else {
        pc.printf("B+0\n");
    }
}

void reset() {
    reset_pin.mode(OpenDrain);
}

#define COMMAND_STATUS       "S"
#define COMMAND_WATER_LEVEL  "W"
#define COMMAND_TEMP         "T"
#define COMMAND_BREW         "B"

// process_line handles one line of input and returns true if the line
// was valid / handled and WDT should be reset
bool process_line() {
    if (starts_with("RESET", recv_buff)) {
        reset();

    } else if (starts_with("S+?", recv_buff)) {
        pc.printf("S+1\n");
        send_water();
        send_temp();
        send_brew();

    } else if (starts_with("W+?", recv_buff)) {
        send_water();

    } else if (starts_with("T+?", recv_buff)) {
        send_temp();

    } else if (starts_with("B+", recv_buff)) {
        // update heater setting and reply with heater value
        // ignore input and don't leave the heater on if there is no water
        bool new_heater = (recv_buff[5] != '0') && probably_has_water();
        heater = new_heater;
        send_brew();

    } else {
        return false;
    }
    // if we don't hit the else above, we processed a line
    return true;
}

// main() runs in its own thread in mbed-OS
int main() {
    // clarify that heater is off on boot
    heater = false;
    // zero other vars
    memset(recv_buff, 0, RECEIVE_BUFF_SIZE);
    water_distance_inches = std::numeric_limits<double>::max();
    temperature = std::numeric_limits<double>::max();

    // Initialization, set up watchdog, serial, etc.
    pc.baud(115200);
    pc.printf("MrCoffeeBot v2.0 Booted.\n");
    // 5 second timeout before rebooting
    // WDT is fed when handling a valid line
    wdt.setTimeout(5);

    // loop vars
    // NOTE: if we poll the HCSR04 too fast the readings are useless
    update_water_level();
    update_temperature();
    RateLimiter water_level_sensor_rate_limiter(5000, update_water_level);
    RateLimiter temperature_sensor_rate_limiter(100000, update_temperature);
    // current location in the receive buffer
    char *curr_buff = recv_buff;
    while (true) {
        // poll sensors
        water_level_sensor_rate_limiter.call();
        temperature_sensor_rate_limiter.call();

        // handle input
        bool received_newline = false;
        while (pc.readable() && !received_newline) {
            // don't overflow read buffer, at this point something is wrong
            if (curr_buff == recv_buff + RECEIVE_BUFF_SIZE) {
                curr_buff = recv_buff;
            }
            *curr_buff = pc.getc();
            received_newline = (*curr_buff == '\n');
            curr_buff++;
        }

        // process a line if we have one
        if (received_newline) {
            // and feed the watchdog if we process a legitimate line
            if (process_line()) {
                wdt.feed();
                // debug feeding watchdog
                led1 = !led1;
            }
            // reset buffer after processing a line
            curr_buff = recv_buff;
        }

        // always disable heater if there is no water
        if (!probably_has_water()) {
            heater = false;
        }
    }
}

