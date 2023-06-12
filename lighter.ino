#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h> 
#include <stdlib.h>
#include <string.h>
/* PIN LAYOUT OF ESP12E 
GPIO: General Purpose IN/OUT
We only need a few of the PWM supported general io pins, so we didn't list all of the pins.
Number on the right side stands for pin number (D1 is pin 5, D2 is 4 etc.)
D1	GPIO5 
D2	GPIO4
D3	GPIO0
D4	GPIO2
D5	GPIO14
D6	GPIO12
D7	GPIO13
...
*/

//Config variables
const char *SSID = "Seyfullah";
const char *password = "seyfullah54";
const char *mqttServer = "test.mosquitto.org";
const uint16_t mqttServerPort = 1883;
const char *mqttTopicIn = "rgb-in";
const char *mqttTopicOut = "rgb-out";

// globals
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
static int RED_PIN = 5;    // D1
static int GREEN_PIN = 4;  // D2
static int BLUE_PIN = 2;   // D4

static int RED_DUTY = 0;
static int GREEN_DUTY = 0;
static int BLUE_DUTY = 0;
const int MAX_DUTY = 255;  // also called PWM range
// we can change the PWM range by calling
// analogWriteRange(1023); e.g 10-bit precision
static int TIMER = 0;



int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

//function to run when a message comes from the topic
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message recieved on topic: '");
  Serial.print(topic);
  Serial.print("' with payload: '");
  // payload isn't a str but a byte array, so we convert each byte to a char then print it.
  // that's also a problem when we try to get a number from the payload because it isn't null-terminated.
  // so we convert it to a str.
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("'.");
  
  //convert bytes to str.
  char *payloadStr = (char *)malloc(length + 1);
  strncpy(payloadStr, (char *)payload, length);
  payloadStr[length] = '\0';
  
  //the payload is of the form rgbt(...)
  if (strncmp(payloadStr, "rgbt(", 4) == 0 && payloadStr[length - 1] == ')') {
    Serial.println("Calling rgbt function");
    processRGBT(payloadStr);
  }
  else { 
    //payload is of the form 'color duty_value'
    Serial.println("Calling single color function.");
    processSingleColor(payloadStr);
  }
  //publish the last rgb values and timer on 'rgb-out' topic.
  mqttClient.publish(mqttTopicOut, ("R: " + String(RED_DUTY) + " G: " + String(GREEN_DUTY) + " B: " + String(BLUE_DUTY) + " T: " + String(TIMER)).c_str());
  delay(TIMER * 1000);
  free(payloadStr);
  reset();
}


// our payload is of the form rgbt(red-pwm-value, green-pwm-value, blue-pwm-value, time-in-seconds)
void processRGBT(char *payloadStr) {
  int r, g, b, t;
  int i, j = 0;
  // remove spaces 
  for (i = 0; payloadStr[i] != '\0'; i++) {
    if (payloadStr[i] != ' ') {
      payloadStr[j] = payloadStr[i];
      j++;
    }
  }
  payloadStr[j] = '\0';

  Serial.print("Trimmed payload: ");
  Serial.println(payloadStr);
  // Get the color values to their corresponding variables
  sscanf(payloadStr, "rgbt(%d,%d,%d,%d)", &r, &g, &b, &t);
  // Call the RGB function with the extracted values
  rgbt(r, g, b, t);
}


// RGB Value and a timer in seconds
void rgbt(int r, int g, int b, int t) {

  // if color value is smaller than 0, don't change it.
  // this funclionality is useless in the current form of our project since we reset the PWM values after every command.
  RED_DUTY = (r < 0) ? RED_DUTY : min(r, MAX_DUTY);
  GREEN_DUTY= (g < 0) ? GREEN_DUTY : min(g, MAX_DUTY);
  BLUE_DUTY= (b < 0) ? BLUE_DUTY : min(b, MAX_DUTY);
  TIMER = t;
  
  analogWrite(RED_PIN, RED_DUTY);
  analogWrite(BLUE_PIN, BLUE_DUTY);
  analogWrite(GREEN_PIN, GREEN_DUTY);

}


//the message is of the form "color pwmValue time(in seconds)"
//we could also use underscore(_) instead of whitespaces
void processSingleColor(char *payloadStr) {
  int *pin = NULL;
  int *duty = NULL;
  switch (payloadStr[0]) {
    // payload is about red
    case 'r':
    case 'R':
      Serial.println("Red color picked.");
      pin = &RED_PIN;
      duty = &RED_DUTY;
      break;
    //payload is about green
    case 'g':
    case 'G':
      Serial.println("Green color picked.");
      pin = &GREEN_PIN;
      duty = &GREEN_DUTY;
      break;
    //payload is about blue
    case 'b':
    case 'B':
      Serial.println("Blue color picked.");
      pin = &BLUE_PIN;
      duty = &BLUE_DUTY;
      break;
    default: // ignore other cases and return early
      return;
  }
  // find the first whitespace (or underscore)
  // this index is where our pwmValue starts
  int i = 1;
  while (payloadStr[i] != ' ' && payloadStr[i != '_']) {
    i++;
  }
  
  // now find the second space's index
  // this is our time value in seconds
  int j = i+1;
  while (payloadStr[j] != ' ' && payloadStr[j != '_']) {
    j++;
  }
  // since atoi ends when it detects a '\0' we put
  // a NULL-terminator in place of the second space
  // to be able to use atoi in both our pwmValue and time
  payloadStr[j] = '\0';
  
  // now we can etract our values.
  *duty = min(atoi(&(payloadStr[i + 1])), MAX_DUTY); //pwm
  if (*duty < 0) {
    *duty = 0;
  }
  TIMER = atoi(&payloadStr[j + 1]); //timer
  Serial.print("Duty value: ");
  Serial.println(*duty);
  analogWrite(*pin, *duty);
}

// Reset pins 
void reset() {
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
}

// connect to wifi
void setupWiFi() {
  //print info about network connection
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(SSID);
  Serial.print(" with password ");
  Serial.println(password);

  WiFi.begin(SSID, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

// connect to the broker
void connect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String mqttClientId = "";
    if (mqttClient.connect(mqttClientId.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe(mqttTopicIn);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" will try again in 2 seconds");
      delay(2 * 1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  mqttClient.setServer(mqttServer, mqttServerPort);
  mqttClient.setCallback(callback);
  analogWrite(RED_PIN, RED_DUTY);
  analogWrite(GREEN_PIN, GREEN_DUTY);
  analogWrite(BLUE_PIN, BLUE_DUTY);
}

void loop() {
  if (!wifiClient.connected()) {
    Serial.println();
    Serial.println("WFi connection interrupted, trying again.");
    setupWiFi();
  }
  if (!mqttClient.connected()) {
    connect();
  }
  mqttClient.loop();
}