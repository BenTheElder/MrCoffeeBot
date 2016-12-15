#!/usr/bin/env python
"""
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
"""
from __future__ import print_function
import time
from datetime import datetime
import sys

import coffee_bot

def do_brew(bot, brew_seconds=30):
    """
        do_brew attempts to connect bot and and brew for brew_seconds
    """
    print("Connecting to: '%s'"%(bot.get_port_path()))
    bot.run()
    print("Brewing for %d seconds."%(brew_seconds))
    bot.turn_on_heater()
    time.sleep(1)
    now = time.time()    
    deadline = now + brew_seconds
    while now < deadline:
        now_date = datetime.fromtimestamp(now)
        remaining = deadline - now
        sys.stdout.write("TIME: %s Remaining: %f Temp: %fc        \r"%
                    (now_date, remaining, bot.maybe_current_temp()))
        sys.stdout.flush()
        now = time.time()
    print("\nTurning off Heater.")
    bot.turn_off_heater()
    bot.stop()
    time.sleep(1)
    sys.stdout.write("Done!\n")
    sys.stdout.flush()

def main():
    """
        the entry point to more_coffee
        makes roughly 4 cups (30 seconds)
    """
    brew_seconds = 30
    print("Finding Mr. Coffee Bot")
    bot = coffee_bot.get_coffee_bot()
    try:
        do_brew(bot, brew_seconds=210)
    except KeyboardInterrupt:
        print("\nCaught KeyboardInterrupt, Turning off Heater.")
        bot.turn_off_heater()
        bot.stop()
        time.sleep(1)

if __name__ == "__main__":
    main()
