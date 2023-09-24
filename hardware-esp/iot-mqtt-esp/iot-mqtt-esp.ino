#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>

#define FIRMWARE_VERSION 1.0
#define UPDATE_URL "https://mondone.github.io/mqtt-firmware/firmware.json"

// Conectar al wifi "ESP32-Web-Config" y posterior ingresar a 192.168.4.1 para configurar el esp32. 
char webpage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configuración WiFi y MQTT</title>
  <style>
    body {
      text-align: center;
      font-size: 1.2em;
    }

    form {
      display: inline-block;
      text-align: left;
    }

    input[type="text"],
    input[type="password"] {
      width: calc(100% - 30px);
      padding: 10px;
      margin-bottom: 10px;
      box-sizing: border-box;
    }

    .ver {
      position: absolute;
      right: 40px; /* Ajusta la distancia del borde derecho según tu preferencia */
      top: 35%;
      transform: translateY(-50%);
      cursor: pointer;
      text-decoration: line-through;
    }
    
    input[type="submit"] {
      background-color: #4caf50;
      color: white;
      padding: 10px 20px;
      font-size: 1em;
      border: none;
      cursor: pointer;
    }

    input[type="submit"]:hover {
      background-color: #45a049;
    }
  </style>
  
  <script>
  function togglePassword() {
    var passwordInput = document.getElementById("password");
    var ver = document.getElementById("ver");

    if (passwordInput.type === "password") {
      passwordInput.type = "text";
      ver.style.textDecoration = "none";
    } else {
      passwordInput.type = "password";
      ver.style.textDecoration = "line-through";
    }
  }
</script>

  
</head>
<body>

<h2>Configuración WiFi y MQTT</h2>

<form action="/configurar" method="post">
  WiFi SSID: <input type="text" name="ssid"><br>
  <div style="position: relative;">
    WiFi Contraseña: <input type="password" name="password" id="password">
    <p id="ver" class="ver" onclick="togglePassword()">Ver</p>
  </div>
  MQTT Servidor: <input type="text" name="mqtt_server"><br>
  MQTT Puerto: <input type="text" name="mqtt_port"><br>
  <input type="submit" value="Guardar">
</form>

</body>
</html

)=====";

String ssid = "ESP32-Web-Config";
String password = "";
String mqtt_server = "";
String mqtt_port = "1883";

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);

// Declaración de funciones
void wifiInit();
void connectWifiMQTT();
void reconnect();
void getConfig();
void serverGet();
void serverPost();
void checkUpdate();

void setup() {

  Serial.begin(115200);

  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Configurar ESP32 como un punto de acceso
  WiFi.softAP(ssid.c_str(), password.c_str());

  // Configurar Servidor
  serverGet(); // server.on "/", HTTP_GET, AsyncWebServer
  serverPost(); // server.on "/configurar", HTTP_POST, AsyncWebServer
  server.begin(); 

  // Obtenet la informacion en la memoria SPIFFS
  getConfig();

  // Informar version de firmware en el setup
  Serial.println("Setup Firmware version: " + String(FIRMWARE_VERSION));
  
}

void loop() {
  // put your main code here, to run repeatedly:
  bool wifi = WiFi.status() == WL_CONNECTED;

  if (wifi) 
  {
    Serial.println("Loop Firmware version: " + String(FIRMWARE_VERSION));
    checkUpdate();

    if (!client.connected()) {
      reconnect();
    }
    
    client.loop();

    String dato = String(random(45,90));
    client.publish("casa/living/temp", dato.c_str());
    Serial.println("Temperatura: " + dato );
    delay(3000);

  }
}

void wifiInit() {
  Serial.println("Conectando a " + ssid);
  Serial.println("Pass: " + password);

  const char* ssid_c = ssid.c_str();
  const char* password_c = password.c_str();

  WiFi.begin(ssid_c, password_c);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado a la red WiFi");
  Serial.println(WiFi.localIP());
}

void connectWifiMQTT() {
  wifiInit();
  client.setServer(mqtt_server.c_str(), mqtt_port.toInt());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");

    if (client.connect("esp32-client")) {
      Serial.println("Conectado");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void serverGet() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", webpage);
  });
}

void serverPost() {
  server.on("/configurar", HTTP_POST, [](AsyncWebServerRequest *request){
    ssid = request->arg("ssid");
    password = request->arg("password");
    mqtt_server = request->arg("mqtt_server");
    mqtt_port = request->arg("mqtt_port");

    Serial.println("Configuración recibida:");
    Serial.println("SSID: " + ssid);
    Serial.println("Contraseña: " + password);
    Serial.println("MQTT Servidor: " + mqtt_server);
    Serial.println("MQTT Puerto: " + mqtt_port);

    // Guardar los datos en SPIFFS
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Error al abrir el archivo de configuración");
    } else {
      DynamicJsonDocument doc(1024);
      doc["ssid"] = ssid;
      doc["password"] = password;
      doc["mqtt_server"] = mqtt_server;
      doc["mqtt_port"] = mqtt_port;

      serializeJson(doc, configFile);
      configFile.close();
      Serial.println("Configuración guardada correctamente");
    }

    if (ssid != "null" && ssid.length() > 2) {
      // Conexion Wifi y MQTT
      Serial.println("Iniciando Wifi despues de guardar los datos");
      wifiInit();
      client.setServer(mqtt_server.c_str(), mqtt_port.toInt());
    }

    request->send(200, "text/plain", "Configuración guardada correctamente");

  });
}

void getConfig() {
  // Intentar cargar la configuración almacenada en SPIFFS
  File configFile = SPIFFS.open("/config.json", "r");
  if (configFile) {
    Serial.println("Leyendo la configuración en SPIFFS");

    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, buf.get());

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    mqtt_server = doc["mqtt_server"].as<String>();
    mqtt_port = doc["mqtt_port"].as<String>();

    Serial.println("Configuración Leida de SPIFFS:");
    Serial.println("SSID: " + ssid);
    Serial.println("Contraseña: " + password);
    Serial.println("MQTT Servidor: " + mqtt_server);
    Serial.println("MQTT Puerto: " + mqtt_port);

    // Conexion Wifi y MQTT
    if (ssid != "null" && ssid.length() > 2) {
      connectWifiMQTT();
    }
    
  }
  else{
      Serial.println("No hay informacion para leer");
  }
}

void checkUpdate(){
    Serial.println("Checking update");
    HTTPClient http;
    String response;
    String url = UPDATE_URL;
    http.begin(url);
    http.GET();
    response = http.getString();
    Serial.println(response);
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, response);
    JsonObject obj = doc.as<JsonObject>();
    String version = obj[String("version")];
    String url_update = obj[String("url")];
    Serial.println(version);
    Serial.println(url_update);
    if (version.toDouble() > FIRMWARE_VERSION)
    {
        Serial.println("Update Available");
        Serial.println(url_update);
        if (updateOverHttp(url_update) == HTTP_UPDATE_OK)
        {
            Serial.println("Update Success");
        }
        else
        {
            Serial.println("Update Failed");
        }
        Serial.println("Update Success");
    }
    else
    {
        Serial.println("No Update Available");
    }
}

t_httpUpdate_return updateOverHttp(String url_update){
  t_httpUpdate_return ret;

  if ((WiFi.status() == WL_CONNECTED))
  {
    ret = ESPhttpUpdate.update(url_update);
    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        return ret;
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        return ret;
        break;
    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        return ret;
        break;
    }
  }
}

