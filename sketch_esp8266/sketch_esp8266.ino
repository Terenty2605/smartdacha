/*
Name:        lightBot.ino
Created:     20/06/2020
Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
Description: a simple example that do:
             1) parse incoming messages
             2) if "LIGHT ON" message is received, turn on the onboard LED
             3) if "LIGHT OFF" message is received, turn off the onboard LED
             4) otherwise, reply to sender with a welcome message

*/

#include <ESPDateTime.h>
#include <Arduino.h>
#include "AsyncTelegram.h"

AsyncTelegram myBot;

#ifndef STASSID
#define STASSID "*"
#define STAPSK  "*"
#endif

const char* ssid = STASSID;
const char* pass = STAPSK;
const char* token = "*";

const uint8_t LED = LED_BUILTIN;
// const uint8_t LED = 6;

void setup() {
  // initialize the Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  Serial.println("Starting TelegramBot...");
  Serial.println("Connecting WIFI...");
  WiFi.setAutoConnect(true);   
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  Serial.println("\nWIFI connected.");
  // To ensure certificate validation, WiFiClientSecure needs time updated
  // myBot.setInsecure(false);
  myBot.setClock("CET-1CEST,M3.5.0,M10.5.0/3");
  
  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // set the pin connected to the LED to act as output pin
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // turn off the led (inverted logic!)

}

void loop() {
  // a variable to store telegram message data
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
