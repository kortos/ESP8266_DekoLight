#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define PIN            2
#define NUMPIXELS      8

unsigned long patternInterval = 20 ; // time between steps in the pattern
unsigned long lastUpdate = 0 ; // for millis() when last update occoured
unsigned long intervals [] = { 20, 20, 50, 100 } ; // speed for each pattern
unsigned long interval_wipe = 100;
unsigned long interval_fade = 100;
unsigned long interval_rainbow = 20;

const char* ssid = "WLAN_SSID";
const char* password = "WLAN_PWD";
const char* mqtt_server = "192.168.2.90";
String CurrentColorMode = "wipe_red";
String LightId = "03";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

void setup_wifi() {
   delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  randomSeed(micros());
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void MQTTCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(" ");
  
  String ColorName = "";
  
  for (int i = 0; i < length; i++) { ColorName = ColorName + (char)payload[i]; }
  
  Serial.println(ColorName);
  
  CurrentColorMode = ColorName;
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "LedDekoLight";
    clientId += LightId;
    clientId += "-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      String CurrentLightNode = "LedDekoLights/Light" + LightId + "/colorMode";
 
      client.subscribe(CurrentLightNode.c_str());
      client.subscribe("LedDekoLights/allColorMode");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  strip.begin(); // Initialisierung der NeoPixel
  strip.setBrightness(255);

  Serial.begin(115200);
  colorWipe(strip.Color(0, 255, 0));
   
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);

  reconnect();
  
  client.setCallback(MQTTCallback);  
}

void colorWipe(uint32_t c) { // modified from Adafruit example to make it a state machine
  static int i =0;
    strip.setPixelColor(i, c);
    strip.show();
  i++;
  if(i >= strip.numPixels()){
    i = 0;
    wipe(); // blank out strip
  }
  lastUpdate = millis(); // time for next change to the display
}

void wipe(){ // clear all LEDs
     for(int i=0;i<strip.numPixels();i++){
       strip.setPixelColor(i, strip.Color(0,0,0));
       }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow() { // modified from Adafruit example to make it a state machine
  static uint16_t j=0;
    for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
     j++;
  if(j >= 256) j=0;
  lastUpdate = millis(); // time for next change to the display
}

void rainbowCycle() { // modified from Adafruit example to make it a state machine
  static uint16_t j=0;
    for(int i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
  j++;
  if(j >= 256*5) j=0;
  lastUpdate = millis(); // time for next change to the display
}

void SetStripColor( String ColorName ) 
{
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    if (ColorName.equals("red")) { strip.setPixelColor(i, strip.Color(255,0,0)); }
    if (ColorName.equals("green")) { strip.setPixelColor(i, strip.Color(0,255,0)); }
    if (ColorName.equals("blue")) { strip.setPixelColor(i, strip.Color(0,0,255)); }
    if (ColorName.equals("yellow")) { strip.setPixelColor(i, strip.Color(255,255,0)); }
    if (ColorName.equals("white")) { strip.setPixelColor(i, strip.Color(255,255,255)); }
    if (ColorName.equals("orange")) { strip.setPixelColor(i, strip.Color(255,165,0)); }
    if (ColorName.equals("pink")) { strip.setPixelColor(i, strip.Color(248,24,148)); }    
    if (ColorName.equals("off")) { strip.setPixelColor(i, strip.Color(0,0,0)); }
  }
  
  strip.show();
  lastUpdate = millis();
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }

  patternInterval = 20;
  
  // Intervall für verschiedene ColorModes einstellen
  if ( CurrentColorMode.equals("wipe_red") || CurrentColorMode.equals("wipe_blue") || CurrentColorMode.equals("wipe_green") || CurrentColorMode.equals("wipe_white") ) {
    patternInterval = interval_wipe;
  }

 if ( CurrentColorMode.equals("fade_red") || CurrentColorMode.equals("fade_blue") || CurrentColorMode.equals("fade_green") || CurrentColorMode.equals("fade_white") ) {
    patternInterval = interval_fade;
  }  
  
  client.loop();

  if(millis() - lastUpdate > patternInterval)
  {
    if ( CurrentColorMode.equals("rainbow") ) 
    {
      rainbow();
    } 
    else if ( CurrentColorMode.equals("rainbowcycle") ) 
    {
      rainbowCycle();
    }
    else if ( CurrentColorMode.equals("wipe_red") ) 
    {
      colorWipe(strip.Color(255, 0, 0));
    }
    else if ( CurrentColorMode.equals("wipe_green") ) 
    {
      colorWipe(strip.Color(0, 255, 0));
    }    
    else if ( CurrentColorMode.equals("wipe_blue") ) 
    {
      colorWipe(strip.Color(0, 0, 255));
    }      
    else if ( CurrentColorMode.equals("wipe_white") ) 
    {
      colorWipe(strip.Color(255, 255, 255));
    }          
    else 
    {
      SetStripColor( CurrentColorMode );     
    }
  }  
}
