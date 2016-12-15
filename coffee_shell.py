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

# TODO: this will constant need to be refined
CUPS_PER_SECOND = 7.5
MAX_BREW_SECONDS = 12*CUPS_PER_SECOND

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

def amount_to_seconds(amount_str):
    """
        amount_to_seconds parses an amount string like `n <cups>`
        returns a float number of seconds or None
    """
    seconds = None
    if not amount_str:
        return seconds
    if amount_str == "more":
        return 4 * CUPS_PER_SECOND
    if amount_str != "cups" and amount_str.endswith("cups"):
        amount = amount_str[:len(amount_str)-len("cups")]
        seconds = float(amount) * CUPS_PER_SECOND
    elif amount_str != "seconds" and amount_str.endswith("seconds"):
        amount = amount_str[:len(amount_str)-len("seconds")]
        seconds = float(amount)
    return seconds

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
        amount = amount_to_seconds(parts[1])
        if amount is not None:
            cmd, args = "brew", {"brew_seconds": amount}
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
        "     <n> cups - a number of cups up to 12\n"+\
        "     <n> seconds - a number of seconds to run the heater\n"+\
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
            #try:
            command, args = get_command()
            #except:
            #     print("Unrecognized Command.")
            #     continue
            if command is None:
                print("Unrecognized Command.")
            elif command == "exit":
                break
            elif command == "brew":
                if args["brew_seconds"] > MAX_BREW_SECONDS:
                    print("ERROR: 12 cups / %d seconds is the maximum." %\
                        (MAX_BREW_SECONDS))
                else:
                    do_brew(bot, **args)
    except KeyboardInterrupt:
        print("\nCaught KeyboardInterrupt, Turning off Heater.")
        bot.turn_off_heater()
        bot.stop()
        time.sleep(1)

if __name__ == "__main__":
    main()
