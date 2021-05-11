#include <FS.h>

#include <WC_AP_HTML.h>
#include <WiFiConnect.h>
#include <WiFiConnectParam.h>
#include <ESPDateTime.h>
#include <AsyncTelegram.h>
#include <DHT.h>

AsyncTelegram myBot;

int admin1_id = 69748375;
int admin2_id = 531808473;

const uint8_t LED = LED_BUILTIN;
const uint8_t LED_GREEN = 12;
const uint8_t LED_RED = 15;
const uint8_t BUTTON_PIN = 4;
const uint8_t PIR_PIN = 0;
const uint8_t DHT_PIN = 5;

#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

const uint8_t analogPin = A0;
int val = 0, temp, pir_state = HIGH;

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

      Serial.println("reading config file");

      File configFile = SPIFFS.open("/conf.json", "r");
      if (configFile) {
        Serial.println(F("opened config file"));
        size_t sizec = configFile.size();

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
  //wc.resetSettings();                                     // Команда для очистки внутренней флеш-памяти
  if (!wc.autoConnect()) {                                  // Автоматическая попытка подключиться к wifi
      wc.startConfigurationPortal(AP_WAIT);                 // Если не удалось - открытие меню конфигурации wifi
  }
}


void setup() {
  Serial.begin(115200);                                     // Настройка серийных портов отладки                                
  pinMode(LED, OUTPUT);                                     // Настойка выходов светодиодов - отдельно синий канал
  pinMode(LED_GREEN, OUTPUT);                               // Многоцветный зеленый канал
  pinMode(LED_RED, OUTPUT);                                 // Многоцветный красный канал
  digitalWrite(LED, HIGH);                                  // Выключение синего светодиода (обратная логика!)
  digitalWrite(LED_RED, HIGH);                              // Включение светодиода (прямая логика)
  pinMode(BUTTON_PIN, INPUT_PULLUP);                        // Пин (контакт) для тревожной кнопки
  pinMode(PIR_PIN, INPUT);                                  // Пин для датчика движения
  dht.begin();                                              // Инициализируем библиотеку цифрового датчика температуры и влажности

  while (!Serial) {                                         // Ожидаем пока откроется серийный порт (таймаут 100 мс)
    delay(100);
  }
  Serial.println("....");
  Serial.println("....");
  delay (5000);
  loadConfiguration();                                      // Пытаемся загрузить конфигурацию из флеш-памяти
  startWiFi();                                              // Вызываем функцию запуска wifi
  Serial.println(token);

  myBot.setClock("CET-1CEST,M3.5.0,M10.5.0/3");             // Перед началом работы синхронизируем внутренние часы в сервером времени в интернете

  myBot.setUpdateTime(1000);                                // Частота считывания сообщений в Telegram боте (1000мс = 1с)
  myBot.setTelegramToken(token);                            // Запуск бота с определенным токеном

  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  digitalWrite(LED_RED, LOW);                               // Выключение красного светодиода
  digitalWrite(LED_GREEN, HIGH);                            // Включение зеленого светодиода

  TBMessage msg;                                            // Отправляем двум админам сообщение, что бот в сети
  msg.sender.id = admin1_id;
  myBot.sendMessage(msg, "Бот в сети");
  msg.sender.id = admin2_id;
  myBot.sendMessage(msg, "Бот в сети");

}

void loop() {                                               // Главный цикл работы бота
  int btn_Status = HIGH;
  int pir_Status = HIGH;
  delay(100);
  if (configNeedsSaving) {                                  // Если конфигурация изменилась - (bot api token) сохранить на флеш-память
    saveConfiguration();
  }

  float h = dht.readHumidity();                             // Считываем температуру и влажность
  float t = dht.readTemperature();

  if (WiFi.status() != WL_CONNECTED) {                      // Если wifi не подключен - открытие меню конфигурации wifi
  if (!wc.autoConnect()) wc.startConfigurationPortal(AP_WAIT);
  }

  btn_Status = digitalRead (BUTTON_PIN);                    // Обработка статуса тревожной кнопки
  if (btn_Status == LOW) {                                  // Если кнопка нажата - сообщить двум админам
    TBMessage alarmMsg;
    alarmMsg.sender.id = admin1_id;
    myBot.sendMessage(alarmMsg, "Сработала тревожная кнопка!");
    alarmMsg.sender.id = admin2_id;
    myBot.sendMessage(alarmMsg, "Сработала тревожная кнопка!");
  }

  pir_Status = digitalRead (PIR_PIN);                       // Обработка статуса датчика движения
  if (pir_Status != pir_state) {                            // Если статус не совпадает с предыдущим
    TBMessage alarmMsg;
    if (pir_Status == HIGH) {                               // И датчик только что сработал
      alarmMsg.sender.id = admin1_id;
      myBot.sendMessage(alarmMsg, "Сработал датчик движения!");
      alarmMsg.sender.id = admin2_id;
      myBot.sendMessage(alarmMsg, "Сработал датчик движения!");
    } else {                                                // Или датчик отключился
      alarmMsg.sender.id = admin1_id;
      myBot.sendMessage(alarmMsg, "Движение прекратилось!");
      alarmMsg.sender.id = admin2_id;
      myBot.sendMessage(alarmMsg, "Движение прекратилось!");
    }
    pir_state = pir_Status;                                 // Сохраняем предыдущее состояние
  }


  TBMessage msg;

                                                            // Если появилось входящее сообщение...
  if (myBot.getNewMessage(msg)) {

    if (msg.text.equalsIgnoreCase("\/on")) {                // Если полученное сообщение /on...
      digitalWrite(LED, LOW);                               // Включение отдельного синего светодиода (обратная логика!)
      myBot.sendMessage(msg, "Сейчас свет (реле) включен"); // Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/off")) {          // Если полученное сообщение /off...
      digitalWrite(LED, HIGH);                              // Выключение светодиода (обратная логика!)
      myBot.sendMessage(msg, "Сейчас свет (реле) выключен");// Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/time")) {         // Если полученное сообщение /time...
      Serial.println(DateTime.toUTCString());
      String reply;
      reply = DateTime.toUTCString();
      myBot.sendMessage(msg, reply);                        // Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/dark")) {         // Если полученное сообщение /dark...
      val = analogRead(analogPin);                          // Считываем состояние фоторезистора аналоговое 0 - 1024
      Serial.println(val);
      String reply = String(val);
      myBot.sendMessage(msg, reply);                        // Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/temp")) {         // Если полученное сообщение /temp...
      String reply = "\nТемпература: "+String(t)+"\nВлажность: "+String(h);
      myBot.sendMessage(msg, reply);                        // Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/id")) {           // Если полученное сообщение /id...
      String reply = "Идентификатор чата = " + String(msg.sender.id);
      myBot.sendMessage(msg, reply);                        // Необходимо уведомить отправителя
    }
    else if (msg.text.equalsIgnoreCase("\/start")) {        // Если полученное сообщение /start...
                                                            // Создание сообщение для отправителя
      String reply;
      reply = "Приветствуем вас, " ;
      reply += msg.sender.username;                         // Бот приветствует вас по вашему id (имени) в Telegram
      reply += ".\nДобро пожаловать в умного бота SmartDacha";
      reply += ".\nПожалуйста, изучите команды (значок косой черты снизу справа)";
      myBot.sendMessage(msg, reply);                        // Отправка
    }
    else {                                                  // В противном случае...
      String reply = "Извините, я еще не умею распознавать человеческую речь, пожалуйста, используйте команды.";
      myBot.sendMessage(msg, reply);
    } 
  }

}
