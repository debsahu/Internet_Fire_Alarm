#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[255] = "home/firealarm1";

int holdPin = 0;  // defines GPIO 0 as the hold pin (will hold CH_PD high untill we power down).
int pirPin = 12;  // defines GPIO 12 as the PIR read pin (reads the state of the PIR output).
int pir = 1;      // sets the PIR record (pir) to 1 (it must have been we woke up).

WiFiClient ethClient;
PubSubClient client(ethClient);

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("firealarm1")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void wifi_connect() {
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  
  WiFiManagerParameter custom_mqtt_server("MQTT Server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("MQTT Port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("MQTT Topic", "mqtt_topic", mqtt_topic, 255);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);
  if (!wifiManager.autoConnect("FireAlarm")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected...yay :)");
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(holdPin, OUTPUT);     // sets GPIO 0 to output
  digitalWrite(holdPin, HIGH);  // sets GPIO 0 to high (this holds CH_PD high even if the PIR output goes low)
  pinMode(pirPin, INPUT);       // sets GPIO 12 to an input so we can read the PIR output state

  Serial.begin(115200);         // Start Serial connection
  wifi_connect();               // Setup WiFi Connection
  client.setServer(mqtt_server, atoi(mqtt_port)); // setup MQTT prameters
  pinMode(LED_BUILTIN, OUTPUT); /////////////////////
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  digitalWrite(LED_BUILTIN, LOW); ///////////////
  
  if((pir) == 0){  // if (pir) == 0, which its not first time through as we set it to "1" above
    client.publish(mqtt_topic, "OFF");
    Serial.println("OFF");
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); ///////////////
    client.disconnect(); // disconnect from MQTT
    ethClient.stop();    // close WiFi client
    delay(1000);         // wait for client to close
    digitalWrite(holdPin, LOW);  // set GPIO 0 low this takes CH_PD & powers down the ESP
  }else{                 // if (pir) == 0 is not true
    client.publish(mqtt_topic, "ON");
    Serial.println("ON");
    while(digitalRead(pirPin) == 1){  // read GPIO 12, while GPIO 2 = 1 is true, wait (delay below) & read again, when GPIO 2 = 1 is false skip delay & move on out of "while loop"
      delay(500);
      client.loop();
      Serial.print(".");
    }
    pir = 0;             // set the value of (pir) to 0
    delay(2000);        // wait 20 sec
  }
  // end of void loop, returns to start loop again at void loop
}
