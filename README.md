# Hologram-SDK
Libraries to help you get connected with the Hologram Nova 

This is an unoffical repo with methods to connect the Hologram Nova and possibly other modems to the hologram network.

## Windows Desktop
This version makes use of the Windows RAS component to create and dial up a connection to the cellular network.

This requires the libs:
 * rasapi32.lib
 * Setupapi.lib
 
## Windows IOT Core
Still a work in progress, currently there does not appear to be a PPP implmentation in the IoT Core so this will try to create a minimal working option using AT commands. Possible options for a PPP stack might rely on: http://savannah.nongnu.org/projects/lwip/

This requires the libs:
 * Ws2_32.lib
 * Cfgmgr32.lib
 * onecoreuap.lib

### Warning 
Without routing internet traffic your machine may make a lot requests on the cellular network and cause you to rack up a lot of data usage unintentionally!

## Thanks & Credit
Some of this code was adapted from parts of the openframeworks project: https://github.com/openframeworks/openFrameworks

Also thanks to the code from winutls which was part of the inspiration on the design: https://github.com/northbright/WinUtil
