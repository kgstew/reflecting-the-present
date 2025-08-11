#include "patterns.h"
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <WebSocketsServer.h>

#define BRIGHTNESS 255
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_PINS 6

#define PIN1 5
#define PIN2 18
#define PIN3 19
#define PIN4 21
#define PIN5 22
#define PIN6 23

uint16_t strip_lengths[]
    = { 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122 };

CRGB pin1_leds[366];
CRGB pin2_leds[488];
CRGB pin3_leds[488];
CRGB pin4_leds[366];
CRGB pin5_leds[488];
CRGB pin6_leds[488];

PinConfig pin_configs[NUM_PINS]
    = { { PIN1, 3, 122, 366, pin1_leds }, { PIN2, 4, 122, 488, pin2_leds }, { PIN3, 4, 122, 488, pin3_leds },
          { PIN4, 3, 122, 366, pin4_leds }, { PIN5, 4, 122, 488, pin5_leds }, { PIN6, 4, 122, 488, pin6_leds } };

uint8_t strip_map[] = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5 };

unsigned long current_time;

// WiFi and WebSocket configuration
const char* ssid = "ReflectingThePresent";
const char* password = "lightshow2024";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Sensor ID to LED strip mapping
#define MAX_SENSORS 10
#define MAX_STRIPS_PER_SENSOR 5

struct SensorMapping {
    uint8_t sensor_id;
    uint8_t led_strips[MAX_STRIPS_PER_SENSOR];
    uint8_t num_strips;
    bool active;
    unsigned long last_trigger_time;
};

SensorMapping sensor_mappings[MAX_SENSORS] = {
    // Add your sensor mappings here manually
    {1, {0, 1, 2}, 3, true, 0},           // Sensor 1 -> strips 0,1,2
    {2, {5, 6}, 2, true, 0},              // Sensor 2 -> strips 5,6
    {3, {10, 11, 12, 13}, 4, true, 0},    // Sensor 3 -> strips 10,11,12,13
    // Add more mappings as needed
};

// Function declarations
void handleSensorMessage(String message);
void setupWiFiAndWebSocket();

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
            
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
            break;
            
        case WStype_TEXT:
            Serial.printf("[%u] Received: %s\n", num, payload);
            handleSensorMessage((char*)payload);
            break;
            
        default:
            break;
    }
}

// Handle incoming sensor messages
void handleSensorMessage(String message) {
    JSONVar json = JSON.parse(message);
    
    if (JSON.typeof(json) == "undefined") {
        Serial.println("Failed to parse JSON");
        return;
    }
    
    if (json.hasOwnProperty("sensorId") && json.hasOwnProperty("timestamp")) {
        int sensor_id = (int)json["sensorId"];
        unsigned long timestamp = (unsigned long)json["timestamp"];
        
        Serial.print("Sensor message received - ID: ");
        Serial.print(sensor_id);
        Serial.print(", Timestamp: ");
        Serial.println(timestamp);
        
        // Find the sensor mapping
        for (uint8_t i = 0; i < MAX_SENSORS; i++) {
            if (sensor_mappings[i].active && sensor_mappings[i].sensor_id == sensor_id) {
                // Check if enough time has passed since last trigger (15 seconds = 15000ms)
                unsigned long time_since_last = current_time - sensor_mappings[i].last_trigger_time;
                
                if (sensor_mappings[i].last_trigger_time == 0 || time_since_last >= 15000) {
                    // Update last trigger time
                    sensor_mappings[i].last_trigger_time = current_time;
                    
                    // Trigger FlashBulb on mapped strips
                    addFlashBulbPattern(sensor_mappings[i].led_strips, sensor_mappings[i].num_strips);
                    uint8_t pattern_index = flashbulb_manager.pattern_count - 1;
                    triggerFlashBulb(pattern_index);
                    
                    Serial.print("FlashBulb triggered on ");
                    Serial.print(sensor_mappings[i].num_strips);
                    Serial.print(" strips: ");
                    for (uint8_t j = 0; j < sensor_mappings[i].num_strips; j++) {
                        Serial.print(sensor_mappings[i].led_strips[j]);
                        if (j < sensor_mappings[i].num_strips - 1) Serial.print(", ");
                    }
                    Serial.println();
                } else {
                    // Too soon since last trigger
                    unsigned long wait_time = 15000 - time_since_last;
                    Serial.print("Sensor ");
                    Serial.print(sensor_id);
                    Serial.print(" trigger ignored - wait ");
                    Serial.print(wait_time / 1000);
                    Serial.println(" more seconds");
                }
                
                break;
            }
        }
    }
}

