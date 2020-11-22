#include <WiFiManager.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
  
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 25

#include <sstream>

#include <ArduinoJson.h>
StaticJsonDocument<250> jsonDocument;
char buffer[250];

 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiManager wm;
const char* ssid = "test";
const char* password = "cegenredemdp";
AsyncWebServer server(80);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int LED = 2;

String tempVoulu = "25";
int temp = 0;
String heurePompe = "600";
String etatChauff = "";

int compteur15 = 900;
unsigned int compteur = 0;

const char* PARAM_INPUT_1 = "temp";
const char* PARAM_INPUT_2 = "heure";

const char index_html[] PROGMEM = R"rawliteral(
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Aquarium Connecté</title>
    <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>
    <style>
        #corp {
            padding-left: 20%;
            padding-right: 20%;
        }
        h1 {
            text-align: center;
        }
        select {
            width: 50px!important;
        }
    </style>
</head>
<body>
    <div id="translator_2894"><noscript>Javascript is disable - <a href="http://www.supportduweb.com/">http://www.supportduweb.com/</a> - <a href="http://www.supportduweb.com/generateur-boutons-traduction-translation-google-gratuit-html-code=script-boutons-traduire-page-web.html">Générateur de boutons de traduction</a></noscript></div><script type="text/javascript" src="http://services.supportduweb.com/translator/2894-2-ynnynnnnn.js"></script>    

    <div class='w3-card w3-blue w3-padding-small w3-jumbo w3-center'>
        <h1>AQUARIUM CONNECTE</h1>
    </div>
    <br>
    <div id='corp'>
        <form action='/get'>
            <p>Changer la température : <input type='number' name='temp' id='temp' style='width:75px;'></p>
            <br>
            <p>Etat de la pompe: </p>
            <p>Activer la pompe (15 minutes) tout les 
                <select class='w3-select' name='heure' id='heure'>
                    <option value='600'>10</option>
                    <option value='1200'>20</option>
                    <option value='1800'>30</option>
                    <option value='2400'>40</option>
                    <option value='3000'>50</option>
                    <option value='3600'>60</option>
                    <option value='4200'>70</option>
                    <option value='4800'>80</option>
                    <option value='5400'>90</option>
                </select>
             minutes</p>
             <button class='w3-btn w3-blue w3-block'>Appliquer</button>
        </form>
        
    </div>
</body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void create_json(char *tag, String value, char *unit) { 
  jsonDocument.clear(); 
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
  Serial.println("Buffer:");
  Serial.println(buffer);  
}

void setup_routing() {
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get temperature");
    create_json("temp", String(temp), "°C");
    request->send_P(200, "application/json", buffer);
  });
  server.on("/tempVoulu", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get temperatureVoulu");
    create_json("tempVoulu", String(tempVoulu), "°C");
    request->send_P(200, "application/json", buffer);
  });
  server.on("/etatPompe", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get etatPompe");
    create_json("etatPompe", String(etatChauff), "");
    request->send_P(200, "application/json", buffer);
  });
  server.on("/heurePompe", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get heurePompe");
    create_json("heurePompe", String(heurePompe), "min");
    request->send_P(200, "application/json", buffer);
  });
  server.on("/restPompe", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get restPompe");
    create_json("restPompe", String(compteur), "min");
    request->send_P(200, "application/json", buffer);
  });
 
}

void testdrawchar(float temp, String heurePompe, String etatChauff, String IpA) {
  display.clearDisplay();
  delay(200);
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(1, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.print("@ip : "+ IpA +" \r\n");
  display.print("Temperature: " + String(temp) + " C \r\n");
  display.print("Prochaine activation de la pompe: " + heurePompe + " min \r\n");
  display.print("Etat du chauffage: " + etatChauff);
  

  display.display();
  delay(2000);
}

String chauffe(int tempActuel, String tempVoulu){
  int value = atoi(tempVoulu.c_str());
  String etat = "";
  if(tempActuel < value){
    etat = "En chauffe";
  }
  else{
    etat = "En arret";
  }
  return etat;
}

void activationPompe(int compteur, String heurePompe){
  int value = atoi(heurePompe.c_str());
  if (compteur < 900){
    digitalWrite(LED, HIGH);
  }
  else if (compteur > 900) {
    digitalWrite(LED, LOW);
  }
}

void setup()
{
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  // Clear the buffer
  display.clearDisplay();
  display.display();
  
  if(!wm.autoConnect(ssid, password))
    Serial.println("Erreur de connexion.");
  else
    Serial.println("Connexion etablie!");

  server.begin();

  Serial.println("Serveur web actif!");
  Serial.println("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
    });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    tempVoulu = request->getParam(PARAM_INPUT_1)->value();
    heurePompe = request->getParam(PARAM_INPUT_2)->value();
    request->send_P(200, "text/html", index_html);
  });


  sensors.requestTemperatures(); 
  temp = sensors.getTempCByIndex(0);
  
  int pompeInt = atoi(heurePompe.c_str());
  compteur = compteur15 + pompeInt;
  Serial.println(compteur);

  setup_routing();  

  pinMode(LED, OUTPUT);
}

void loop(){

  sensors.requestTemperatures(); 
  Serial.print("temperature: ");
  Serial.println(sensors.getTempCByIndex(0));

  
  etatChauff = chauffe(sensors.getTempCByIndex(0), tempVoulu);
  testdrawchar(sensors.getTempCByIndex(0), heurePompe, etatChauff, String(WiFi.localIP()));
  compteur = compteur - 10;
  int pompeInt = atoi(heurePompe.c_str());
  if (compteur < 11){
    compteur = 900 + pompeInt;
  }
  Serial.println(compteur);
  activationPompe(compteur, heurePompe);
  

  Serial.println(tempVoulu);
  Serial.println(heurePompe);

  delay(10000);

}
