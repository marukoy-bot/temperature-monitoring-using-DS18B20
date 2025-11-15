# Schematic
<div align="center">
    <image src="proj-temp-DS18B20_bb.png" height=350 alt="Circuit Diagram"/>
</div>

# Temperature Monitoring and Alarm System
Monitors temperature changes every 1 second and sends an SMS alert if certain temperature levels are reached. The buzzer module provides auditory alarm feedback while the RGY LED shows the current temperature level.
- if temperature < 50, light: green
- if temperature >= 50 && <= 70, light yellow, buzzer alarm, send SMS (5 mins. cooldown)
- if temperature > 70, light: red
- Sends SMS alerts if there is change in current temperature threshold level.
# GSM Subscription Feature
You can message the device certain keywords to perform subscription actions. Device can store up to 5 max contacts/subscribers. Subscribers = contacts/phone numbers.
- `GSM SUB` - subscribes to the device, will receive alert messages. It will also notify the user if they are already subscribed if tyring to subscribe again.
- `GSM UNSUB` - unsubscribers to the device, will no longer receive alerts messages.
- `GSM LIST` - gets the list of the subscribers.
- `GSM STATUS` - gets the current temperature, threshold level, and LED status. 
# Components
- DS18B20 temperature Sensor (probe)
- ESP32 + Breakout board
- SIM800L EVB GSM module
- Buzzer module
- RGY LED module
# Hardware Notes
- SIM800L (or GSM modules in general) consumes a lot of current when transmitting/receiving messages. This can be mitigated by
    - using a DC boost converter + >=1000uf capacitor in parallel to the power pins of the GSM Module. Capacitor must be placed as close to the GSM module as possible, with thick traces for lesser resistance. Boost converter should be set at least 4.4v, max 5v (depending on the GSM module model requirements).
    - using 2 USB power sources (1 plugged to ESP32, 1 plugged to breakout board to power GSM module + components) along with the boost converter + smoothing capacitors.
