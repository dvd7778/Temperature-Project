# Is the Air Conditioner on? Final Project
INEL 4206 sec 030
Due: December 12, 2022

# Project Participants:
David A. Castillo Martínez (david.castillo1@upr.edu)
Marielys Quiñones Pérez (marielys.quinones@upr.edu)

# Project Description:
This project utilizes an ESP32 microprocessor to measure the voltage across a thermistor, and depending on the voltage read, the temperature corresponding to that voltage is sent to an AWS Linux server. The information in the server is then accessed through node-red and displayed on a dashboard UI through node-red. The project works by sending information through WiFi and MQTT. The project works by applying a "high" temperature to the thermistor and inputting the applied temperature to node-red, then the same thing is done for a "low" temperature. After this calibration process, the ESP32 will interpolate between both inputted temperatures and return the current temperature in the room. In addition, anyone who sets up a smartphone shortcut is able to ask Siri or Ok Google if the air conditioner is on in the room the ESP32 is in, and Siri or Ok Google will reply with if the air conditioner in said room is on or off (depending on the current temperature in the room).

# Materials:
• ESP32 Freenove Basic Starter Kit
    ○ Link to buy the kit: https://www.amazon.com/ESP32-WROVER-Contained-Compatible-Bluetooth-Tutorials/dp/B09BC1N9LL/ref=sr_1_1?crid=1FYAX49OTDYJY&keywords=esp32+basic+starter+kit&qid=1670518934&sprefix=esp32+basic+starter+kit%2Caps%2C122&sr=8-1
• Thermistor 
    ○ Found in the starter kit above
• Resistor
    ○ Found in the starter kit above
• AWS Lightsail Linux Cloud Server
    ○ Link to setup a Lightsail server: https://aws.amazon.com/lightsail/
• Node-red (installed in the Linux Server)
    ○ Link for instructions to install: https://nodered.org/

# Copyright:
Aditional trademarks of products used in this project:

• Node.js® - Trademark for Node.js - https://nodejs.org/en/
• Node-RED® - Trademark for Node-RED - https://nodered.org/
• ESPRESSIF® - Trademark for Espressif Systems (shanghai) Co. Ltd. - https://www.espressif.com/
• Arduino® - Trademark for Arduino - https://www.arduino.cc/