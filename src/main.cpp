#include "Arduino.h"

// Sensors
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// NTP client
#include <TimeLib.h>
#include <NtpClientLib.h>



// OTA updates
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

//Web server
#include <ESP8266WebServer.h>

// Average calcutation
#include "RunningAverage.h"


#include "config.h"

#define ArraySize 25
#define CacheSize 25
#define CaptureInterval 300
#define CacheInterval 20

#define PressureOffset 9.85


float TempList[ArraySize];
float HumidityList[ArraySize];
String TimeList[ArraySize];
float PressureList[ArraySize];
RunningAverage TempAvg(CacheSize);
RunningAverage HumAvg(CacheSize);
RunningAverage PressureAvg(CacheSize);

unsigned long timeNow = 0;

unsigned long timeLastStored = 0;
unsigned long timeLastCached = 0;

int myindex=0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP280 bmp; // I2C


ESP8266WebServer server(80);

String s2="";

// Please create a config/config.h in the lib folder with the following lines
//
//const char* ssid = "<ssid>";
//const char* password = "<password>";

void store() {
    timeNow = millis()/1000;
    if (timeNow - timeLastCached > CacheInterval)
    {
        TempAvg.addValue(sht31.readTemperature());
        HumAvg.addValue(sht31.readHumidity());
        PressureAvg.addValue((bmp.readPressure() / 100) + PressureOffset);


        timeLastCached = timeNow;
    }
    if (timeNow - timeLastStored > CaptureInterval)
    {
        myindex++;
        if (myindex == ArraySize)
        {
            myindex=0;
        }


        TempList[myindex] = TempAvg.getAverage();
        HumidityList[myindex] = HumAvg.getAverage();
        TimeList[myindex] = NTP.getTimeStr();
        PressureList[myindex] = PressureAvg.getAverage();

        timeLastStored = timeNow;
    }
}

String dump() {
    String debugstring="";
    int j = myindex + 1;
    for (int i=j ; i <= j + ArraySize - 1; i++)
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
    page +=           TempAvg.getAverage();
    page +=           "</span> Temp&eacute;rature</a>";
    page +=         "</li><li>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           HumAvg.getAverage();
    page +=           "</span> Humidit&eacute;</a>";
    page +=         "</li><li>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           PressureAvg.getAverage();
    page +=           "</span> Pression atmosph&eacute;rique</a></li>";
    page +=         "</li><li>";
    page +=           "<a href='#'> <span class='badge pull-right'>";
    page +=           NTP.getTimeDateString(NTP.getLastNTPSync());
    page +=           "</span> NTP last sync</a></li>";
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

  NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
      if (error) {
          Serial.print("Time Sync error: ");
          if (error == noResponse)
              Serial.println("NTP server not reachable");
          else if (error == invalidAddress)
              Serial.println("Invalid NTP server address");
      }
      else {
          Serial.print("Got NTP time: ");
          Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
      }
  });
  NTP.begin("fr.pool.ntp.org", 1, true);
  NTP.setInterval(500);

}


void loop() {

    delay(1000);
    ArduinoOTA.handle();
    store();
    server.handleClient();
}
