# Temperature Monitoring and Alarm System
Monitors temperature changes every 1 second and sends an SMS alert if certain temperature levels are reached. The buzzer module provides auditory alarm feedback while the RGY LED shows the current temperature level.
- if temperature < 50, light: green
- if tempereature >= 50 && <= 70, light yellow, buzzer alarm, send SMS (5 mins. cooldown)
- if temperature > 70, light: red
# GSM Subscription Feature
You can message the device certain keywords to perform subscription actions. Device can store up to 5 max contacts/subscribers. Subscribers = contacts/phone numbers.
- `GSM SUB` - subscribes to the device, will receive alert messages. It will also notify the user if they are already subscribed if tyring to subscribe again.
- `GSM UNSUB` - unsubscribers to the device, will no longer receive alerts messages.
- `GSM LIST` - gets the list of the subscribers.
# Components
- DS18B20 temperature Sensor
- ESP32
- SIM800L EVB GSM module
- Buzzer module
- RGY LED module
