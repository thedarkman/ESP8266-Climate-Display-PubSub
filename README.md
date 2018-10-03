# ESP8266-Climate-Display-PubSub
This sketch means to be run on a WeMos D1 Mini with OLED Shield

It connects to the topics from the mqtt broker used in 
http://bitluni.net/solar-powered-weather-station/

Uses the following (external) libraries:
* PubSubClient: https://pubsubclient.knolleary.net/
* Forked Adafruit SSD1306 library to support the 64x48 Display: https://github.com/mcauser/Adafruit_SSD1306/tree/esp8266-64x48
