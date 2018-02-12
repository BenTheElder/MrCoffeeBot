/*
Copyright 2018 Benjamin Elder (BenTheElder)

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
#ifndef HCSR04_H
#define HCSR04_H

#include "mbed.h"

// HC-SR04 sensor
class HCSR04 {
private:
    DigitalOut trig;
    DigitalIn echo;
public:
    HCSR04(PinName trigger_pin, PinName echo_pin) : trig(trigger_pin), echo(echo_pin) {}

    // trigger sensor and read raw timing in us
    int read_raw() {
        Timer tmr;
        // clear trigger pin low
        this->trig.write(0);
        wait_us(5);
        // write 10us high trigger
        this->trig.write(1);
        wait_us(10);
        this->trig.write(0);
        // read result, time low to high
        // wait for low first, then start timing
        while (!echo);
        tmr.start();
        while (echo);
        tmr.stop();
        return tmr.read_us();
    }

    // convert raw us timing to inches
    static double inches_from_raw(int raw_us) {
        return (raw_us/2.) / 74.;
    }

    // trigger sensor and return inches read
    double read_inches() {
        return inches_from_raw(this->read_raw());
    }
};

#endif