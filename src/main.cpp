#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <Preferences.h>

#define one_wire_bus    4 // A0
#define rx              32 // 2
#define tx              33 // 3
#define red             27   // 0
#define yellow          26   // 1
#define green           25   // 2
#define buzzer          13
#define MAX_SUBSCRIBERS 5

#define TEST false
#define test_num "+639xxxxxxxxx"

OneWire oneWire(one_wire_bus);
DallasTemperature temp_sensor(&oneWire);
SoftwareSerial gsm(rx, tx);

static const char* PREFS_NAMESPACE = "nums";
Preferences prefs;
String subscribers[MAX_SUBSCRIBERS];
int subscriber_count = 0;

float getTemperature();
void toggleLight(int color, bool state);
void toggleBuzzer(bool state);
void initializeGSM();
void updateGSM();
void onAlarm();
void offAlarm();
void sendSMS(String number, String message);
void getSubscriberList(String num);
void checkSMS();
void loadSubscribers();
void addSubscriber(String num);
void deleteSubscriber(String num);
bool isSubscribed(String num);
void clearNamespace();
void showStats();
void toggleLEDs();
void getStatus(String sender);

float temperature = 0;
unsigned long lastSMS = 0;
const unsigned long smsCooldown = 300000;   // 5 mins.
unsigned long last_loop_time = 0;
unsigned long lastCooldownPrint = 0;  // Track last cooldown print time
bool messageSent = false;
bool state_green = false, state_yellow = false, state_red = false, state_alarm = false, led_state = true;
int lastTempState = -1;  // Track last temperature state (-1: unknown, 0: cold, 1: moderate, 2: hot)
String tempLevel = "";

