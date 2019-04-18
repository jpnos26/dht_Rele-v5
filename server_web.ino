#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <Arduino.h>
//flag to use from web update to reboot the ESP
   

void setup_Server(){
  Serial.println ("Inizializzo OTA .........\n");
    String hostNAME = wifiName;
    hostNAME += "-";
    hostNAME += String(ESP.getChipId(), HEX);
    Serial.print("set OTA+WiFi hostname: "); Serial.println(hostNAME);Serial.print("\n");
    WiFi.hostname(hostNAME);
    ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
    ArduinoOTA.onEnd([]() { events.send("UpdaWifiStart();te End", "ota"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        char p[32];
        sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
        events.send(p, "ota");
        });
    ArduinoOTA.onError([](ota_error_t error) {
        if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
        else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
        else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
        else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    
    });
  
    ArduinoOTA.setHostname((const char *)hostNAME.c_str());
  
    ArduinoOTA.begin();

    MDNS.addService("http","tcp",80);
  
  
  //////Setto web Server
  Serial.print ("Inizializzo Web Server .........\n");
  
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(httpusername,httppassword));
  
  server.on("/menu", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/menu.json", "text/plain");
  });
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
   server.on("/test_gpio", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Test GPIO ON & OFF-----");
    for (int p=0; p<digitalVer; p++){
      Serial.print("Test gpio; ");Serial.println (rele[0][p].toInt());
      digitalWrite(rele[0][p].toInt(), ON);
      Serial.println (digitalRead(rele[0][p].toInt()));
      digitalWrite(rele[0][p].toInt(), OFF);
      Serial.println(digitalRead(rele[0][p].toInt()));
    }
    thermo_timer = millis();
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
   server.on("/panicStop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Panic Stop   -----");
    for (int p=0; p<digitalVer; p++){
      digitalWrite(rele[0][p].toInt(), OFF);
      Serial.println(digitalRead(rele[0][p].toInt()));
    }
    thermo_timer = millis();
    request->redirect("/index.html");
  });
   server.on("/setRele", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.print ("Page /setRele .........\n");
      int params = request->params();
      int i = 0;
      AsyncWebParameter* p = request->getParam(i);
      String pin =  p->name().c_str();
      int setPin = p->value().toInt();
      Serial.printf("Parametro[%s]: %s\n", p->name().c_str(), p->value().c_str());
      Serial.println(pin+" : " + String(setPin));
      for (int p=0; p<digitalVer; p++){
        Serial.print("loop : ");Serial.println(p);
        if (pin == rele[1][p]){
            Serial.print("zona : ");Serial.println(rele[1][p]);
            if(setPin == 1){
              Serial.print("setPin = 1 - ");Serial.print("Zona Antagonista : ");Serial.print(rele[1][p]);Serial.print(" --> stato : ");Serial.println(digitalRead(rele[2][p].toInt()));
              if (digitalRead(rele[2][p].toInt()) == OFF || rele[2][p].toInt() == 99){
                digitalWrite (rele[0][p].toInt(), ON) ;
                unsigned long addtime = rele[3][p].toInt() * 1000;
                timerOn[p]= millis() + addtime;
                Serial.print(rele[1][p]);Serial.print(" - ");Serial.print(rele[0][p]);Serial.print(" ON - delay : ");Serial.println(addtime);
                }
            }
            else {
              Serial.print("setPin = 0 - ");Serial.print("Zona Antagonista : ");Serial.print(rele[1][p]);Serial.print(" --> stato : ");Serial.println(digitalRead(rele[2][p].toInt()));
              if (digitalRead( rele[2][p].toInt()) == OFF || rele[2][p].toInt() == 99){
                digitalWrite (  rele[0][p].toInt(), OFF) ;
                Serial.print(rele[1][p]);Serial.print(" - ");Serial.print(rele[0][p]);Serial.println(" OFF");
                }
            }
           
        }
      }
      thermo_timer = millis();
    request->redirect("/index.html");
  });
  server.on("/gpio", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        //Serial.println("Creo DigitalVer....."+String(digitalVer));
        json += "\"name\":\"" + wifiName +"\"";
        json += ",\"digitalVer\":" + String(digitalVer);
        json += ",\"version\":\""+thermo_version+"\"";
        Serial.print("lettura Pin : ");Serial.print(digitalRead(rele[0][0].toInt()));Serial.print(" - ON : ");Serial.println(ON);
        if (digitalRead(rele[0][0].toInt()) == ON){
          send_gpio = "1";
           }
        else{
          send_gpio= "0";
        }
        json += ",\"gpio\":["+ send_gpio;
        for (int p=1; p<digitalVer; p++){
            Serial.print("lettura Pin : ");Serial.print(rele[1][p]);Serial.print(" - ");Serial.print(digitalRead(rele[0][p].toInt()));Serial.print(" - ON : ");Serial.println(ON);
            if (digitalRead(rele[0][p].toInt()) == ON){
                send_gpio = "1";
                }
            else{
                send_gpio= "0";
                }
            json+= ","+send_gpio;
            }
        json += "]";
        //Serial.println("Creo zonaPin....");
        json += ",\"zona\":[\""+rele[1][0]+"\"";
        Serial.print("Zona 0 ");Serial.print(rele[1][0]);
        for (int p=1; p<digitalVer; p++){
           json+= ",\""+rele[1][p]+"\"";
          }
        json += "]";
        //Serial.println("Creo timePin....");
        json += ",\"timer\":["+rele[3][0];
        for (int p=1; p<digitalVer; p++){
           json+= ","+rele[3][p];
          }
        json += "]";
        //Serial.println("Creo imgPin....");
        json += ",\"image\":["+rele[4][0];
        for (int p=1; p<digitalVer; p++){
           json+= ","+rele[4][p];
          }
        json += "]}";
        Serial.print ("Page /gpio json data .........\n");Serial.println(json);
        request->send(200, "text/json", json);
        json = String();
        
    });
   server.on("/setAll", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.print ("Page /setAll .........\n");
      int params = request->params();
      /////leggiamo pin
      int i = 0;
      AsyncWebParameter* p = request->getParam(i);
      String pin =  p->name().c_str();
      int setPin = p->value().toInt();
      ///leggiamo stato
      i = 1;
      AsyncWebParameter* s = request->getParam(i);
      String stato =  s->name().c_str();
      int setStato = s->value().toInt();
      Serial.println(pin+" : " + String(setPin) +" - " +stato + ": "+String(setStato));;
      for (int p=0; p<digitalVer; p++){
        if (setStato == 1){
           if (rele[5][p].toInt() == setPin){
              Serial.print("setAll = 1 - ");Serial.print("Zona Antagonista : ");Serial.print(rele[1][p]);Serial.print(" --> stato : ");Serial.println(digitalRead(rele[2][p].toInt()));
              if (digitalRead(rele[2][p].toInt()) == OFF || rele[2][p].toInt() == 99){
                digitalWrite (rele[0][p].toInt(), ON) ;
                unsigned long addtime = rele[3][p].toInt() * 1000;
                timerOn[p]= millis() + addtime;
                }
           }
        }
        else{
           Serial.print("setAll = 0 - ");Serial.print("Zona : ");Serial.print(rele[5][p]);Serial.print(" --> stato : ");Serial.println(setPin);
          if (rele[5][p].toInt() == setPin){
              if (digitalRead(rele[2][p].toInt()) == OFF || rele[2][p].toInt() == 99){
                digitalWrite (rele[0][p].toInt(), OFF) ;
                }
              }
          }
        }
    thermo_timer = millis();
    request->redirect("/index.html");
  });

    server.on("/setNetwork", HTTP_GET, [](AsyncWebServerRequest *request){
    int params = request->params();
    int i = 0;
    AsyncWebParameter* p = request->getParam(i);
    Serial.printf("HEADER[%s]: %s\n", p->name().c_str(), p->value().c_str());
    String salva_network = p->name().c_str();
    String S_filena_WBS = "/settings.json";
    Serial.println("Data saved "+S_filena_WBS+" - "+salva_network);
          File fsUploadFile = SPIFFS.open(S_filena_WBS, "w");
          if (!fsUploadFile){ 
            Serial.println("file open failed");
            }
          else{
            fsUploadFile.println(salva_network); 
            Serial.printf("BodyEnd: %s\n",salva_network.c_str());
          }
          fsUploadFile.close();
    wifi_check = 1;
    request->redirect("/index.html");
  });

  server.on("/setScheda", HTTP_GET, [](AsyncWebServerRequest *request){
    int params = request->params();
    int i = 0;
    AsyncWebParameter* p = request->getParam(i);
    Serial.printf("HEADER[%s]: %s\n", p->name().c_str(), p->value().c_str());
    String salva_rele = p->name().c_str();
    String S_filena_WBS = "/scheda.json";
    Serial.println("Data saved "+S_filena_WBS+" - "+salva_rele);
          File fsUploadFile = SPIFFS.open(S_filena_WBS, "w");
          if (!fsUploadFile){ 
            Serial.println("file open failed");
            }
          else{
            fsUploadFile.println(salva_rele); 
            Serial.printf("BodyEnd: %s\n",salva_rele.c_str());
          }
          fsUploadFile.close();
    wifi_check = 2;
    request->redirect("/index.html");
  });

   server.on("/setMenu", HTTP_GET, [](AsyncWebServerRequest *request){
    int params = request->params();
    int i = 0;
    AsyncWebParameter* p = request->getParam(i);
    Serial.printf("HEADER[%s]: %s\n", p->name().c_str(), p->value().c_str());
    String salva_menu = p->value().c_str();
    String S_filena_WBS = "/menu.json";
    salva_menu.replace("uguale","=");
    salva_menu.replace("ampersand","&");
    Serial.println("Data saved "+S_filena_WBS+" - "+salva_menu);
          File fsUploadFile = SPIFFS.open(S_filena_WBS, "w");
          if (!fsUploadFile){ 
            Serial.println("file open failed");
            }
          else{
            fsUploadFile.println(salva_menu); 
            Serial.printf("BodyEnd: %s\n",salva_menu.c_str());
          }
          fsUploadFile.close();
    thermo_timer = millis();     
    request->redirect("/index.html");
  });
   
   server.on("/Restart", HTTP_GET, [](AsyncWebServerRequest *request){
      ESP.restart();
   });
   
  server.on("/webImage", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Leggo Nomi File Immagini ..........");
    String webImage=loadFile();
    request->send(200, "text/json", webImage);
    request->redirect("/index.html");
  });
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server
    .serveStatic("/", SPIFFS, "/")
    .setDefaultFile("index.html")
    .setCacheControl("max-age=300");
    //.setAuthentication(httpusername, httppassword);

  
   server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
     Serial.print("Parametri");
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
     Serial.print("On File Upload");
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.print("On request Body ");
    if(!index)
     Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  
 
    
  server.begin();

}  
