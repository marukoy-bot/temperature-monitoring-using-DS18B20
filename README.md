# Temperature Monitoring and Alarm System
Monitors temperature changes every 1 second and sends an SMS alert if certain temeprature levels are reached. The buzzer module provides auditory alarm feedback while the RGY LED shows the current temeprature level.
- if temperature < 50, light: green
- if tempereature >= 50 && <= 70, light yellow, buzzer alarm, send SMS (5 mins. cooldown)
- if temperature > 70, light: red
# Components
- DS18B20 temperature Sensor
- Arduino UNO
- SIM800L GSM module
- Buzzer module
- RGY LED module
