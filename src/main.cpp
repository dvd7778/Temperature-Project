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

const char* ssid = "LIB-2091526";
const char* password = "ckFcqkkB8mkz";
//const char* ssid = "Dvd";
//const char* password = "Mini7313";
const char *MQTT_Broker = "44.212.191.85";
const char *pubTopic = "Temperature";
const char *subTopic = "config";
const char *maxmintempTopic = "maxmintemp";
const char *currentTopic = "current";
const char *roomNameTopic = "roomName";
const int MQTT_Port = 1883;

struct Temp {
  float voltage;
  float actualTemp = 0;
};

struct Room {
  Temp maxTemp;
  Temp minTemp;
  Temp currentTemp;
  char roomName[20];
  bool b = true;
};

WiFiClient esp32Client;
PubSubClient client(esp32Client);
Room R;
//char msg[80];

void calibration(){
  char msg[80];
  sprintf(msg, "%s", "Which Room is this?");
  client.publish(currentTopic, msg, false);
  client.subscribe(maxmintempTopic);
 /*
  //Saves the number of the room the ESP32 is in
  //sprintf(msg, "%s", "What Room is this?");
 // client.publish(currentTopic, msg, false);
 // client.subscribe(maxmintempTopic);
  printf("\n\nWhat Room is this?\n");
  while(Serial.available() == 0){}
  R.roomName = Serial.parseInt();

  delay(500);
  //Measures and saves the voltage and value of a low temp
  sprintf(msg, "%s", "Apply a low temperature and input that temperature: ");
  client.publish(currentTopic, msg, false);
  client.subscribe(maxmintempTopic);
  printf("\nPlease apply a low temperature to the thermistor, and input that temperature:\n");
  while(Serial.available() == 0){}
  R.minTemp.actualTemp = Serial.parseFloat();
  R.minTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

  delay(500);
  //Measures and saves the voltage and value of a high temp
  sprintf(msg, "%s", "Apply a high temperature and input that temperature: ");
  client.publish(currentTopic, msg, false);
  client.subscribe(maxmintempTopic);
  printf("\nPlease apply a high temperature to the thermistor, and input that temperature:\n");    
  while(Serial.available() == 0){}
  R.maxTemp.actualTemp = Serial.parseFloat();
  R.maxTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7); 
 */
}

void measure(void *pvParameter){
  char msg[50];

  while(1){
    R.currentTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);
    R.currentTemp.actualTemp = R.minTemp.actualTemp + (R.currentTemp.voltage - R.minTemp.voltage) * ((R.maxTemp.actualTemp - R.minTemp.actualTemp) / (R.maxTemp.voltage - R.minTemp.voltage));

    printf("\n%f, %f\n", R.maxTemp.actualTemp, R.maxTemp.voltage);
    printf("%f, %f\n", R.minTemp.actualTemp, R.minTemp.voltage);
    printf("%f, %f\n", R.currentTemp.actualTemp, R.currentTemp.voltage);
    //printf("%f\n", R.currentTemp.voltage);
    
    //sprintf(msg, "%s%d%s%f", "Room: ", 204, " Temp: ", R.currentTemp.voltage);
    //sprintf(msg, "%d", R.roomName);
    client.publish(roomNameTopic, R.roomName, false);

    sprintf(msg, "%f", R.currentTemp.actualTemp);
    client.publish(pubTopic, msg, false);

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
  //Prints messages recieved from the MQTT Broker to the serial port
  /*Serial.println();
  Serial.print("Message[");
  Serial.print(topic);
  Serial.print("] ");

  for(int i=0; i< length; i++){
    Serial.print((char)payload[i]);
  }

  Serial.println();
  */
 ///*
/*
  if(charPayload[0] == '#'){
    charPayload++;
    R.roomName = atoi(charPayload);
    */

  char* charPayload = (char *)payload;
  int temp = -100;
  if(isDigit(charPayload[0]))
    temp = atoi(charPayload);

  if(!isDigit(charPayload[0])){
    for(int i=0; i< length; i++){
      R.roomName[i] = (char)payload[i];
    }

    char msg[80];
    sprintf(msg, "%s", "Apply a high or low temperature to the thermistor and input that value:");
    client.publish(currentTopic, msg, false);

  } else if(temp >= 30){
    R.maxTemp.actualTemp = temp;
    R.maxTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

    if(R.minTemp.actualTemp == 0){
      char msg[80];
      sprintf(msg, "%s", "Apply a low temperature to the thermistor and input that value:");
      client.publish(currentTopic, msg, false);
    } else {
      char msg[80];
      sprintf(msg, "%s", "Only input a temperature to recalibrate the highest or lowest temperature");
      client.publish(currentTopic, msg, false);
    }
    

  } else if(temp < 30){
    R.minTemp.actualTemp = temp;
    R.minTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

    if(R.maxTemp.actualTemp == 0){
      char msg[80];
      sprintf(msg, "%s", "Apply a high temperature to the thermistor and input that value:");
      client.publish(currentTopic, msg, false);
    } else {
      char msg[80];
      sprintf(msg, "%s", "Only input a temperature to recalibrate the highest or lowest temperature");
      client.publish(currentTopic, msg, false);
    }
  }
 // */
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
  client.setCallback(callback);
  connectToMQTT();
}

void setup() {
  // put your setup code here, to run once:
  //Initializes serial port
  Serial.begin(115200);

  //Initializes LEDs
  gpio_reset_pin(PIN_LED);
  gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LED, ON);

  //Initializes Analog to digital measurement
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

  //Initializes WiFi and MQTT Broker connection
  initializeMQTT();

  //Calibrates the temperature measurement
  calibration();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!client.connected())
    connectToMQTT();
  client.loop();

  if(R.maxTemp.actualTemp != 0 && R.minTemp.actualTemp != 0 && R.b){
    R.b = false;
    xTaskCreate(&measure, "measure", 2048, NULL, 5, NULL);
  }
}