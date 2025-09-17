#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include "config.h"

#define one_wire_bus    A0
#define rx              2
#define tx              3
#define red             6   // 0
#define yellow          5   // 1
#define green           4   // 2
#define buzzer          7

OneWire oneWire(one_wire_bus);
DallasTemperature temp_sensor(&oneWire);
SoftwareSerial gsm(rx, tx);

float getTemperature();
void toggleLight(int color, bool state);
void toggleBuzzer(bool state);
void initializeGSM();
void updateGSM();
void onAlarm();
void offAlarm();
void sendSMS(String number, float temperature);

unsigned long lastSMS = 0;
const unsigned long smsCooldown = 300000;   // 5 mins.
bool messageSent = false;

void setup() {
    Serial.begin(9600);
    temp_sensor.begin();    
    initializeGSM();
    pinMode(red, OUTPUT);
    pinMode(yellow, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(buzzer, OUTPUT);
    Serial.println("Components ready.");
}

void loop() {    
    float temperature = getTemperature();
    Serial.print("temperature: "); Serial.print(temperature, 2); Serial.println("°C");
    
    if (temperature < 50) {
        toggleLight(0, LOW);
        toggleLight(1, LOW);
        toggleLight(2, HIGH);
    } 
    else if (temperature >= 50 && temperature <= 70) {
        toggleLight(0, LOW);
        toggleLight(1, HIGH);
        toggleLight(2, LOW);
    }
    else if (temperature > 70) {
        toggleLight(0, HIGH);
        toggleLight(1, LOW);
        toggleLight(2, LOW);
    }

    if (temperature >= 60) {
        onAlarm();
        unsigned long elapsed = millis() - lastSMS;

        if (!messageSent) {
            sendSMS(PHONE_NUMBER, temperature);
            Serial.println("SMS Sent.");
            messageSent = true;
        }
        
        if (elapsed >= smsCooldown) {
            lastSMS = millis();
        }
        
    }
    else {
        offAlarm();
    }
    
    unsigned long elapsed = millis() - lastSMS;
    if (elapsed < smsCooldown && messageSent) {
        unsigned long remaining = (smsCooldown - elapsed) / 1000;
        Serial.println("SMS Cooldown: " + String(remaining) + " seconds");
    } else {
        messageSent = false;
    }

    delay(1000);
}

float getTemperature() {
    temp_sensor.requestTemperatures();
    float temp_c = temp_sensor.getTempCByIndex(0);
    return temp_c != DEVICE_DISCONNECTED_C ? temp_c : 0;
}

void toggleLight(int color, bool state) {
    switch (color) {
        case 0: digitalWrite(red, state); Serial.println("RED: " + String(state)); break;
        case 1: digitalWrite(yellow, state); Serial.println("YELLOW: " + String(state)); break;
        case 2: digitalWrite(green, state); Serial.println("GREEN: " + String(state)); break;
        default:
        {
            digitalWrite(red, LOW);
            digitalWrite(yellow, LOW);
            digitalWrite(green, LOW);
        }
    }
}

unsigned long lastToggle = 0;
bool buzzerState = false;
void onAlarm() {
    if (millis() - lastToggle >= 500) {
        lastToggle = millis();
        buzzerState = !buzzerState;
        toggleBuzzer(buzzerState);
    }
}

void offAlarm() {
    digitalWrite(buzzer, LOW);
}

void toggleBuzzer(bool state) {
    digitalWrite(buzzer, state);
}

void updateGSM() {
    delay(500);
    while(gsm.available()) Serial.write((char)gsm.read());
    while(Serial.available()) gsm.write(Serial.read());
}

void initializeGSM() {
    gsm.begin(9600);

    Serial.println("Initializing module...");
    gsm.println("AT");
    updateGSM();
    delay(500);

    Serial.println("Updating time...");
    gsm.println("AT+CLTS=1");
    updateGSM();
    gsm.println("AT&W");
    updateGSM();
}

void sendSMS(String number, float temperature) {
    String message = "";
    gsm.println("AT+CMGF=1");
    updateGSM();
    delay(250);

    gsm.println("AT+CMGS=\"" + number + "\"");
    updateGSM();
    delay(1000);    

    message = 
        "WARNING: High temperature detected (" + 
        String(temperature) + " C) ";

    gsm.print(message);
    gsm.print(char(26));
    updateGSM();
    delay(1000);
}