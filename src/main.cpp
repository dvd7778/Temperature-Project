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
const char *MQTT_Broker = "44.212.191.85";
const char *pubTopic = "Temperature";
const char *subTopic = "config";
const int MQTT_Port = 1883;

typedef struct Temp {
  int voltage;
  int actualTemp;
};

typedef struct Room {
  Temp maxTemp;
  Temp minTemp;
  Temp currentTemp;
  int roomNum;
};

WiFiClient esp32Client;
PubSubClient client(esp32Client);
Room R;

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
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0);

  //Initializes WiFi and MQTT Broker connection
  initializeMQTT();

  //Creates a task to measure the temperature and publish the temp to the server
  xTaskCreate(&measure, "measure", 2048, NULL, 5, NULL);

  //Calibrates the temperature measurement
  calibration();
}

void calibration(){
  //Saves the number of the room the ESP32 is in
  printf("\n\nWhat room is this?\n\n");
  scanf("%d", &R.roomNum);
  
  //Measures and saves the voltage and value of a low temp
  printf("\nPlease apply a low temperature to the thermistor, \nand input that temperature: \n");
  scanf("%d", &R.minTemp.actualTemp);
  R.minTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);

  //Measures and saves the voltage and value of a high temp
  printf("\nPlease apply a high temperature to the thermistor, \nand input that temperature: \n");    
  scanf("%d", &R.maxTemp.actualTemp);
  R.maxTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);
}

void measure(void *pvParameter){
  char msg[50];

  while(1){
    R.currentTemp.voltage = adc1_get_raw(ADC1_CHANNEL_7);
    R.currentTemp.actualTemp = R.minTemp.actualTemp + (R.currentTemp.voltage - R.minTemp.voltage) * ((R.maxTemp.actualTemp - R.minTemp.actualTemp) / (R.maxTemp.voltage - R.minTemp.voltage));

    sprintf(msg, "%s%d%S%d", "Room ", R.roomNum, " has a temperature of: ", R.currentTemp.actualTemp);
    client.publish(pubTopic, msg, false);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
  client.setCallback(callback);
  connectToMQTT();
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
  Serial.println();
  Serial.print("Message[");
  Serial.print(topic);
  Serial.print("] ");

  for(int i=0; i< length; i++){
    Serial.print((char)payload[i]);
  }

  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!client.connected())
    connectToMQTT();
  client.loop();
}