# MrCoffeeBot

This is the software that runs my modified 12 - Cup Mr. Coffee pot.

## Hardware

The controller is an Mbed [NXP LPC1768](https://developer.mbed.org/platforms/mbed-LPC1768/),
 connected to a solid state relay wired inline to the heating element power,
and a ds18b20 temperature sensor embedded against the hot plate. 

The controller communicates over USB serial with a host device.

## Question: Why an Mbed? Isn't that overkill?
Answer: Well I already wrote a different version of this for a class
 project that communicated with an android app via an Android Open Accessory
 stack using the Mbed's USB host capabilities (and I already owned one at the
 time.) That said the new [mbed-os](https://www.mbed.com/en/platform/mbed-os/) is pretty nice and open source.

// TODO: pin-out / wiring.

## Building
Install the mbed tooling and other requirements with: 
 `pip install -r requirements.txt`.  

Then install [gcc-arm-embedded](https://launchpad.net/gcc-arm-embedded).  

On macOS you can do `brew install Caskroom/cask/gcc-arm-embedded`.  

## License

Licensed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).  
See LICENSE.