# Hologram-Nova
Libraries to help you get connected with the Hologram Nova 

This is an unoffical repo with methods to connect the Hologram Nova and possibly other modems to the hologram network.

## Windows Desktop
This version makes use of the Windows RAS component to create and dial up a connection to the cellular network.

This requires the libs:
 * rasapi32.lib
 * Setupapi.lib

### Warning 
Without routing internet traffic your machine may make a lot requests on the cellular network and cause you to rack up a lot of data usage unintentionally!

## Thanks & Credit
Some of this code was adapted from parts of the openframeworks project: https://github.com/openframeworks/openFrameworks

Also thanks to the code from winutls which was part of the inspiration on the design: https://github.com/northbright/WinUtil
