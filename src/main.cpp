#include "Smarty.h"
#include <ESP8266WebServer.h>

void handleRoot();
void handleOk();
void handleGetData();
void handleReboot();

//Smarty smarty(ESP_BASE_NAME, "Базовая станция");
Smarty smarty("dfgdfgdf", "Базовая станция");

ESP8266WebServer webserver(80);
WiFiServer esp_server(ESP_SERVER_PORT);
WiFiClient esp_client;

const char *www_username PROGMEM = "admin";
const char *www_password PROGMEM = "admin";

String html_header = "<html>\
 <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
 <head>\
   <title>Настройка ESP8266</title>\
   <style>\
     body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
   </style>\
 </head>";

 conn_data_t conn_data;

void setup (void) {
  smarty.begin();
  Serial.begin(115200);

  Serial.print("\nAP started: ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  esp_server.begin();

  webserver.on("/", [](){
    if(!webserver.authenticate(www_username, www_password))
      return webserver.requestAuthentication();
    handleRoot();
  });
  webserver.on("/ok", [](){
    if(!webserver.authenticate(www_username, www_password))
      return webserver.requestAuthentication();
    handleOk();
  });

  webserver.on("/reboot", [](){
    if(!webserver.authenticate(www_username, www_password))
      return webserver.requestAuthentication();
    handleReboot();
  });

  webserver.begin();
  Serial.println("HTTP server started");
}

void loop (void) {
  smarty.checkConnection();

  webserver.handleClient();

  if (!esp_client.connected()) {
    esp_client = esp_server.available();
  }
  else
  {
    Serial.println("Connected " + esp_client.remoteIP().toString() + ". Sending WiFi-data");
    esp_client.write("asasdsdas", 9);
    esp_client.stop();
  }

  yield();
}

void handleRoot() {
  String str = "";
  str += html_header;
  str += "<body><form method=\"POST\" action=\"ok\"> \
       <h1 align=\"center\">Настройка основного узла</h1> \
         <table align=\"center\"><tr><td valign=\"top\">SSID<br><input name=\"ssid\" value=\"";
  str += smarty.getSSID();
  str += "\"><br><br>Пароль (для изменения<br>введите новый)<br><input name=\"pswd\" value=\"\"> \
          <br></td><td>Текущий IP<br><input disabled value=\"";
  str += WiFi.localIP().toString();
  str += "\"><br><br><br>IP сервера<br><input name=\"baseIP\" value=\"";
  str += smarty.getServerIP().toString();
  str += "\"><br><br>Порт сервера<br><input name=\"port\" value=\"";
  str += smarty.getPort();
  str += "\"></td></tr><tr><td colspan=\"2\" align=\"center\"><br><input type=SUBMIT value=\"Сохранить\"></td></tr> \
       </table></form></body></html>";
  webserver.send(200, "text/html", str);
}

void handleOk(){
  String ssid_ap;
  String pass_ap;
  String str_addr;
  uint8_t ip_addr[4];
  uint16_t port;

  unsigned char* buf = new unsigned char[64];

  String str = "";
  str += html_header;
  str += "<body>";

  ssid_ap = webserver.arg(0);
  pass_ap = webserver.arg(1);
  str_addr = webserver.arg(2);
  port = (uint16_t) atoi(webserver.arg(3).c_str());
  if ( !pass_ap.length() )
    pass_ap = smarty.getPASS();

  str_addr.toCharArray((char*) buf, 16);
  ip_addr[0] = atoi(strtok((char*) buf, "."));
  ip_addr[1] = atoi(strtok(NULL, "."));
  ip_addr[2] = atoi(strtok(NULL, "."));
  ip_addr[3] = atoi(strtok(NULL, "."));

  if( ssid_ap.length() > 0 && ssid_ap.length() < 33 && pass_ap.length() < 64 && pass_ap.length() > 7 && ip_addr[3] > 0 && ip_addr[3] < 255 )
  {
    strcpy(conn_data.ssid, ssid_ap.c_str());
    strcpy(conn_data.pass, pass_ap.c_str());
    conn_data.serverIP[0] = ip_addr[0];
    conn_data.serverIP[1] = ip_addr[1];
    conn_data.serverIP[2] = ip_addr[2];
    conn_data.serverIP[3] = ip_addr[3];
    conn_data.port = port;

    smarty.EEPROM_write(&conn_data);

    str +="Конфигурация сохранена в EEPROM<br>\
    Изменения будут приняты после перезагрузки</p><br>";
  }
  else
    str += "Проверьте правильность ввода данных<br><br>";
  str += "<a href=\"/\">Назад</a> к настройкам<br></body></html>";
  webserver.send(200, "text/html", str);
}

void handleReboot(){
  String str = "";
  str += html_header;
  webserver.send(200, "text/html", str);
  delay(2000);
  ESP.restart();
}