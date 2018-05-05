/*
 * This sketch means to be run on a WeMos D1 Mini with OLED Shield
 * 
 * It connects to the topics from the mqtt broker used in 
 * http://bitluni.net/solar-powered-weather-station/
 * (i used voltage instad of light sensor, so this display shows voltage)
 * 
 * As i have multiple WiFi AccessPoints around the house, i set three here in this 
 * sketch. You can remove two of them or integrate the WiFiManger library.
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <PubSubClient.h>

// OLED specific
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

/************************* CONFIG *****************************************************/
// MQTT broker settings
// IP address of the mqtt broker
#define HOST        "xxx.xxx.xxx.xxx" 
#define PORT        1883
// credentials needed for connection
#define USERNAME    "username"
#define PASSWORD    "password"
// Topics, change if yours are different
#define TOPIC_TEMPERATURE "weatherStation/temperature"
#define TOPIC_VOLTAGE     "weatherStation/voltage"
#define TOPIC_HUMIDITY    "weatherStation/humidity"
#define TOPIC_BAROMETRIC  "weatherStation/barometric"

// Multi WiFi
const char* ssid1     = "SSID_1";
const char* ssid2     = "SSID_2";
const char* ssid3     = "SSID_3";
const char* password1  = "PASSWORD_1";
const char* password2  = "PASSWORD_2";
const char* password3  = "PASSWORD_3";
/************************* CONFIG END *************************************************/

// Multiple WiFi connections
ESP8266WiFiMulti multi;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
PubSubClient mqtt(client);

double currentTemp = 0.0;
double currentVoltage = 0.0;
double currentHum = 0.0;
double currentBaro = 0.0;
const int timeout = 5;

// Define some display states
int state = 0;
#define STATE_TEMP 0
#define STATE_HUM 1
#define STATE_BARO 2
// how long to wait until switching state
#define WAIT_SECONDS 5
// millis of last switch
long lastSwitch = 0;

// ICONS
const unsigned char temp_icon [] = {
0x08, 0x00, 0x14, 0x00, 0x14, 0x00, 0x14, 0xF8, 0x14, 0x00, 0x14, 0xF8, 0x14, 0x00, 0x14, 0xF8,
0x14, 0x00, 0x14, 0x00, 0x22, 0x00, 0x41, 0x00, 0x41, 0x00, 0x41, 0x00, 0x22, 0x00, 0x1C, 0x00
};
const unsigned char humidity [] = {
0x00, 0x80, 0x01, 0xC0, 0x01, 0xC0, 0x03, 0x60, 0x03, 0x60, 0x06, 0x30, 0x06, 0x30, 0x0E, 0x38,
0x1C, 0x1C, 0x18, 0x0C, 0x30, 0x06, 0x30, 0x06, 0x18, 0x0C, 0x1C, 0x1C, 0x0F, 0xF8, 0x07, 0xF0
};
const unsigned char pressure [] = {
0x00, 0x3C, 0x00, 0x47, 0x00, 0xC1, 0x00, 0x83, 0x01, 0x02, 0x04, 0xFE, 0x08, 0x00, 0x1F, 0xFC,
0x08, 0x00, 0x24, 0x00, 0x40, 0x00, 0xFF, 0xE8, 0x40, 0x04, 0x2F, 0xFE, 0x00, 0x04, 0x00, 0x08
};

void barocallback(double p) {
  currentBaro = p;
}

void humcallback(double h) {
  currentHum = h;
}

void tempcallback(double t) {
  currentTemp = t;
}

void lipocallback(double v) {
  currentVoltage = v;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // we know there will be only double data as payload, so ...
  double dValue = atof((char *) payload);
  if(strcmp(topic, TOPIC_TEMPERATURE) == 0)
    tempcallback(dValue);
  else if(strcmp(topic, TOPIC_VOLTAGE) == 0)
    lipocallback(dValue);
  else if(strcmp(topic, TOPIC_HUMIDITY) == 0)
    humcallback(dValue);
  else if(strcmp(topic, TOPIC_BAROMETRIC) == 0)
    barocallback(dValue);  
}

