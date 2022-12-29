#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

const char* ssid = "Wokwi-GUEST";
const char* password = "";
//***Set server***
const char* mqttServer = "broker.mqtt-dashboard.com"; 
WiFiClient espClient;
PubSubClient client(espClient);

//Init val
int port = 1883;
int trigPin = 15;
int echoPin = 2;
int DHTPin = 12;
int buzzerPin = 5;
int distanceToAlarm=100;
int led1 = 13;
int led2 = 4;
int ledState1 = LOW;
int ledState2 = LOW;
bool isOn = true;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;


DHTesp dhtSensor;

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void mqttReconnect() {
  while(!client.connected()) {
    Serial.println("Attemping MQTT connection...");
    //***Change "123456789" by your student id***
    if(client.connect("438_542_627")) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      client.subscribe("438_542_627/text");
      client.subscribe("438_542_627/ledAndBuzzer");
    }
    else {
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  dhtSensor.setup(DHTPin, DHTesp::DHT22);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  wifiConnect();
  client.setServer(mqttServer, port);
  client.setCallback(callback);
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
  //Serial.println(topic);
  String strMsg;
  for(int i=0; i<length; i++) {
    strMsg += (char)message[i];
  }
  //Serial.println(strMsg);
  //***Insert code here to control other devices***
  if(strMsg == "onOff"){
    isOn = !isOn;
    Serial.println(isOn);
  }else{
    distanceToAlarm = strMsg.toInt();
    Serial.print("Changed distance to: ");
    Serial.println(distanceToAlarm);
  }
  
}

long getDistance(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distanceCm=duration*0.034/2;
  return distanceCm;
}

void loop() {
  //Connect to wifi
  if(!client.connected()) {
    mqttReconnect();
  }
  client.loop();

  //Get distance and send to node red
  long distanceR=getDistance();
  char dist[50];
  char temp[50];
  char humi[50];
  char allData[100];
  unsigned long currentMillis = millis();
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  if (currentMillis - previousMillis >= 1000){
    previousMillis = currentMillis;
    sprintf(dist, "%d",distanceR);
    client.publish("438_542_627/disN", dist);
    Serial.println("Distance: " + String(distanceR));

    //Send Temperature and Humidity
    Serial.println("Temperature: " + String(data.temperature, 2) + "℃");
    sprintf(temp, "%.2f",data.temperature);
    client.publish("438_542_627/temp", temp);

    Serial.println("Humidity: " + String(data.humidity, 1) + "%");
    sprintf(humi, "%.2f",data.humidity);
    client.publish("438_542_627/humi", humi);

    strcpy(allData, dist);
    strcat(allData, ",");
    strcat(allData, temp);
    strcat(allData, ",");
    strcat(allData, humi);
    Serial.println(allData);
    client.publish("438_542_627/allData", allData);
    Serial.println("---");
  }

  if (currentMillis - previousMillis3 >= 60000){
    previousMillis3 = currentMillis;
    sprintf(dist, "%d",distanceR);
    sprintf(temp, "%.2f",data.temperature);
    sprintf(humi, "%.2f",data.humidity);

    strcpy(allData, dist);
    strcat(allData, ",");
    strcat(allData, temp);
    strcat(allData, ",");
    strcat(allData, humi);
    client.publish("438_542_627/allDataAfter1Minute", allData);
  }

  if(data.temperature < 40){
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
  }

  if (isOn){
    if(distanceR < distanceToAlarm){
      if (currentMillis - previousMillis1 >= distanceR*10) {
        previousMillis1 = currentMillis;

        if (ledState1 == LOW) {
          ledState1 = HIGH;
          ledState2 = LOW;
        } else {
          ledState1 = LOW;
          ledState2 = HIGH;
        }

        digitalWrite(led1, ledState1);
        digitalWrite(led2, ledState2);
        tone(buzzerPin, 600, 100);
      }
    }
    
    if(data.temperature >= 40){
      if (currentMillis - previousMillis2 >= 500) {
        previousMillis2 = currentMillis;

        if (ledState1 == LOW) {
          ledState1 = HIGH;
          ledState2 = LOW;
        } else {
          ledState1 = LOW;
          ledState2 = HIGH;
        }

        digitalWrite(led1, ledState1);
        digitalWrite(led2, ledState2);
        tone(buzzerPin, 600, 100);
      }
    }
  }
}

