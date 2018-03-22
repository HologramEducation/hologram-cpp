# Hologram-SDK
Libraries to help you get connected with the Hologram Nova 

This is an unoffical repo with methods to connect the Hologram Nova and possibly other modems to the hologram network. Right now it contains things written in C++ but in the future it should have many usable languages.

## Windows Desktop
This version makes use of the Windows RAS component to create and dial up a connection to the cellular network.

### Uses the following static libraries:
 * rasapi32.lib
 * Cfgmgr32.lib
 * Ws2_32.lib
 
## Windows IOT Core/Windows Universal App
Still a work in progress, currently there does not appear to be a PPP implmentation in the IoT Core so this will try to create a minimal working option using AT commands. Possible options for a PPP stack might rely on: http://savannah.nongnu.org/projects/lwip/

### Uses the following static libraries:
 * onecoreuap.lib
 * Cfgmgr32.lib
 * Ws2_32.lib

## MacOS
Early beta, still some bugs to work out!

### Required Frameworks:
 * IOKit
 * Core Foundation

## Linux
While it should probably compile it has not been tested yet and our offical python SDK is a much better option for deveopers
https://github.com/hologram-io/hologram-python

### Warning 
Without routing internet traffic your machine may make a lot requests on the cellular network and cause you to rack up a lot of data usage unintentionally!

## Thanks & Credit
Some of this code was adapted from parts of the openframeworks project: https://github.com/openframeworks/openFrameworks
