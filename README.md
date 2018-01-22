# Internet_Fire_Alarm
Connecting a ESP8266 to Kidde Smoke/Fire Alarm System. ESP8266 sends a MQTT message until the smoke alarm is beeping.

[![YouTube](http://img.youtube.com/vi/rgAuyPvZLB4/0.jpg)](http://www.youtube.com/watch?v=rgAuyPvZLB4)

Refer to https://github.com/debsahu/ESP_External_Interrupt to build the ESP8266 part of the internet connected smoke alarm.

Overall idea is to detect smoke alarm signals and send MQTT message. Home Assistant reads the MQTT message and sends out notifications that can be used to notify your local fire station.

![Overall](Overview.png)

We will be hacking into Kidde RF-SM-DC

![KiddeRFSMDC](Kidde_overall.png)

Zoomed Kidde RF-SM-DC's location of 3.3V signal 

![KiddeRFSMDCZoom](Kidde_zoom.png)
