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
#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include "mbed.h"

// RateLimiter throttles calls to a method
class RateLimiter {
private:
    Timer timer;
    int rate_us;
public:
    void (*fn)(void);

    RateLimiter(int rate_us, void (*f)(void)) {
        this->rate_us = rate_us;
        this->fn = f;
        this->timer.start();
    }

    // call if enough time has passed, return true if called
    bool call() {
        if (this->timer.read_us() >= this->rate_us) {
            this->fn();
            this->timer.reset();
            return true;
        }
        return false;
    }
};

#endif