/*
 * Draw values on display depending on the current state
 */
void drawValues() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  int decimals = 2;
  switch(state) {
    case STATE_TEMP:      
      if((currentTemp >= 0 || currentTemp > -10) && currentTemp < 10)
        display.print(" ");
      if(currentTemp < -10)
        decimals = 1;
      display.println(currentTemp, decimals);
      display.print("   ");
      display.print(char(167));
      display.print("C");
      display.drawBitmap(0,display.height()-16, temp_icon, 16, 16, WHITE);
      break;
    case STATE_HUM:
      if(currentHum < 10)
        display.print(" ");
      display.println(currentHum);
      display.print("    %");
      display.drawBitmap(0,display.height()-16, humidity, 16, 16, WHITE);
      break;
    case STATE_BARO:
      int h = (int) currentBaro / (int)100;
      if(h < 10000)
        display.print(" ");
      if(h < 1000)
        display.print(" ");
      if(h < 100)
        display.print(" ");
      if(h < 10)
        display.print(" ");
      display.println(h);
      display.print("  hPa");
      display.drawBitmap(0,display.height()-16, pressure, 16, 16, WHITE);      
      break;
  }
  display.setCursor(29,41);
  display.setTextSize(1);
  display.print(currentVoltage);
  display.print("V");
  display.display();
}

void setup() {
  Serial.begin(9600);

  multi.addAP(ssid1, password1);
  multi.addAP(ssid2, password2);
  multi.addAP(ssid3, password3);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)

  // Clear the buffer.
  display.clearDisplay();
  display.dim(1);
  display.setTextWrap(false);
  // show "welcome" screen
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(14,0);
  display.print("ESP");
  display.setCursor(8,17);
  display.print("8266"); 
  display.setTextSize(1);
  display.setCursor(11,40);
  display.print("Climate");
  display.display();
  delay(2000);

  display.clearDisplay();
  display.setTextWrap(true);
  Serial.println("Connecting Wifi...");
  display.setCursor(0,4);
  display.print("Connecting   Wifi");
  display.display();
  delay(500);

  display.setTextWrap(false);
  int fake = 3;
  while(fake != 0) {
    display.setCursor(23,display.height()-8);
    display.setTextColor(BLACK);
    display.print("...");
    display.display();
    delay(400);
    display.setTextColor(WHITE);
    display.setCursor(23,display.height()-8);
    display.print(".");
    display.display();
    delay(250);
    display.print(".");
    display.display();
    delay(250);
    display.print(".");
    display.display();
    delay(500);
    fake--;
  }
  int c = 0;
  while(multi.run() != WL_CONNECTED || c >= timeout) {
    Serial.println("next wifi connect try in 1s");
    delay(1000);
    c++;
  }
  if(c >= timeout)
    while(1); // WDT reset

  // remove 3 dots (if there are some)
  display.setCursor(23,display.height()-8);
  display.setTextColor(BLACK);
  display.print("...");
  display.setTextColor(WHITE);
  display.setCursor(5,display.height()-8);
  display.print("connected");
  display.display();
  delay(500);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("WiFi SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt.setServer(HOST, PORT);
  mqtt.setCallback(callback);
  
  display.clearDisplay();
  display.setCursor(8,15);
  display.print("fetching");
  display.setCursor(20,24);
  display.print("data");
  display.display();
  delay(1500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("ESP8266-OLED", USERNAME, PASSWORD)) {
      Serial.println("connected");
      // ... and resubscribe
      mqtt.subscribe("weatherStationTest/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop() {
  // make sure we're connected
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  // show the values
  drawValues();
  long now = millis();
  if(lastSwitch == 0)
    lastSwitch = now;
  int waited = now - lastSwitch;
  // time to switch display values?
  if(waited >= WAIT_SECONDS * 1000) {
    lastSwitch = now;
    state++;
    if(state > 2)
      state = 0;
  }
}
