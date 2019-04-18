#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <SPIFFSEditor.h>

void WiFiStart(){ 
        //WiFi.mode(WIFI_OFF);
        //WiFi.disconnect(true);
        //WiFi.softAPdisconnect(false);
        //yield();
        //delay(100);
        WiFi.setAutoConnect(false);
        WiFi.setAutoReconnect(false);
        Serial.print("Connessione con :"); Serial.print(wifissid);Serial.print(" - ");Serial.print(wifipassword);Serial.print("\n");
        WiFi.mode(WIFI_STA);
        if (dhcp == 0){
            WiFi.config(ip,gateway,netmask,gateway);
            delay(50);
          }
        WiFi.begin(wifissid, wifipassword);
        delay(50);
        WiFi.printDiag(Serial);
        for ( int count = 0; count < 20 ; count++ )  {
            Serial.print( count ); Serial.print( "." );Serial.print( WiFi.status() );Serial.print("-");
            if (WiFi.status()  == WL_CONNECTED ) {
              break;
            }
            delay(500);
        }
        /*if (WiFi.waitForConnectResult()  == WL_CONNECTED) {
            Serial.println("");
            Serial.print("WiFi connessa --> ");Serial.println(WiFi.localIP());
            ulNextWifiCheck = millis()+500000;
            }*/
        if (WiFi.status() == WL_CONNECTED ) {
              ulNextWifiCheck = millis()+30000;
              Serial.println("Connection ready.........");
            }
        else{
           Serial.printf("\r\nWiFi connect aborted !\r\n");
           delay(50);
           WiFi.mode(WIFI_AP);
           WiFi.hostname(wifiName);
           WiFi.softAP(httpusername, httppassword);
           delay(150);
           Serial.print("Access Point : ");Serial.print(wifiName);Serial.print(" - address ");Serial.println(WiFi.softAPIP());
           check_wifi +=1;
           if (check_wifi >= 3){
                delay(100);
                Serial.println("Tentativi reconnect esuriti REBOOT");
                ESP.restart();
                }
           ulNextWifiCheck = millis()+300000;
        }
       if (wifi_change == 1){
        ws.enable(true);
        wifi_change = 0; 
       }
}

