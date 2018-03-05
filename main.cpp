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


// TODO(bentheelder): move this to it's own file(s)
// manages the heater's state, automatic shutoff, etc
// requries regularly calling .poll()
class Heater {
private:
    DigitalOut pin;
    Timer      sinceLastUserWrite;
    int        enableTimeout;
    void (*enableCallback)(void);
    void (*disableCallback)(void);

    void disable_internal() {
        this->pin.write(0);
        if (this->disableCallback) {
            this->disableCallback();
        }
    }

    void enable_internal() {
        this->pin.write(1);
        if (this->enableCallback){
            this->enableCallback();
        }
    }

public:
    Heater(PinName heaterPin) : pin(heaterPin) {
        this->pin.write(0);
        this->sinceLastUserWrite.start();
    }

    void setTimeout(int enable_timeout_us) {
        this->enableTimeout = enable_timeout_us;
    }

    void setEnableCallback(void (*f)(void)) {
        this->enableCallback = f;
    }

    void setDisableCallback(void (*f)(void)) {
        this->disableCallback = f;
    }

    // call regularly
    void poll() {
        if (this->sinceLastUserWrite.read_us() >= this->enableTimeout) {
            this->disable_internal();
        }
    }

    // return the heater's state (reads the pin)
    bool read() {
        return this->pin.read();
    }

    void disable() {
        this->sinceLastUserWrite.reset();
        this->disable_internal();
    }

    void enable() {
        this->sinceLastUserWrite.reset();
        this->enable_internal();
    }
};


// Globals
// device watchdog timer
Watchdog wdt;

// the coffeepot heater
Heater heater(HEATER_PIN);

// temperature probe in the base
DS1820 temp_probe(TEMPERATURE_PROBE_PIN);
double temperature;

// ultrasonic sensor in top of water resevoir
HCSR04 water_level_sensor(WATER_HCSR04_TRIG_PIN, WATER_HCSR04_ECHO_PIN);
double water_distance_inches;

// helper to reset the device (uses a pin wired to reset)
DigitalInOut reset_pin(RESET_PIN);
void reset() {
    reset_pin.mode(OpenDrain);
}

// Serial communcation over USB
Serial pc(USBTX, USBRX);
// serial receive buffer
#define RECEIVE_BUFF_SIZE 7*3
char recv_buff[RECEIVE_BUFF_SIZE];

// For debug only, these are LEDs on the mbed device.
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
// debug helpers
void led1_toggle() {
    led1 = !led1;
}
void led2_on() {
    led2.write(1);
}
void led2_off() {
    led2.write(0);
}
void led3_toggle() {
    led3 = !led3;
}
void heater_disble_callback() {
    led3_toggle();
    led2_off();
}
void heater_enable_callback() {
    led3_toggle();
    led2_on();
}



// helpers rate limited in main loop to poll sensors
void update_temperature() {
    temp_probe.convertTemperature(false, DS1820::this_device);
    temperature = temp_probe.temperature();
}

void update_water_level() {
    water_distance_inches = water_level_sensor.read_inches();
}

// serial command strings
#define COMMAND_RESET        "RESET"
#define COMMAND_STATUS       "S+?"
#define COMMAND_BREW_ENABLE  "B+1"
#define COMMAND_BREW_DISABLE "B+0"

// NOTE: if we poll the HCSR04 too fast the readings are useless
RateLimiter water_level_sensor_rate_limiter(5000, update_water_level);
RateLimiter temperature_sensor_rate_limiter(5000, update_temperature);

// helper method for handling serial commands
bool starts_with(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

// status of all sensors + heater enable (W = Water, T = Temp, B = BREW)
void send_status() {
    pc.printf("W+%.2f,T+%.1f,B+%d\n", 
              water_distance_inches, temperature, heater.read() ? 1 : 0);
}

// process_line handles one line of input and returns true if the line
// was valid / handled and WDT should be reset
bool process_line() {
    if (starts_with(COMMAND_STATUS, recv_buff)) {
        send_status();

    } else if (starts_with(COMMAND_BREW_ENABLE, recv_buff)) {
        heater.enable();
        send_status();

    } else if (starts_with(COMMAND_BREW_DISABLE, recv_buff)) {
        heater.disable();
        send_status();

    } else if (starts_with(COMMAND_RESET, recv_buff)) {
        reset();

    } else {
        return false;
    }
    // if we don't hit the else above, we processed a line
    return true;
}

// main() runs in its own thread in mbed-OS
int main() {
    // init heater
    heater.setDisableCallback(heater_disble_callback);
    heater.setEnableCallback(heater_enable_callback);
    heater.setTimeout(1000000); // 1s
    // clarify that heater is off on boot
    heater.disable();

    // init various vars
    memset(recv_buff, 0, RECEIVE_BUFF_SIZE);
    water_distance_inches = std::numeric_limits<double>::max();
    temperature = std::numeric_limits<double>::max();

    // Initialization, set up watchdog, serial, etc.
    pc.baud(115200);
    pc.printf("MrCoffeeBot v2.0 Booted.\n");
    // 5 second timeout before rebooting
    // WDT is fed when handling a valid command
    wdt.setTimeout(5);

    // update sensors once before main loop
    water_level_sensor_rate_limiter.ignore_limit_and_call();
    temperature_sensor_rate_limiter.ignore_limit_and_call();
    // current location in the receive buffer
    char *curr_buff = recv_buff;
    while (true) {
        // update the heater every loop, potentially disabling it on timeout
        heater.poll();

        // poll sensors
        water_level_sensor_rate_limiter.call();
        temperature_sensor_rate_limiter.call();

        // handle input
        bool received_newline = false;
        while (pc.readable() && !received_newline) {
            // don't overflow read buffer, at this point something is wrong
            if (curr_buff == recv_buff + RECEIVE_BUFF_SIZE) {
                curr_buff = recv_buff;
                memset(recv_buff, 0, RECEIVE_BUFF_SIZE);
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
                led1_toggle();
            }
            // reset buffer after processing a line
            curr_buff = recv_buff;
            memset(recv_buff, 0, RECEIVE_BUFF_SIZE);
        }
    }
}
