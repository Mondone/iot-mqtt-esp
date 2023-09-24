#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);

const char* ssid     = "";
const char* password = "";
const char* server = ""; // IP de la pc en donde corre docker o pagina del servidor mqtt
int port = 1883;

//int var = 0;
//char datos[40];
//String resultS = "";

// Declaración de funciones.
void wifiInit();
//void callback();
void reconnect();

void setup()
{
  Serial.begin(115200);
  delay(10);
  wifiInit();
  mqttClient.setServer(server, port);
  //mqttClient.setCallback(callback);
}

void loop()
{
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  if (mqttClient.connected()) {

    String temp = String(random(45,90));
    mqttClient.publish("casa/living/temp", temp.c_str());
    Serial.println("Temperatura: " + temp);

  }
  delay(5000);
}

void wifiInit() {
    Serial.print("Conectándose a ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
        delay(500);  
    }
    Serial.println("");
    Serial.println("Conectado a WiFi");
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

/*
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");

  char payload_string[length + 1];
  
  int resultI;

  memcpy(payload_string, payload, length);
  payload_string[length] = '\0';
  resultI = atoi(payload_string);
  
  var = resultI;

  resultS = "";
  
  for (int i=0;i<length;i++) {
    resultS= resultS + (char)payload[i];
  }
  Serial.println();
}
*/

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");

    if (mqttClient.connect("esp32Client")) {
      Serial.println("Conectado");
      //mqttClient.subscribe("Entrada/01");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" intentar de nuevo en 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}