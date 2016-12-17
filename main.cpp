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

#include "mbed.h"
#include "DS1820.h"
#include <limits>

// hardware pinout constants
#define TEMPERATURE_PROBE_PIN p8
#define HEATER_PIN p21

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
DigitalOut heater(HEATER_PIN);
DS1820 probe(TEMPERATURE_PROBE_PIN);
Watchdog wdt;
Serial pc(USBTX, USBRX);

#define RECEIVE_BUFF_SIZE 7*3
char recv_buff[RECEIVE_BUFF_SIZE];

// For debug only, this is an LED on the mbed device.
DigitalOut led1(LED1);

// main() runs in its own thread in mbed-OS
int main() {
    // clarify that heater is off on boot
    heater = false;
    pc.baud(115200);
    pc.printf("MrCoffeeBot v2.0 Booted.\n");
    // 5 second timeout before rebooting
    wdt.setTimeout(10);
    // TODO: talk to DS1820
    double temperature = std::numeric_limits<double>::quiet_NaN();
    // main loopFlag
    char *curr_buff = recv_buff;
    memset(recv_buff, 0, RECEIVE_BUFF_SIZE);
    while (true) {
        led1 = !led1;
        // get a line
        bool newline = false;
        while (!newline) {
            // wait for character
            // while (!pc.readable());
            // don't overflow read buffer, at this point something is wrong
            if (curr_buff == recv_buff + RECEIVE_BUFF_SIZE) {
                curr_buff = recv_buff;
            }
            *curr_buff = pc.getc();
            newline = (*curr_buff == '\n');
            curr_buff++;
        }
        // process line
        if (starts_with("TEMP+?", recv_buff)) {
            // reset watchdog
            wdt.feed();
            // reply with temperature
            probe.convertTemperature(true, DS1820::this_device);
            temperature = probe.temperature();
            pc.printf("TEMP+%.1f\n", temperature);
        } else if (starts_with("BREW+", recv_buff)) {
            // reset watchdog
            wdt.feed();
            // update heater setting and reply with heater value
            bool new_heater = recv_buff[5] != '0';
            heater = new_heater;
            if (new_heater) {
                pc.printf("BREW+1\n");
            } else {
                pc.printf("BREW+0\n");
            }
        }
        curr_buff = recv_buff;
    }
}

