#include <FS.h>

#include <WC_AP_HTML.h>
#include <WiFiConnect.h>
#include <WiFiConnectParam.h>
#include <ESPDateTime.h>
#include <AsyncTelegram.h>

AsyncTelegram myBot;

const uint8_t LED = LED_BUILTIN;

#define TOKEN_LEN 200
char token[TOKEN_LEN] = "token";

int json_doc_size = 1024;

WiFiConnectParam custom_token("token", "token", token, TOKEN_LEN);

WiFiConnect wc;

bool configNeedsSaving = false;

void saveConfigCallback() {
  Serial.println("Should save config");
  configNeedsSaving = true;
}

void configModeCallback(WiFiConnect *mWiFiConnect) {
  Serial.println("Entering Access Point");
}

/* Save our custom parameters */
void saveConfiguration() {
  configNeedsSaving = false;
  if (!SPIFFS.begin()) {
    Serial.println("UNABLE to open SPIFFS");
    return;
  }

  strcpy(token, custom_token.getValue());

  Serial.println("saving config");
  DynamicJsonDocument doc(json_doc_size);

  doc["token"] = token;

  File configFile = SPIFFS.open("/conf.json", "w");

  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  serializeJson(doc, Serial);
  Serial.println("");
  serializeJson(doc, configFile);
  configFile.close();
  SPIFFS.end();
}

void loadConfiguration() {
  if (!SPIFFS.begin()) {
    Serial.println(F("Unable to start SPIFFS"));
    while (1) {
      delay(1000);
    }
  } else {
    Serial.println(F("mounted file system"));

    if (SPIFFS.exists("/conf.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");

      File configFile = SPIFFS.open("/conf.json", "r");
      if (configFile) {
        Serial.println(F("opened config file"));
        size_t sizec = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[sizec]);

        configFile.readBytes(buf.get(), sizec);
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, buf.get());
        if (error) {
          Serial.println("failed to load json config");
        } else {
          strcpy(token, doc["token"]);
          custom_token.setValue(token);
        }
        configFile.close();

      }
    } else {
      Serial.println(F("Config file not found"));
    }
    SPIFFS.end();
  }
}

void startWiFi(boolean showParams = false) {
 
  wc.setDebug(true);

  wc.setSaveConfigCallback(saveConfigCallback);
  wc.setAPCallback(configModeCallback);

  wc.addParameter(&custom_token);
  //wc.resetSettings(); //helper to remove the stored wifi connection, comment out after first upload and re upload

    /*
       AP_NONE = Continue executing code
       AP_LOOP = Trap in a continuous loop - Device is useless
       AP_RESET = Restart the chip
       AP_WAIT  = Trap in a continuous loop with captive portal until we have a working WiFi connection
    */
    if (!wc.autoConnect()) { // try to connect to wifi
      wc.startConfigurationPortal(AP_WAIT);//if not connected show the configuration portal
    }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  Serial.println("....");
  Serial.println("....");
  delay (5000);
  loadConfiguration();
  startWiFi();
  Serial.println(token);
  
  myBot.setClock("CET-1CEST,M3.5.0,M10.5.0/3");
  
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // turn off the led (inverted logic!)

}

void loop() {
  delay(100);
  if (configNeedsSaving) {
    saveConfiguration();
  }
    
  
  // Wifi Dies? Start Portal Again
  if (WiFi.status() != WL_CONNECTED) {
    if (!wc.autoConnect()) wc.startConfigurationPortal(AP_WAIT);
  }
  
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {

    if (msg.text.equalsIgnoreCase("\/on")) {      // if the received message is "LIGHT ON"...
      digitalWrite(LED, LOW);                           // turn on the LED (inverted logic!)
      myBot.sendMessage(msg, "Light is now ON");        // notify the sender
    }
    else if (msg.text.equalsIgnoreCase("\/off")) {        // if the received message is "LIGHT OFF"...
      digitalWrite(LED, HIGH);                          // turn off the led (inverted logic!)
      myBot.sendMessage(msg, "Light is now OFF");       // notify the sender
    }
    else if (msg.text.equalsIgnoreCase("\/time")) {        // if the received message is "LIGHT OFF"...
      Serial.println(DateTime.toUTCString());
      String reply;
      reply = DateTime.toUTCString();
      myBot.sendMessage(msg, reply);
    }
    else if (msg.text.equalsIgnoreCase("\/start")) {        // if the received message is "LIGHT OFF"...
      // generate the message for the sender
      String reply;
      reply = "Welcome " ;
      reply += msg.sender.username;
      reply += ".\nTry \/on or \/off (case insensitive)";
      myBot.sendMessage(msg, reply);             // and send it
    }
    else {                                                    // otherwise...
      myBot.sendMessage(msg, msg.text);             // echo
    }
  }

}
