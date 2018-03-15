# Windows IoT Core
Beta version that emulates the python SDK structure, connecting to a Nova and sending AT commands/sending messages to through the modem socket works. Currently missing PPP functionality as the IoT core does not implment the full RAS features found on windows desktops. 

Check the legacy folder code for the PPP class written by Microsoft though it has not been tested. Alternatively exploring LwIP as an option for creating the PPP connection - http://savannah.nongnu.org/projects/lwip/
