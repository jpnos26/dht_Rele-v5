///// Thermostat  Rele   /////////
// Copyright (c) 2016 Jpnos <jpnos@gmx.com>

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// Gestione di rele tramite wifi                                
// Attivata interfaccia per Thermostat                               
// versione 5.0.1 prima versione                               
// 5.0.2 aggiunte pagine di servizio /edit per editare lo spiffs
//       e /update per  update firmware. Modificata la index diventata dinamica
//       implementato la ricarica automatica dei dati senza reboot

///////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <Arduino.h>
#include <SPIFFSEditor.h>


// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

///Varibile versione del Sistema
String thermo_version = "5.0.2 Nov 2018";

///Inizializzo gestione PIN
int ON = 0;                                                     /// cosi settiamo On oppure off  con opto oppure no ......               
int OFF = 1;

//// parte dedicata ad accesso a Thermostat
String send_gpio;                                               /// stringa con i dati per Thermostat
unsigned long thermo_timer;                                     /// Timer per mandare dati a Thermostat
String thermostat_ip = "192.168.1.117";                         /// ip del Thermostat da raggiungere for slave mode
byte thermo_on   = 1;                                           /// abilito uso di thermostat 0 = 0ff , 1 = ON


///Variabile Reboot da web
bool shouldReboot = true;

///Variabili di Servizio
int wifi_check = 0;                                             // variabile check connessione wifi
byte wifi_change = 1; 
int check_wifi = 0;                                             /// test per il wifi in ap se rimane troppo reboot
unsigned int digitalVer = 4;                                    /// set numero di pin utilizzati
String rele [][8]= {{"12","sala su","13","10","1","0"},   ///{ Pin usato,nome zona,pin antagonista,timer,image,type(0 su , 1 giu oppure 0 accendo e 1 spengo ricordarsi che se 0 quando accendo tutti li porto a on e quando spengo tutti porto a on tutti gli 1}
                    {"13","sala giu","12","10","1","1"}, 
                    {"14","cucina su","15","10","1","0"}, 
                    {"15","cucina giu","14","10","1","1"}, 
                    {"15","sala","14","10","1","0"},
                    {"12","sala","13","10","1","0"}, 
                    {"12","sala","13","10","1","0"}, 
                    {"12","sala","13","10","1","0"}} ;

unsigned long timerOn[10]= {0,0,0,0,0,0,0,0};


///Wifi Configuration
const char* wifissid = "wifi ssid";
const char* wifipassword = "wifi pwd";
String wifiName = "DhtRele";
const char* httpusername = "admin";
const char* httppassword = "admin";
const char* apssid = "DhtRele";
const char* appassword = "dhtrele";
unsigned long ulNextWifiCheck;      // siamo in ap check wifi
int dhcp = 0; ////0 dhcp disable, 1 dhcp enabled
char static_ip[16] = "192.168.1.120";
char static_gateway[16] = "192.168.1.1";
char static_netmask[16] = "255.255.255.0";
//IPAddress static_ip(192,168,1,120);
//IPAddress gateway(192,168,1,1);
//IPAddress netmask(255,255,255,0);
//IPAddress dns(192,168,1,1);
IPAddress ip;
IPAddress gateway;
IPAddress netmask;
IPAddress dns(8,8,8,8);

// needed to avoid link error on ram check
extern "C" 
{
#include "user_interface.h"
}
 

void setup() {
    ////Init Serial
    Serial.begin(115200);
  	Serial.setDebugOutput(true);
  	Serial.println("Esp8266 WI-FI Rele Jpnos-2017 v1.0");
   
    ///inizializzo spiffs
    Serial.print ("Inizializzo SPIFFS .........\n");
    SPIFFS.begin();
    
    
    Serial.print ("Inizializzo Dati .........\n");
    CaricaDati();
    
    Serial.print ("Inizializzo Wifi .........\n");
    WiFiStart(); ////Start Wifi
    
  
    Serial.print ("Inizializzo Scheda Rele .........\n");
    loadData();
    
    delay(500);
    Serial.print ("Inizializzo Pin .........\n");
     /////Inizializzo Pin
    for (int p=0; p<digitalVer; p++){
      Serial.println (rele[0][p].toInt());
      pinMode(rele[0][p].toInt(),OUTPUT); ///Set Pin to Output
      digitalWrite(rele[0][p].toInt(), OFF);
    } 
    
    Serial.print ("Inizializzo Server WEb .........\n");
    setup_Server();  

    Serial.print ("Inizializzo Immagini Web .........\n");
    loadFile();
}

/// routine inizializzazione Pin
void initPin(){
    for (int p=0; p<digitalVer; p++){
      Serial.println (rele[0][p].toInt());
      pinMode(rele[0][p].toInt(),OUTPUT); ///Set Pin to Output
      digitalWrite(rele[0][p].toInt(), OFF);
    }
   }
////// write web data su thermostat
void  writeWebData(){
    Serial.print("scrivo dati su Thermostat: " );Serial.println(thermostat_ip);
    String web_gpio = "";
    String web_zona = "";
    String web_img = "";
    for (int p=0; p<digitalVer; p++){
        if (digitalRead(rele[0][p].toInt()) == ON){
            web_gpio += "1";
            }
        else{
            web_gpio += "0";
            }
        web_gpio +=",";
        web_zona +=rele[1][p]+",";
        web_img += rele[4][p] +",";
        }
        web_gpio = web_gpio.substring(0, web_gpio.length() - 1);
        web_zona = web_zona.substring(0, web_zona.length() - 1);
        web_img = web_img.substring(0, web_img.length() - 1);
      String url = "http://"+thermostat_ip+":9090/?"+String(ESP.getChipId(), HEX)+"&"+wifiName+"&"+WiFi.localIP().toString()+"&"+String(digitalVer)+"&"+web_gpio+"&"+web_img+"&"+web_zona+"&&&&RELE";
      url.replace(" ","%20");
      Serial.print("Requesting URL: ");Serial.println(url);
      HTTPClient http;
      
      http.begin(url);
      http.addHeader("Content-Type", "text/html");
      int httpCode = http.GET();
      if (httpCode == 200) {
          Serial.println("Dati Trasmessi....");
        }
        else{
          Serial.print("Errore...: ");Serial.println(httpCode);
        }
    http.end();

    // Read all the lines of the reply from server and print them to Serial
    Serial.print ("Heap Memory: ");Serial.println(String(ESP.getFreeHeap()));
    Serial.println("closing connection");
    thermo_timer = millis()+15000; 
    
}



void loop() {
    /* add main program code here */

    // DO NOT REMOVE. Attend OTA update from Arduino IDE
   ArduinoOTA.handle();
   
   //////////////////////////////
  // check if WLAN is connected
  //////////////////////////////
  if (millis() >= ulNextWifiCheck){
  	if (WiFi.status() != WL_CONNECTED){
  	WiFiStart();
   }
  }
  if (thermo_on == 1){
    if (millis() >= thermo_timer){
      writeWebData();
      }
  }

  if (wifi_check == 1){
    ///ricarico rete e wifi
    ricarica_rete(); 
  }
  else if ( wifi_check == 2 ){
    /// ricarico rele
    ricarica_rele();
  }

  //  }
   
    for (int p=0; p<digitalVer; p++){	
      if (rele[3][p].toInt() != 99){
        if (millis() >= timerOn[p]){  
         digitalWrite (rele[0][p].toInt(), OFF) ;
         //Serial.print(rele[3][p].toInt());Serial.println(" - OFF");
        }   
       }
    }
}

///// Initialize the async web
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
