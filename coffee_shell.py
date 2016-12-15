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

# TODO: these constants will need to be refined
TARGET_INITIAL_TEMP = 54
SECONDS_PER_CUP = 52.5

def do_brew(bot, seconds=0, cups=None):
    """
        do_brew attempts to connect bot and and brew for brew_seconds
        unless cups != None, in which case do_brew will attemp to brew
        <cups> cups of coffee.
    """
    print("Connecting to: '%s'"%(bot.get_port_path()))
    bot.run()
    if cups is None:
        print("Brewing for %d seconds."%(seconds))
        bot.turn_on_heater()
        time.sleep(1)
        now = time.time()
        deadline = now + seconds
        while now < deadline:
            now_date = datetime.fromtimestamp(now)
            remaining = deadline - now
            temp = bot.maybe_current_temp()
            temp_s = ""
            if temp is None or temp == 0:
                temp_s = "?"
            else:
                temp_s = "%.1f" % (temp)
            sys.stdout.write("TIME: %s Remaining: %f Temp: %s c        \r"%
                                (now_date, remaining, temp_s))
            sys.stdout.flush()
            now = time.time()
    else:
        print("Brewing ~ %d cups."%(cups))
        bot.turn_on_heater()
        # ensure we wait for a temperature update
        bot._state.current_temp = 0
        now = time.time()
        while bot.maybe_current_temp() < TARGET_INITIAL_TEMP:
            now = time.time()
            now_date = datetime.fromtimestamp(now)
            temp = bot.maybe_current_temp()
            temp_s = ""
            if temp is None or temp == 0:
                temp_s = "?"
            else:
                temp_s = "%.1f" % (temp)
            sys.stdout.write("TIME: %s Target Temp: %.1f Temp: %s c        \r"%
                                (now_date, TARGET_INITIAL_TEMP, temp_s))
            sys.stdout.flush()
        brew_seconds = SECONDS_PER_CUP * cups
        now = time.time()
        deadline = now + brew_seconds
        while now < deadline:
            now_date = datetime.fromtimestamp(now)
            remaining = deadline - now
            temp = bot.maybe_current_temp()
            temp_s = ""
            if temp is None or temp == 0:
                temp_s = "?"
            else:
                temp_s = "%.1f" % (temp)
            sys.stdout.write("TIME: %s Remaining: %f Temp: %s c        \r"%
                                (now_date, remaining, temp_s))
            now = time.time()
    print("\nTurning off Heater.")
    bot.turn_off_heater()
    bot.stop()
    time.sleep(1)
    sys.stdout.write("Done!\n")
    sys.stdout.flush()

def amount_to_args(amount_str):
    """
        amount_to_args parses an amount string like `n <cups>`
        returns a kwargs for do_brew
    """
    kwargs = {}
    if not amount_str:
        return None
    if amount_str == "more":
        return {"seconds": 210}
    if amount_str != "cup" and amount_str.endswith("cup"):
        amount = amount_str[:len(amount_str)-len("cup")]
        kwargs = {"cups": float(amount)}
    elif amount_str != "cups" and amount_str.endswith("cups"):
        amount = amount_str[:len(amount_str)-len("cups")]
        kwargs = {"cups": float(amount)}
    elif amount_str != "second" and amount_str.endswith("second"):
        amount = amount_str[:len(amount_str)-len("seconds")]
        kwargs = {"seconds": float(amount)}
    elif amount_str != "seconds" and amount_str.endswith("seconds"):
        amount = amount_str[:len(amount_str)-len("seconds")]
        kwargs = {"seconds": float(amount)}
    return kwargs

def get_command():
    """
        get_command displays a prompt and parses the command
        returns (None or command string, None or command arugments)
    """
    # get input
    if sys.version_info[0] < 3:
        get_input = raw_input
    else:
        get_input = input
    raw = get_input("> ")
    # parse command
    cmd, args = None, None
    parts = raw.split(" ", 1)
    # handle no arguments case
    if raw == "exit":
        return ("exit", args)
    elif len(parts) < 2:
        return (cmd, args)
    # handle commands with arguments
    if parts[0] == "brew":
        # parse amount
        try:
            args = amount_to_args(parts[1])
            if args is not None:
                cmd = "brew"
        except ValueError:
            pass
    return (cmd, args)

def print_motd():
    """
        print_motd prints the Coffee Shell MOTD
    """
    motd = "Welcome to the Mr. Coffee Bot Shell.\n\n"+\
        "Recognized commands are:\n"+\
        " `exit` - quits the shell.\n\n"+\
        " `brew <amount>` - attempts to brew <amount> of coffee.\n"+\
        "   recognized amounts are:\n"+\
        "     <n> cup(s) - a number of cups up to 12\n"+\
        "     <n> second(s) - a number of seconds to run the heater\n"+\
        "     more - roughly 4 cups\n"+\
        "   Please ensure that the reservoir contains enough water!\n"
    print(motd)

def main():
    """
        this is the entry point to the Coffee Shell
    """
    print("Finding Mr. Coffee Bot")
    bot = coffee_bot.get_coffee_bot()
    print_motd()
    try:
        # Coffee REPL
        while True:
            command, args = get_command()
            if command is None:
                print("Unrecognized Command.")
            elif command == "exit":
                break
            elif command == "brew":
                do_brew(bot, **args)
    except KeyboardInterrupt:
        print("\nCaught KeyboardInterrupt, Turning off Heater.")
        bot.turn_off_heater()
        bot.stop()
        time.sleep(1)

if __name__ == "__main__":
    main()

