#include <stdbool.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "WiFiConfig.h"

#define SERIAL_RATE 115200
#define DELAY_BEFORE_REBOOT 2000

#define LED_PIN D4
#define ONEWIRE_BUS_PIN D7
#define RELAY0_PIN D3
#define RELAY1_PIN D2

#define PORT 6666

#define PROTOCOL_IDENTIFIER_REQUEST  0x00
#define PROTOCOL_IDENTIFIER_RESPONSE 0x7F
#define RESPONSE_CODE_OK  0x00
#define RESPONSE_CODE_NOK 0x7F

#define GET_BIT(value, pos) ((value) & (1 << (pos)))
#define SET_BIT(value, pos) ((value) | (1 << (pos)))

OneWire oneWireBus(ONEWIRE_BUS_PIN);
DallasTemperature tempSensors(&oneWireBus);

WiFiServer server(PORT);

typedef struct {
  uint8_t protocolIdentifier;
  uint8_t relayStates;
} Request;

typedef struct {
  uint8_t protocolIdentifier;
  uint8_t responseCode;
  uint8_t relayStates;
  float temperature;
} Response;

void setup() {
  setup_pins(OUTPUT, 3, LED_PIN, RELAY0_PIN, RELAY1_PIN);
  setup_serial(SERIAL_RATE);
  if (!setup_wifi(WIFI_SSID, WIFI_PASSWORD)) {
    reboot(DELAY_BEFORE_REBOOT);
  }

  tempSensors.begin();
  server.begin();
  Serial.printf("Listening at port %d\n\n", PORT);
}

void setup_pins(uint8_t mode, int n, ...) {
  va_list ap;
  va_start(ap, n);
  for (int i = 0; i < n; i++) {
    pinMode(va_arg(ap, int), mode);
  }
  va_end(ap);
}

void setup_serial(int serial_rate) {
  Serial.begin(serial_rate);
  Serial.printf("\n\nSerial communication set up at %d baud\n\n", serial_rate);
}

bool setup_wifi(const char* ssid, const char* password) {
  Serial.printf("Setting Wi-Fi mode to WIFI_STA\n\n");  
  if (!WiFi.mode(WIFI_STA)) {
    Serial.printf("Wi-Fi setup failed!\n\n");
    return false;
  }
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.printDiag(Serial);
    Serial.printf("Wi-Fi setup failed!\n\n");
    return false;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  return true;
}

void reboot(int boot_delay) {
    Serial.printf("Rebooting in %d ms...", boot_delay);
    delay(boot_delay);
    ESP.restart();
}

void blink(int wait, int times, int pin) 
{
  for (int i=0; i < times; i++) {
    digitalWrite(pin, !digitalRead(pin));
    delay(wait);
  }  
}

int receive_request(WiFiClient client, uint8_t *buffer, int length)
{
  int received = 0;
  while (client && client.available() && received < length) buffer[received++] = client.read();
  return received;
}

bool validate_request(Request request, int received)
{ 
  return received == sizeof(request) && request.protocolIdentifier == PROTOCOL_IDENTIFIER_REQUEST;
}

void set_relay_states(uint8_t states) 
{
  digitalWrite(RELAY0_PIN, GET_BIT(states, 0));
  digitalWrite(RELAY1_PIN, GET_BIT(states, 1));  
}

uint8_t get_relay_states()
{
  uint8_t states = 0x00;
  states = digitalRead(RELAY0_PIN) ? SET_BIT(states, 0) : states;
  states = digitalRead(RELAY1_PIN) ? SET_BIT(states, 1) : states;
  return states;
}

uint8_t process_request(Request request, bool valid)
{
  if (valid) {
    blink(20, 4, LED_PIN);      
    set_relay_states(request.relayStates);
    return RESPONSE_CODE_OK;
  } else {
    blink(40, 4, LED_PIN);
    return RESPONSE_CODE_NOK;
  }
}

void send_response(WiFiClient client, uint8_t response_code, uint8_t relay_states, float temperature)
{
    Response response;
    response.protocolIdentifier = PROTOCOL_IDENTIFIER_RESPONSE;
    response.responseCode = response_code;
    response.relayStates = relay_states;
    response.temperature = temperature;
    client.write((uint8_t*) &response, sizeof(response));
}

void loop() {  
  digitalWrite(LED_PIN, HIGH);
  tempSensors.requestTemperatures();
  
  WiFiClient client = server.available();
  if (client && client.connected()) {
    Serial.print("Client connected ");
    Serial.print(client.remoteIP());
    Serial.print(":");
    Serial.print(client.remotePort());
    Serial.print("->");
    Serial.print(client.localIP());
    Serial.print(":");
    Serial.print(client.localPort());
    Serial.print(" - ");
    
    Request request;
    int received = receive_request(client, (uint8_t*) &request, sizeof(request));
    bool valid_request = validate_request(request, received);
    Serial.println(valid_request ? "valid request." : "INVALID request.");

    uint8_t response_code = process_request(request, valid_request);

    send_response(client, response_code, get_relay_states(), tempSensors.getTempCByIndex(0));
    
    client.stop();
  }
}
