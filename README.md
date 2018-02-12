# MrCoffeeBot

This is the software that runs my modified 12 - Cup Mr. Coffee pot.

NOTE: only the mbed / c++ code is used currently, the python client is dated and bad.

## Hardware

The controller is an Mbed [NXP LPC1768](https://developer.mbed.org/platforms/mbed-LPC1768/),
 connected to a solid state relay wired inline to the heating element power,
 and a ds18b20 temperature sensor embedded against the hot plate. 

The controller communicates over USB serial with a host device.

Pin #8 controls the solid state relay, while pin 21 is wired to the DS1820 temperature probe.

## Question: Why an Mbed? Isn't that overkill?
Answer: Well I already wrote a different version of this for a class
 project that communicated with an android app via an Android Open Accessory
 stack using the Mbed's USB host capabilities (and I already owned one at the
 time.) That said the new [mbed-os](https://www.mbed.com/en/platform/mbed-os/)
 is pretty nice and open source.


## Building
This project requires Python 2.7.X and the [gcc-arm-embedded](https://launchpad.net/gcc-arm-embedded) toolchain.

Install the mbed tooling and other requirements with: 
 `pip install -r requirements.txt`.  

Then install [gcc-arm-embedded](https://launchpad.net/gcc-arm-embedded).  

On macOS you can do `brew install Caskroom/cask/gcc-arm-embedded`.  
On Ubuntu you can do:  
```
sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
sudo apt-get update
sudo apt-get install gcc-arm-embedded
```

Or alternatively on either you could install from the source linked above.  
This code is not tested on other compilers.



Finally run `mbed deploy` from the repository to fetch the mbed dependencies,
 and then `mbed compile -t GCC_ARM -m LPC1768` or `./build.py`.


## License

Licensed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).  
See LICENSE.

DS1820.h and DS1820.cpp are written by Michael Hagberg Michael@RedBoxCode.com with 
 modifications by Benjamin Elder, and are also under the Apache v2.0 License. 
