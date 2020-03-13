/*
  This is a simple example show the Heltec.LoRa recived data in OLED.

  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connecthin:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series

  3/2020: Edited to combine with DHT sensor code
  3/11/2020: Edited to combine with web server example code
*/
#include "heltec.h" 
#include "images.h"
#include "DHTesp.h"
#include "Ticker.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

/** Heltec LoRa global vars **/
String rssi = "RSSI --";
String packSize = "--";
String packet;

/*** DHT Vars***/
DHTesp dht; //dht struct for heat index and humidity calculations
ComfortState cf;

/** Web server vars**/
const char *ssid = "TsudaLan4";
const char *password = "6469766164";

WebServer server(80);

const int led = 13;

/** new vars **/
TempAndHumidity* receivedVars;
TempAndHumidity stableVars;

/** Functions **/

/**********************************
 *  logo()
 *  displays logo
 *  Origin: Heltec code
 **********************************/
void logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,logo_width,logo_height,logo_bits);
  Heltec.display->display();
}

/*********************************
 * initRecdVars()
 * 
 ********************************/
 void initRecdVars(){
  //receivedVars = new TempAndHumidity;
  stableVars.temperature = -6.6;
  stableVars.humidity = -4.4;
  receivedVars = &stableVars;
 }

/**********************************************
 *  LoRaData()
 *  Displays packet size and contents
 * Called in cbk()
 * Origin: Heltec Code
 *********************************************/
void LoRaData(){
  //calculate heat index, dew point, and ~comfort status~ from rStruct
  float heatIndex = dht.computeHeatIndex(receivedVars->temperature, receivedVars->humidity);
  float dewPoint = dht.computeDewPoint(receivedVars->temperature, receivedVars->humidity);
  float cr = dht.getComfortRatio(cf, receivedVars->temperature, receivedVars->humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };

   //print to serial port
  Serial.println(" T:" + String(receivedVars->temperature) + " H:" + String(receivedVars->humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  //LoRa.print(" T:" + String(receivedVars->temperature) + " H:" + String(receivedVars->humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  
  // our text to display DHT data on Heltech display
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Heltec.display->drawString(0, 0, "Current Temperature: " + String(receivedVars->temperature));
  Heltec.display->drawString(0, 10, "Relative Humidity  : " + String(receivedVars->humidity));
  Heltec.display->drawString(0, 20, "Heat Index         : " + String(heatIndex));
  Heltec.display->drawString(0, 30, "Dew Point          : " + String(dewPoint));
  Heltec.display->drawString(0, 40, comfortStatus);
  
  //Heltec.display->drawString(90, 0, String(counter));
  Heltec.display->display();
  
 /*display code for Heltec OLED (from OG receiver code)
  Heltec.display->clear(); 
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 15 , "Received "+ packSize + " bytes");
  Heltec.display->drawStringMaxWidth(0 , 26 , 128, packet);
  Heltec.display->drawString(0, 0, rssi);  
  Heltec.display->display();
  */
}

/***************************************************************** 
 * cbk(packetSize)
 * Parses packet to get packet size, reads packet into buffer, 
 * calls LoRaData()
 ****************************************************************/
void cbk(int packetSize) {

  Serial.println("In CBK...");

  //TempAndHumidity* receivedVars;
  static uint8_t buffer[sizeof(TempAndHumidity)];
  
  //packet ="";
  //packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++) { 
    buffer[i] = (uint8_t) LoRa.read(); 
    //Serial.print(String(buffer[i],HEX) + " "); test printing
  }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;

  receivedVars = (TempAndHumidity *) buffer;

  //test printing
  Serial.println(" T:" + String(receivedVars->temperature) + " H:" + String(receivedVars->humidity));
  LoRaData();
}

/********************************************************************
 * handleRoot()
 * Displays content for web page
 * Called in setup()
 * Origin: Web Server code
 * *****************************************************************/
void handleRoot() {
  digitalWrite(led, 1);
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Temperature Readings from DHT</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Check out these temps y'all!</h1>\
    <p>Temperature: %02f Humidity: %02f</p>\
  </body>\
</html>",

           receivedVars->temperature, receivedVars->humidity
          );
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

/**************************************************************
 * handleNotFound()
 * Error handling
 * Origin: Web Server code
 *************************************************************/
void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

/*******************************************************************
 *  Arduino setup() function
 *  
 * Merged code from all 3 sources
 ******************************************************************/
void setup() { 

  initRecdVars();
   //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  logo();
  delay(1500);
  Heltec.display->clear();
  
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  Heltec.display->display();


/***server code: setup server****/
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
/** Receive data **/
  delay(1000);
  //LoRa.onReceive(cbk);
  LoRa.receive();

  /***Server code: update page ***/

  server.on("/", handleRoot);
  //server.on("/test.svg", drawGraph); // call draw graph
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
}

/***************************************************************
 * Arduino loop() function
 * Merged from Heltech receiver code and web server code
 **************************************************************/
void loop() {
  
  //Serial.println("In loop..."); // TEST PRINTING
  
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) { 
    Serial.print("Packet size: ");
    Serial.println(packetSize);
    cbk(packetSize);     
  }
  
  server.handleClient(); // web server loop call
  delay(10);
  

}
