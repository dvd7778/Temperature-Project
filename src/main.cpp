#include <Arduino.h>
#include <stdio.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <PubSubClient.h>
#include <driver/adc.h>

#define PIN_LED GPIO_NUM_4
#define ON 1
#define OFF 0

//const char* ssid = "LIB-2091526";
//const char* password = "ckFcqkkB8mkz";
const char* ssid = "Dvd";
const char* password = "Mini7313";
const char *MQTT_Broker = "44.212.191.85";
const char *TemperatureTopic = "Temperature";
const char *subTopic = "config";
const char *maxmintempTopic = "maxmintemp";
const char *currentTopic = "current";
const char *roomNameTopic = "roomName";
const int MQTT_Port = 1883;

//Temperature struct to store the voltage and the actual temperature
struct Temp {
  float voltage;
  float actualTemp = 0;
};

//Room struct where a maximum, minimum and current temperature will be stored in addition to the room name and a boolean value to not repeat the creation of the measure task
struct Room {
  Temp maxTemp;
  Temp minTemp;
  Temp currentTemp;
  char roomName[40];
  bool b = true;
};

//WiFi client and PubSubClient client objects created
WiFiClient esp32Client;
PubSubClient client(esp32Client);

//Created the Room object
Room R;
char msg[80];

void measure(void *pvParameter){
  char output[50];

  while(1){
    //Measures the voltage in the GPIO 35
    R.currentTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

    //Applies the interpollation formula to calculate the current temperature of the room
    R.currentTemp.actualTemp = R.minTemp.actualTemp + (R.currentTemp.voltage - R.minTemp.voltage) * ((R.maxTemp.actualTemp - R.minTemp.actualTemp) / (R.maxTemp.voltage - R.minTemp.voltage));

    //FOR DEBUGGING PURPOSES (prints in the serial terminal the high, low and current saved voltage and temperature)
    printf("\n%f, %f\n", R.maxTemp.actualTemp, R.maxTemp.voltage);
    printf("%f, %f\n", R.minTemp.actualTemp, R.minTemp.voltage);
    printf("%f, %f\n", R.currentTemp.actualTemp, R.currentTemp.voltage);
    
    //Publishes the room name with the roomName topic through MQTT
    client.publish(roomNameTopic, R.roomName, false);

    //Writes the current temperature in the msg array and publishes that temperature with the Temperature topic through MQTT
    sprintf(output, "%f", R.currentTemp.actualTemp);
    client.publish(TemperatureTopic, output, false);

    //A delay to perform the task
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void connectToMQTT(){
  //If the client is not connected to MQTT it will attempt to connect until it connects
  while(!client.connected()){
    if(client.connect("ESP32_clientID"))
      client.subscribe(subTopic);
    else 
      delay(3000);
  }
}

void callback(char* topic, byte* payload, unsigned int length){

  char* charPayload = (char *)payload;
  int temp = -273;

  //If the payload sent through MQTT is a digit then the digit is saved in the temp object
  if(isDigit(charPayload[0]))
    temp = atoi(charPayload);

  //If the payload is not a digit then it is saved in a char array as the name of the room
  if(!isDigit(charPayload[0])){
    for(int i=0; i< length; i++){
      R.roomName[i] = (char)payload[i];
    }

    //Will prompt the user throug MQTT to input a high or low temperature next
    sprintf(msg, "%s", "Apply a high or low temperature to the thermistor and input that value:");
    client.publish(currentTopic, msg, false);
  
  //If the value saved in the temp object is greater than or equal to 30 then it is saved a as a high temperature
  } else if(temp >= 30){
    R.maxTemp.actualTemp = temp;
    R.maxTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

    //If the low temperature has not been inputted then it will prompt the user through MQTT to input a low temperature.
    if(R.minTemp.actualTemp == 0){
      sprintf(msg, "%s", "Apply a low temperature to the thermistor and input that value:");
      client.publish(currentTopic, msg, false);

    //If the low temperature has already been inputted then it will prompt the user through MQTT to input a high or low value only to recalibrate
    } else {
      sprintf(msg, "%s", "Only input a temperature to recalibrate the highest or lowest temperature");
      client.publish(currentTopic, msg, false);
    }
    
  //If the value stored in the temp object is less than 30, then it is considered a low temperature and saved as one
  } else if(temp < 30 && temp != -273){
    R.minTemp.actualTemp = temp;
    R.minTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

    //If a high temperature has yet to be inputted then it will prompt the user through MQTT to input a high temperature
    if(R.maxTemp.actualTemp == 0){
      sprintf(msg, "%s", "Apply a high temperature to the thermistor and input that value:");
      client.publish(currentTopic, msg, false);

    //If the high temperature has already been inputted then it will prompt the user through MQTT to only input a high or low temperature to recalibrate it
    } else {
      sprintf(msg, "%s", "Only input a temperature to recalibrate the highest or lowest temperature");
      client.publish(currentTopic, msg, false);
    }
  }
}

void initializeMQTT(){
  //Initialize WiFi with SSID and password
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    gpio_set_level(PIN_LED, OFF);
  }
  
  //Connects to the MQTT Broker
  client.setServer(MQTT_Broker, MQTT_Port);
  //Set the callback function to occur once a message is recieved through MQTT
  client.setCallback(callback);
  //Calls the connectToMQTT function to finish connecting
  connectToMQTT();
}

void setup() {
  //Initializes serial port
  Serial.begin(115200);

  //Initializes LEDs to know when the ESP32 is connected to WiFi
  gpio_reset_pin(PIN_LED);
  gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LED, ON);

  //Initializes Analog to digital measurement
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

  //Initializes WiFi and MQTT Broker connection
  initializeMQTT();

  //Initiates the calibration by prompting the user for the room name
  sprintf(msg, "%s", "Which Room is this? To write a number, please enter a '#' at the begining.");
  client.publish(currentTopic, msg, false);
  client.subscribe(maxmintempTopic);
}

void loop() {
  //If the client disconnected, then it will continously attempt to connect until connected 
  if(!client.connected())
    connectToMQTT();
  client.loop();

  //Once a high and low temperature are inputted then a measure task will be created to measure and publish te temperature of the room
  if(R.maxTemp.actualTemp != 0 && R.minTemp.actualTemp != 0 && R.b){
    R.b = false;
    xTaskCreate(&measure, "measure", 2048, NULL, 5, NULL);
  }
}