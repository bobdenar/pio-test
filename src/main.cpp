#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <NTPClient.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
// #include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>


#include "config.h"

#define ArraySize 25

float TempList[ArraySize];
float Temp2List[ArraySize];
float HumidityList[ArraySize];
String TimeList[ArraySize];
float PressureList[ArraySize];

unsigned long timeNow = 0;

unsigned long timeLast = 0;

int myindex=0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP280 bmp; // I2C

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

ESP8266WebServer server(80);

String s2="";

// Please create a config/config.h in the lib folder with the following lines
//
//const char* ssid = "<ssid>";
//const char* password = "<password>";

void store() {
    timeNow = millis()/1000;
    if (timeNow - timeLast > 120 || timeNow < 60)
    {
        timeClient.update();
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();
        float t2 = bmp.readTemperature();
        float p = bmp.readPressure() / 100;
        String debugstring = timeClient.getFormattedTime() + " " + String(t) + " " + String(t2) + " " + String(h) + " " + String(p) + " " + String(myindex % ArraySize);
        Serial.println(debugstring);

        TempList[myindex % ArraySize] = t;
        Temp2List[myindex % ArraySize] = t2;
        HumidityList[myindex % ArraySize] = h;
        TimeList[myindex % ArraySize] = timeClient.getFormattedTime();
        PressureList[myindex % ArraySize] = p;
        myindex++;
        timeLast = timeNow;
    }
}

String dump() {
    String debugstring="";
    for (int i=myindex ; i <= myindex + ArraySize - 1; i++)
    {
        debugstring += "<tr><td>" + TimeList[i % ArraySize]+ "</td><td>" + String(TempList[i % ArraySize], 1) + "</td><td>" + String(HumidityList[i % ArraySize], 0) +"</td><td>" + String(PressureList[i % ArraySize], 1) + "</tr>";
        //Serial.println(debugstring);

    }
    return debugstring;
}

String getPage()  {
    String page = "<html lang='fr'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/>";
    page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
    page += "<title>ESP8266 Demo</title></head><body>";
    page += "<div class='container-fluid'>";
    page +=   "<div class='row'>";
    page +=     "<div class='col-md-12'>";
    page +=       "<h1>Demo Webserver ESP8266 + Bootstrap</h1>";
    page +=       "<h3>Mini station m&eacute;t&eacute;o</h3>";
    page +=       "<ul class='nav nav-pills'>";
    page +=         "<li class='active'>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           "t";
    page +=           "</span> Temp&eacute;rature</a>";
    page +=         "</li><li>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           "h";
    page +=           "</span> Humidit&eacute;</a>";
    page +=         "</li><li>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           "p";
    page +=           "</span> Pression atmosph&eacute;rique</a></li>";
    page +=       "</ul>";
    page +=       "<table class='table table-condensed'>";  // Tableau des relevés
    page +=         "<thead><tr><th>Time</th><th>Temp1</th><th>Hum</th><th>Pression</th></tr></thead>"; //Entête
    page +=         "<tbody>";  // Contenu du tableau
    page +=         dump();
    page +=       "</tbody></table>";
    page += "</div></div></div>";
    page += "</body></html>";
    return page;
}



void handleRoot(){
    server.send ( 200, "text/html", getPage() );
}




void setup() {

  Serial.begin(9600);

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.println(F("BMP280 test"));

  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);

  }



  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // On branche la fonction qui gère la premiere page / link to the function that manage launch page
  server.on ( "/", handleRoot );

  server.begin();
  Serial.println ( "HTTP server started" );


}


void loop() {

    delay(1000);
    ArduinoOTA.handle();
    server.handleClient();
    store();
}
