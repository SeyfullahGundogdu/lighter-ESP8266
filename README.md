# MQTT Communication with ESP8266 Board
This project uses an ESP8266 Wi-Fi chip.
To connect to your Wireless Network, change SSID and password variables accordingly.

The chip connects to `test.mosquitto.org:1883` and uses QoL 0 level. 
Then it waits for user input at the `rgb-in` topic.
Parsing the incoming messages, it runs 1 of 2 functions to set the light source's brightness and color:

1- processRGBT Function:
This function sets all 3 values at once, color parameters must be between 0-255, timer parameter  can be chosen arbitrarily.

2- processSingleColor function:
This function sets a single color's PWM value, again in 0-255 range.
First parameter is the PWM value and the second parameter is timer value, again chosen arbitrarily.

The chip outputs many info through Serial port(baud: 115200), those can be used for debuggin in case something goes wrong, chip also outputs the RGB and timer values to the MQTT topic `rgb-out` every time callback funtion gets triggered, which is when a message gets published on the `rgb-in` topic.
RGB pins used for communicating with a light source is D1, D2 and D4 respectively.
It might be possible to increase PWM range of these pins, but haven't tried that yet.