void CaricaDati(){
//// controllo se esiste file di configurazione
     File  carica_setting = SPIFFS.open("/settings.json", "r");
    if (!carica_setting) {
      Serial.println(" ");
      Serial.println("Non riesco a leggere settings.json! ...");
      }
    else{
      String risultato = carica_setting.readStringUntil('\n');
      const size_t bufferSize = 3*JSON_ARRAY_SIZE(1) + 7*JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + 350;
      char json[bufferSize];
      risultato.toCharArray(json, bufferSize);
      Serial.print("Array json convertito: ");Serial.println(json);Serial.print("Num char: ");Serial.println(bufferSize);
      DynamicJsonBuffer jsonBuffer(bufferSize);
      //StaticJsonBuffer<650> jsonBuffer_inlettura;
      JsonObject& root = jsonBuffer.parseObject(json);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        }
      else{
        JsonObject& rete0 = root["rete"][0];
        wifissid = rete0["ssid"].as<const char*>();
        wifipassword = rete0["pwd"].as<const char*>();
        dhcp = rete0["dhcp"].as<int>();
        JsonArray& rete0_ip = rete0["ip"];
        String test= rete0_ip[0].as<String>()+"."+rete0_ip[1].as<String>()+"."+rete0_ip[2].as<String>()+"."+rete0_ip[3].as<String>();
        Serial.print(test);Serial.print(" - ");Serial.println(static_ip);
        test.toCharArray(static_ip, 16);
        JsonArray& rete0_gateway = rete0["gateway"];
        test= rete0_gateway[0].as<String>()+"."+rete0_gateway[1].as<String>()+"."+rete0_gateway[2].as<String>()+"."+rete0_gateway[3].as<String>();
        Serial.print(test);Serial.print(" - ");Serial.println(static_gateway);
        test.toCharArray(static_gateway, 16);
        JsonArray& rete0_netmask = rete0["netmask"];
        test= rete0_netmask[0].as<String>()+"."+rete0_netmask[1].as<String>()+"."+rete0_netmask[2].as<String>()+"."+rete0_netmask[3].as<String>();
        Serial.print(test);Serial.print(" - ");Serial.println(static_netmask);
        test.toCharArray(static_netmask, 16);
        //IPAddress netmask(rete0_netmask[0],rete0_netmask[1],rete0_netmask[2],rete0_netmask[3]);
        JsonObject& ap0 = root["ap"][0];
        wifiName = ap0["hostname"].as<const char*>();
        httpusername = ap0["http_username"].as<const char*>();
        httppassword = ap0["http_password"].as<const char*>();
        apssid = ap0["ap_ssid"].as<const char*>();
        appassword = ap0["ap_password"].as<const char*>();
        JsonObject& thermo0 = root["thermo"][0];
        thermostat_ip = thermo0["ip"][0].as<String>()+"."+thermo0["ip"][1].as<String>()+"."+thermo0["ip"][2].as<String>()+"."+thermo0["ip"][3].as<String>();
        thermo_on = thermo0["on"].as<int>();
        
        ////Serial.print(wifihostName);Serial.print(httpusername);Serial.print(httppassword);
        ////Serial.print(ip);Serial.print(" - ");Serial.print(subnet);Serial.println("  - Dati accesso ap");
        delay(100);
        }
     carica_setting.close();
     }
    delay(100);
    
    //set static ip
    
    ip.fromString(static_ip);
    gateway.fromString(static_gateway);
    netmask.fromString(static_netmask);
     
    Serial.print("Letto  Ip Fisso: ");Serial.print(ip);Serial.print(" - ");Serial.print(gateway);Serial.print(" - ");Serial.println(netmask);
     ////Inizializzo Wifi
    
    String hostNAME = wifiName;
    hostNAME += "-";
    hostNAME += String(ESP.getChipId(), HEX);
    Serial.print("set OTA+WiFi hostname: "); Serial.println(hostNAME);
    WiFi.hostname(hostNAME);
}
void loadData(){
    File  carica_scheda = SPIFFS.open("/scheda.json", "r");
     if (!carica_scheda) {
      Serial.println(" ");
      Serial.println("Non riesco a leggere scheda.json! ...");
      }
    else{
      String risultato = carica_scheda.readStringUntil('\n');
      //const size_t bufferSize = 4*JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(5) + 160;
      char json[1000];
      risultato.toCharArray(json, 1000);
      Serial.print("Array json convertito: ");Serial.println(json);
      //DynamicJsonBuffer jsonBuffer(bufferSize);
      StaticJsonBuffer<1000> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(json);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        }
      else{
        digitalVer = root["digitalver"].as <unsigned int>();
        Serial.print("Numero loop :");Serial.println(digitalVer);
        //JsonArray& root_data = root["data"];
        JsonArray& root_data = root["data"];
        for(int i = 0; i < digitalVer; i++)
          {
           JsonArray& root_rele = root_data[i]["rele"];
           for (int a = 0 ;a <6; a++){
              rele[a][i] = root_rele[a].as <String>();
              delay(20);
              Serial.print(a);Serial.print(" - ");Serial.print(i);Serial.print(" --> ");Serial.print(rele[a][i]);Serial.print(" ,");
              }
          Serial.print("\n");
          }
        }
      } 
    carica_scheda.close();
    }


void ricarica_rete(){
        Serial.println("Sto ricaricando il Sistema ..........");
        CaricaDati();
        delay(100);
        WiFi.mode(WIFI_OFF);
        delay(50);
        ulNextWifiCheck = millis()+5000;
        wifi_check = 0;
        wifi_change = 1;
        delay(1000);
        if ( ws.enabled() )
          ws.enable(false);        
}

void ricarica_rele(){
  Serial.println("Ricarica Rele......");
  loadData();
  delay(100);
  initPin();
  delay(100);
  loadFile();
  delay(100);
  wifi_check = 0;
  thermo_timer = millis();
}

String loadFile(){
    String webImage = "";String strdata[2][10];int inde;int index;
    Dir openweb = SPIFFS.openDir("/img");
    while (openweb.next()) {
        if(isDigit(openweb.fileName().charAt(6))){
           //Serial.print("leggo ---->");Serial.print(dir.fileName().substring(5,6));Serial.print(" - ");Serial.println(dir.fileName().substring(6,7));
           index = openweb.fileName().substring(5,6).toInt();
           inde = openweb.fileName().substring(6,7).toInt();
           //Serial.print("scrivo ---->");Serial.print(index);Serial.print(" - ");Serial.print(inde);Serial.print(" --> ");Serial.println(dir.fileName());
           if (inde == 1){
            strdata[1][index] = openweb.fileName();
           }
           else if (inde == 0){ 
            strdata[0][index] = openweb.fileName();
           }
        }
    }
        delay(100);
        for (int c = 0 ;c <10;c++){
          //Serial.print("strdata ");Serial.print(c);Serial.print(strdata[0][c]);Serial.print(" - ");Serial.println(strdata[1][c]);
          webImage += strdata[0][c]+",";
          webImage += strdata[1][c]+",";
        }
     webImage = webImage.substring(0, webImage.length() - 1);
     return(webImage);
}
