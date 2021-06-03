#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const char *WIFI_SSID = "Xiaomi";
const char *WIFI_PASSWORD = "123456qq";

// MQTT: ID, server IP, port, username and password

const PROGMEM char *MQTT_CLIENT_ID = "office_rgb_light";
const PROGMEM char *MQTT_SERVER_IP = "192.168.31.130";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char *MQTT_USER = "admin";
const PROGMEM char *MQTT_PASSWORD = "123456qq";

// MQTT topics
// state
// 获取rgb灯的状态并设置
const PROGMEM char *MQTT_LIGHT_STATE_TOPIC = "/light/rgb/rgb1/getOn";
// 返回rgb灯灯状态
const PROGMEM char *MQTT_LIGHT_COMMAND_TOPIC = "/light/rgb/rgb1/setOn";

// colors (rgb)
const PROGMEM char *MQTT_LIGHT_RGB_STATE_TOPIC = "/light/rgb/rgb1/getRGB";
const PROGMEM char *MQTT_LIGHT_RGB_COMMAND_TOPIC = "/light/rgb/rgb1/setRGB";

// payloads by  default (on/off)
const PROGMEM char *LIGHT_ON = "true";
const PROGMEM char *LIGHT_OFF = "false";

// variables used to store the state, the brightness and the color of the light
boolean m_rgb_state = false;
uint8_t m_rgb_brightness = 100;
uint8_t m_rgb_red = 255;
uint8_t m_rgb_green = 255;
uint8_t m_rgb_blue = 255;

// pins used for the rgb led (PWM)
const PROGMEM uint8_t RGB_LIGHT_RED_PIN = 4;
const PROGMEM uint8_t RGB_LIGHT_GREEN_PIN = 5;
const PROGMEM uint8_t RGB_LIGHT_BLUE_PIN = 14;

// buffer used to send/receive data with MQTT

const uint8_t MSG_BUFFER_SIZE = 20;
char m_msg_buffer[MSG_BUFFER_SIZE];

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// function called to adapt the brightness and the color of the led
void setColor(uint8_t p_red, uint8_t p_green, uint8_t p_blue)
{
  analogWrite(RGB_LIGHT_RED_PIN, p_red);
  analogWrite(RGB_LIGHT_GREEN_PIN, p_green);
  analogWrite(RGB_LIGHT_BLUE_PIN, p_blue);
}

void publishRGBState()
{
  if (m_rgb_state)
  {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_ON, true);
  }
  else
  {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_OFF, true);
  }
}

void publishRGBColor()
{
  sniprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_rgb_red, m_rgb_green, m_rgb_blue);
  client.publish(MQTT_LIGHT_RGB_STATE_TOPIC, m_msg_buffer, true);
}

// function called when a MQTT message arrived
void callback(char *p_topic, byte *p_payload, unsigned int p_length)
{
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++)
  {
    payload.concat((char)p_payload[i]);
  }
  Serial.println(payload);
  // handle message topic
  if (String(MQTT_LIGHT_COMMAND_TOPIC).equals(p_topic))
  {
    if (payload.equals(String(LIGHT_ON)))
    {
      if (m_rgb_state != true)
      {
        m_rgb_state = true;
        setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
        publishRGBState();
      }
    }
    else if (payload.equals(String(LIGHT_OFF)))
    {
      if (m_rgb_state != false)
      {
        m_rgb_state = false;
        setColor(0, 0, 0);
        publishRGBState();
      }
    }
  }
  else if (String(MQTT_LIGHT_RGB_COMMAND_TOPIC).equals(p_topic))
  {
    // get the position of the first and second commas
    uint8_t firstIndex = payload.indexOf(",");
    uint8_t lastIndex = payload.lastIndexOf(",");

    uint8_t rgb_red = payload.substring(0, firstIndex).toInt();
    if (rgb_red < 0 || rgb_red > 255)
    {
      return;
    }
    else
    {
      m_rgb_red = rgb_red;
    }

    uint8_t rgb_green = payload.substring(firstIndex + 1, lastIndex).toInt();
    if (rgb_green < 0 || rgb_green > 255)
    {
      return;
    }
    else
    {
      m_rgb_green = rgb_green;
    }

    uint8_t rgb_blue = payload.substring(lastIndex + 1).toInt();
    if (rgb_blue < 0 || rgb_blue > 255)
    {
      return;
    }
    else
    {
      m_rgb_blue = rgb_blue;
    }

    setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
    publishRGBColor();
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID))
    {
      Serial.println("INFO: connected");

      // Once connected, publish an announcement...
      // publish the initial values
      publishRGBState();
      publishRGBColor();

      // ... and resubscribe
      client.subscribe(MQTT_LIGHT_COMMAND_TOPIC);
      client.subscribe(MQTT_LIGHT_RGB_COMMAND_TOPIC);
    }
    else
    {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  // init the serial
  Serial.begin(115200);

  // init the RGB led
  pinMode(RGB_LIGHT_BLUE_PIN, OUTPUT);
  pinMode(RGB_LIGHT_RED_PIN, OUTPUT);
  pinMode(RGB_LIGHT_GREEN_PIN, OUTPUT);
  analogWriteRange(255);
  setColor(0, 0, 0);

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}