void setup() {
    Serial.begin(115200);
    temp_sensor.begin();    
    initializeGSM();
    loadSubscribers();
    pinMode(red, OUTPUT);
    pinMode(yellow, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(2, OUTPUT);
    Serial.println("Components ready.");
}

void loop() {    
    checkSMS();

    if (millis() - last_loop_time >= 1000) {
        last_loop_time = millis();
        digitalWrite(2, led_state);
        led_state = !led_state;
        temperature = getTemperature();
        toggleLEDs();
        showStats();  
    }

    int currentTempState = -1;

    if (temperature < 50) {
        state_green = true;
        state_yellow = false;
        state_red = false;
        currentTempState = 0;
        tempLevel = "NORMAL";
    } 
    else if (temperature >= 50 && temperature <= 70) {
        state_green = false;
        state_yellow = true;
        state_red = false;
        currentTempState = 1;
        tempLevel = "MODERATE";
    }
    else if (temperature > 70) {
        state_green = false;
        state_yellow = false;
        state_red = true;
        currentTempState = 2;
        tempLevel = "HIGH";
    }

    // Check if temperature state has changed
    if (currentTempState != lastTempState) {
        lastTempState = currentTempState;
        messageSent = false;  // Reset message flag when state changes
        lastSMS = millis();   // Reset cooldown timer
    }

    // Send SMS alert based on current temperature state
    unsigned long elapsed = millis() - lastSMS;

    if (currentTempState == 0 && !messageSent) {
        // Moderate alert
        String message = "[ALERT]\nTemperature is NORMAL (" + String(temperature) + " C)"; 
        
        #if (TEST == false)
            for (int i = 0; i < subscriber_count; i++) {
                if (subscribers[i].length() > 0) {
                    sendSMS(subscribers[i], message);
                }
            }
        #else
            sendSMS(test_num, message);
        #endif
        
        Serial.println("NORMAL alert SMS sent to all subscribers.");
        messageSent = true;
    }
    else if (currentTempState == 1 && !messageSent) {
        // Moderate alert
        String message = "[ALERT]\nTemperature is MODERATE (" + String(temperature) + " C)";
        
        #if (TEST == false)
            for (int i = 0; i < subscriber_count; i++) {
                if (subscribers[i].length() > 0) {
                    sendSMS(subscribers[i], message);
                }
            }
        #else
            sendSMS(test_num, message);
        #endif
        
        Serial.println("Moderate alert SMS sent to all subscribers.");
        messageSent = true;
    }
    else if (currentTempState == 2 && !messageSent) {
        // Hot alert (high temperature)
        String message = "[WARNING]\nHIGH temperature detected (" + String(temperature) + " C)";
        
        #if (TEST == false)
            for (int i = 0; i < subscriber_count; i++) {
                if (subscribers[i].length() > 0) {
                    sendSMS(subscribers[i], message);
                }
            }
        #else
            sendSMS(test_num, message);
        #endif
        
        Serial.println("Hot alert SMS sent to all subscribers.");
        messageSent = true;
    }

    (currentTempState == 2) ? onAlarm() : offAlarm();
    
    if (elapsed < smsCooldown && messageSent) {
        if (millis() - lastCooldownPrint >= 1000) {
            lastCooldownPrint = millis();
            unsigned long remaining = (smsCooldown - elapsed) / 1000;
            Serial.println("SMS Cooldown: " + String(remaining) + " seconds");
        }
    }
}

float getTemperature() {
    temp_sensor.requestTemperatures();
    float temp_c = temp_sensor.getTempCByIndex(0);
    return temp_c != DEVICE_DISCONNECTED_C ? temp_c : 0;
}

void toggleLight(int color, bool state) {
    switch (color) {
        case 0: digitalWrite(red, state); break;
        case 1: digitalWrite(yellow, state); break;
        case 2: digitalWrite(green, state); break;
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
    state_alarm = true;
    if (millis() - lastToggle >= 500) {
        lastToggle = millis();
        buzzerState = !buzzerState;
        toggleBuzzer(buzzerState);
    }
}

void offAlarm() {
    state_alarm = false;
    digitalWrite(buzzer, LOW);
}

void toggleBuzzer(bool state) {
    digitalWrite(buzzer, state);
}

void updateGSM() {
    delay(100);
    int timeout = 0;
    while (gsm.available() && timeout < 100) {
        Serial.write((char)gsm.read());
        timeout++;
        delay(5);
    }
}

void showStats() {
    Serial.println("temp: " + String(temperature) + "°C | G: " + String(state_green) + " | Y: " + String(state_yellow) + " | R: " + String(state_red) + " | Alarm: " + String(state_alarm ? "ON" : "OFF")); 
}

void toggleLEDs(){
    toggleLight(2, state_green);
    toggleLight(1, state_yellow);
    toggleLight(0, state_red);
}

void initializeGSM () {
    Serial.println("Checking module...");
    gsm.begin(9600);
    delay(100);

    gsm.println("AT");
    delay(100);

    gsm.println("AT+CSCS=\"GSM\"");
    delay(100);

    gsm.println("AT+CNMI=1,2,0,0,0");
    delay(100);

    gsm.println("AT+CMGF=1");
    delay(100);

    Serial.println("Updating time...");
    gsm.println("AT+CLTS=1");
    delay(100);

    gsm.println("AT&W");
    delay(100);
}

void sendSMS (String num, String message) {
    gsm.println("AT+CMGF=1");
    delay(100);
    
    gsm.println("AT+CMGS=\"" + num + "\"");
    delay(500);    

    gsm.print(message);
    gsm.print(char(26));
    delay(1000);
    updateGSM();
}

void loadSubscribers() {
    prefs.begin(PREFS_NAMESPACE, false);
    subscriber_count = prefs.getInt("count", 0);
    for (int i = 0; i < subscriber_count; i++) {
        String key = "num" + String(i);
        subscribers[i] = prefs.getString(key.c_str(), "");
        Serial.println(subscribers[i]);
    }
    prefs.end();  // Close after all operations
    Serial.println("Loaded " + String(subscriber_count) + " subscribers.");
}

void clearNamespace() {
    prefs.clear();
}

bool isSubscribed(String num) {
    for (int i = 0; i < subscriber_count; i++) {
        if (subscribers[i] == num) return true;
    }
    return false;
}

/// @brief add suscribers for sms feature
/// @param num mobile number of subscriber
void addSubscriber(String num) {
    if (isSubscribed(num)) {
        sendSMS(num, "[" + num + "] Already subscribed to device. Text GSM UNSUB to unsubscribe, GSM LIST to see list of subscribers. ");
        Serial.println("Already subscribed");
        return;
    }
    if (subscriber_count >= MAX_SUBSCRIBERS) {
        sendSMS(num, "Reached max. number of subscribers. Text GSM UNSUB to unsubscribe, GSM LIST to see list of subscribers. ");
        Serial.println("Reached max number of subscribers");
        return;
    }
    
    prefs.begin(PREFS_NAMESPACE, false);
    
    String key = "num" + String(subscriber_count);
    prefs.putString(key.c_str(), num);
    subscribers[subscriber_count] = num;
    subscriber_count++;
    prefs.putInt("count", subscriber_count);
    
    prefs.end();
    sendSMS(num, "[" + num + "] Subscribed to device. Text GSM UNSUB to unsubscribe, GSM LIST to see list of subscribers. ");
    Serial.println("Added subscriber: " + num);
}

void deleteSubscriber(String num) {
    bool found = false;

    for (int i = 0; i < subscriber_count; i++) {
        if (subscribers[i] == num) {
            for (int j = i; j < subscriber_count - 1; j++) {
                subscribers[j] = subscribers[j + 1];
            }
            subscriber_count--;
            found = true;
            break;
        }
    }

    if (found) {
        prefs.begin(PREFS_NAMESPACE, false);
        prefs.putInt("count", subscriber_count);

        for (int i = 0; i < subscriber_count; i++) {
            String key = "num" + String(i);
            prefs.putString(key.c_str(), subscribers[i]);
        }
        
        // Remove the last orphaned key
        String lastKey = "num" + String(subscriber_count);
        prefs.remove(lastKey.c_str());
        
        prefs.end();
        
        sendSMS(num, "[" + num + "] Unsubscribed to device. text GSM SUB to subscribe, GSM LIST to see list of subscribers.");
        Serial.println("Deleted subscriber: " + num);
    }
}

void checkSMS() {    
    if (gsm.available()) {
        String response = "";
        int timeout = 0;
        
        while (gsm.available() && timeout < 200) {
            response += (char)gsm.read();
            delay(5);
            timeout++;
        }

        response.toUpperCase();
        Serial.println(response);

        int numStartIndex = response.indexOf('"');
        int numEndIndex = response.indexOf('"', numStartIndex + 1);
        String sender = "", message = "";

        if (numStartIndex != -1 && numEndIndex != -1) {
            sender = response.substring(numStartIndex + 1, numEndIndex);
        }

        int msgStart = response.indexOf('\n', numEndIndex);
        if (msgStart != -1) {
            message = response.substring(msgStart + 1);
            message.trim();
        }

        if (message.indexOf("GSM SUB") > -1) {
            addSubscriber(sender);
        }
        else if (message.indexOf("GSM UNSUB") > -1) {
            deleteSubscriber(sender);
        }
        else if (message.indexOf("GSM LIST") > -1) {
            getSubscriberList(sender);
        }
        else if (message.indexOf("GSM STATUS") > -1) {
            getStatus(sender);
        }
    }    
}

void getSubscriberList(String sender) {
    String message = "";

    for (int i = 0; i < subscriber_count; i++) {
        message += String(i + 1) + ". " + subscribers[i] + "\n";
    }
    sendSMS(sender, message);
}

void getStatus(String sender) {
    String message = 
        "[STATUS]\nTemp: " + String(temperature) + " C\n"
        + "Level: " + tempLevel + "\n"
        + "LED: " + String(state_green ? "GREEN" : "") + String(state_yellow ? "YELLOW" : "") + String(state_red ? "RED" : "");

    sendSMS(sender, message);
}