// Setup WiFi Access Point and WebSocket server
void setupWiFiAndWebSocket() {
    // Set up WiFi Access Point
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    // Setup WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("WebSocket server started on port 81");
    
    // Setup basic web server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html><head><title>Reflecting The Present</title></head><body>";
        html += "<h1>Reflecting The Present - LED Control</h1>";
        html += "<p>WebSocket server running on port 81</p>";
        html += "<p>Send JSON messages with format: {\"sensorId\": 1, \"timestamp\": 1234567890}</p>";
        html += "<h2>Sensor Mappings:</h2><ul>";
        
        for (uint8_t i = 0; i < MAX_SENSORS; i++) {
            if (sensor_mappings[i].active) {
                html += "<li>Sensor " + String(sensor_mappings[i].sensor_id) + " -> Strips: ";
                for (uint8_t j = 0; j < sensor_mappings[i].num_strips; j++) {
                    html += String(sensor_mappings[i].led_strips[j]);
                    if (j < sensor_mappings[i].num_strips - 1) html += ", ";
                }
                html += "</li>";
            }
        }
        html += "</ul></body></html>";
        request->send(200, "text/html", html);
    });
    
    server.begin();
    Serial.println("HTTP server started on port 80");
}

void setup()
{
    Serial.begin(115200);
    Serial.print("Connecting");
    delay(3000);

    FastLED.addLeds<LED_TYPE, PIN1, COLOR_ORDER>(pin1_leds, pin_configs[0].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN2, COLOR_ORDER>(pin2_leds, pin_configs[1].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN3, COLOR_ORDER>(pin3_leds, pin_configs[2].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN4, COLOR_ORDER>(pin4_leds, pin_configs[3].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN5, COLOR_ORDER>(pin5_leds, pin_configs[4].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN6, COLOR_ORDER>(pin6_leds, pin_configs[5].total_leds).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("LED configuration:");
    uint16_t total_leds = 0;
    uint8_t strip_index = 0;

    for (int pin = 0; pin < NUM_PINS; pin++) {
        Serial.printf("Pin %d: %d LEDs (%d strips) - Strips: ", pin_configs[pin].pin, pin_configs[pin].total_leds,
            pin_configs[pin].num_strips);

        for (int s = 0; s < pin_configs[pin].num_strips; s++) {
            Serial.printf("%d", strip_lengths[strip_index]);
            if (s < pin_configs[pin].num_strips - 1)
                Serial.print(", ");
            strip_index++;
        }
        Serial.println();
        total_leds += pin_configs[pin].total_leds;
    }

    Serial.printf("Total: %d LEDs across %d strips\n", total_leds, strip_index);
    Serial.println("Expected: 3+4+4+3+4+4 = 22 strips total");

    // Setup the pattern program
    setupPatternProgram();

    // Initialize FlashBulb system
    initFlashBulbManager();
    
    // Setup WiFi Access Point and WebSocket server
    setupWiFiAndWebSocket();
    
    Serial.println("=== System Ready ===");
    Serial.println("Connect to WiFi: ReflectingThePresent");
    Serial.println("Password: lightshow2024");
    Serial.println("WebSocket: ws://192.168.4.1:81");
    Serial.println("Web interface: http://192.168.4.1");
}

void demoFlashBulb()
{
    static unsigned long last_demo = 0;

    if (current_time - last_demo >= 10000) { // Every 5 seconds
        last_demo = current_time;

        // Random number of strips (0-5)
        uint8_t num_random_strips = random(0, 6);

        if (num_random_strips > 0) {
            // Create random strip array
            uint8_t random_strips[5];
            for (uint8_t i = 0; i < num_random_strips; i++) {
                random_strips[i] = random(0, 22); // Random strip 0-21
            }

            // Add temporary FlashBulb pattern
            addFlashBulbPattern(random_strips, num_random_strips);

            // Trigger the newly added pattern (it will be the last one)
            uint8_t pattern_index = flashbulb_manager.pattern_count - 1;
            triggerFlashBulb(pattern_index);

            Serial.print("FlashBulb demo triggered on ");
            Serial.print(num_random_strips);
            Serial.print(" strips: ");
            for (uint8_t i = 0; i < num_random_strips; i++) {
                Serial.print(random_strips[i]);
                if (i < num_random_strips - 1)
                    Serial.print(", ");
            }
            Serial.println();
        } else {
            Serial.println("FlashBulb demo: No strips selected this time");
        }
    }
}

void loop()
{
    current_time = millis();

    // Handle WebSocket events
    webSocket.loop();

    // Run demo FlashBulb trigger (optional - comment out when using real sensors)
    demoFlashBulb();

    // Run the queued pattern program
    runQueuedChasePattern();
}
