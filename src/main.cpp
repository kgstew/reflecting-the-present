#include "patterns.h"
#include <Arduino.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#define BRIGHTNESS 255
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_PINS 6

#define PIN1 13
#define PIN2 5
#define PIN3 19
#define PIN4 23
#define PIN5 18
#define PIN6 12

// LED arrays for each pin
CRGB pin1_leds[366];
CRGB pin2_leds[488];
CRGB pin3_leds[488];
CRGB pin4_leds[366];
CRGB pin5_leds[488];
CRGB pin6_leds[488];

// Pin configuration for FastLED setup (still needed for FastLED.addLeds calls)
PinConfig pin_configs[NUM_PINS]
    = { { PIN1, 3, 122, 366, pin1_leds }, { PIN2, 4, 122, 488, pin2_leds }, { PIN3, 4, 122, 488, pin3_leds },
          { PIN4, 3, 122, 366, pin4_leds }, { PIN5, 4, 122, 488, pin5_leds }, { PIN6, 4, 122, 488, pin6_leds } };

// Configure which strips should be reversed (can be modified as needed)
// Example configuration - modify these values to change strip directions
bool strip_reverse_config[22] = {
    false, false, false, // Pin 1: strip 1 reversed
    false, false, false, false, // Pin 2: all forward
    false, false, false, false, // Pin 3: all forward
    false, false, false, // Pin 4: strip 12 reversed
    false, false, false, false, // Pin 5: all forward
    false, false, false, false // Pin 6: all forward
};

// New unified strip configuration (CRGBSets will be initialized in initializeStripConfigs())
StripConfig strips[22];

unsigned long current_time;

// WiFi and WebSocket configuration
const char* ssid = "ReflectingThePresent";
const char* password = "lightshow2024";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Sensor ID to LED strip mapping
#define MAX_SENSORS 8
#define MAX_STRIPS_PER_SENSOR 4

struct SensorMapping {
    uint8_t sensor_id;
    uint8_t led_strips[MAX_STRIPS_PER_SENSOR];
    uint8_t num_strips;
    bool active;
    unsigned long last_trigger_time;
};

SensorMapping sensor_mappings[MAX_SENSORS] = {
    // Add your sensor mappings here manually
    { 1, { 0 }, 1, true, 0 }, // Sensor 1
    { 2, { 1 }, 1, true, 0 }, // Sensor 2
    { 3, { 2 }, 1, false, 0 }, // Sensor 3
    { 4, { 3, 4, 5, 6 }, 4, false, 0 }, // Sensor 4
    { 5, { 7, 8, 9, 10 }, 4, true, 0 }, // Sensor 5
    { 6, { 11 }, 1, true, 0 }, // Sensor 6
    { 7, { 12 }, 1, true, 0 }, // Sensor 7
    { 8, { 13 }, 1, true, 0 }, // Sensor 8
    // Add more mappings as needed
};

// Function declarations
void initializeStripConfigs();
void handleSensorMessage(String message);
void setupWiFiAndWebSocket();

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length)
{
    switch (type) {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    } break;

    case WStype_TEXT:
        Serial.printf("[%u] Received: %s\n", num, payload);
        handleSensorMessage((char*)payload);
        break;

    default:
        break;
    }
}

// Handle incoming sensor messages
void handleSensorMessage(String message)
{
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
                        if (j < sensor_mappings[i].num_strips - 1)
                            Serial.print(", ");
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
void setupWiFiAndWebSocket()
{
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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
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
                    if (j < sensor_mappings[i].num_strips - 1)
                        html += ", ";
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

    for (int pin = 0; pin < NUM_PINS; pin++) {
        Serial.printf("Pin %d (%d): %d LEDs (%d strips)\n", pin, pin_configs[pin].pin, pin_configs[pin].total_leds,
            pin_configs[pin].num_strips);
        total_leds += pin_configs[pin].total_leds;
    }

    Serial.printf("Total: %d LEDs across 22 strips\n", total_leds);
    Serial.println("Expected: 3+4+4+3+4+4 = 22 strips total");

    // Initialize and validate new strip configuration system
    initializeStripConfigs();

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

void initializeStripConfigs()
{
    // Validation and setup for the new strip configuration system
    Serial.println("Initializing unified strip configuration...");

    // Initialize strip configuration data using individual field assignment
    // Pin 1 (13) - 3 strips (strips 0-2)
    strips[0].physical_pin = 13;
    strips[0].pin_index = 0;
    strips[0].start_offset = 0;
    strips[0].length = 122;
    strips[0].reverse_direction = false;
    strips[0].led_array_ptr = pin1_leds;
    strips[1].physical_pin = 13;
    strips[1].pin_index = 0;
    strips[1].start_offset = 122;
    strips[1].length = 122;
    strips[1].reverse_direction = false;
    strips[1].led_array_ptr = pin1_leds;
    strips[2].physical_pin = 13;
    strips[2].pin_index = 0;
    strips[2].start_offset = 244;
    strips[2].length = 122;
    strips[2].reverse_direction = false;
    strips[2].led_array_ptr = pin1_leds;

    // Pin 2 (5) - 4 strips (strips 3-6)
    strips[3].physical_pin = 5;
    strips[3].pin_index = 1;
    strips[3].start_offset = 0;
    strips[3].length = 122;
    strips[3].reverse_direction = false;
    strips[3].led_array_ptr = pin2_leds;
    strips[4].physical_pin = 5;
    strips[4].pin_index = 1;
    strips[4].start_offset = 122;
    strips[4].length = 122;
    strips[4].reverse_direction = false;
    strips[4].led_array_ptr = pin2_leds;
    strips[5].physical_pin = 5;
    strips[5].pin_index = 1;
    strips[5].start_offset = 244;
    strips[5].length = 122;
    strips[5].reverse_direction = false;
    strips[5].led_array_ptr = pin2_leds;
    strips[6].physical_pin = 5;
    strips[6].pin_index = 1;
    strips[6].start_offset = 366;
    strips[6].length = 122;
    strips[6].reverse_direction = false;
    strips[6].led_array_ptr = pin2_leds;

    // Pin 3 (19) - 4 strips (strips 7-10)
    strips[7].physical_pin = 19;
    strips[7].pin_index = 2;
    strips[7].start_offset = 0;
    strips[7].length = 122;
    strips[7].reverse_direction = false;
    strips[7].led_array_ptr = pin3_leds;
    strips[8].physical_pin = 19;
    strips[8].pin_index = 2;
    strips[8].start_offset = 122;
    strips[8].length = 122;
    strips[8].reverse_direction = false;
    strips[8].led_array_ptr = pin3_leds;
    strips[9].physical_pin = 19;
    strips[9].pin_index = 2;
    strips[9].start_offset = 244;
    strips[9].length = 122;
    strips[9].reverse_direction = false;
    strips[9].led_array_ptr = pin3_leds;
    strips[10].physical_pin = 19;
    strips[10].pin_index = 2;
    strips[10].start_offset = 366;
    strips[10].length = 122;
    strips[10].reverse_direction = false;
    strips[10].led_array_ptr = pin3_leds;

    // Pin 4 (23) - 3 strips (strips 11-13)
    strips[11].physical_pin = 23;
    strips[11].pin_index = 3;
    strips[11].start_offset = 0;
    strips[11].length = 122;
    strips[11].reverse_direction = false;
    strips[11].led_array_ptr = pin4_leds;
    strips[12].physical_pin = 23;
    strips[12].pin_index = 3;
    strips[12].start_offset = 122;
    strips[12].length = 122;
    strips[12].reverse_direction = false;
    strips[12].led_array_ptr = pin4_leds;
    strips[13].physical_pin = 23;
    strips[13].pin_index = 3;
    strips[13].start_offset = 244;
    strips[13].length = 122;
    strips[13].reverse_direction = false;
    strips[13].led_array_ptr = pin4_leds;

    // Pin 5 (18) - 4 strips (strips 14-17)
    strips[14].physical_pin = 18;
    strips[14].pin_index = 4;
    strips[14].start_offset = 0;
    strips[14].length = 122;
    strips[14].reverse_direction = false;
    strips[14].led_array_ptr = pin5_leds;
    strips[15].physical_pin = 18;
    strips[15].pin_index = 4;
    strips[15].start_offset = 122;
    strips[15].length = 122;
    strips[15].reverse_direction = false;
    strips[15].led_array_ptr = pin5_leds;
    strips[16].physical_pin = 18;
    strips[16].pin_index = 4;
    strips[16].start_offset = 244;
    strips[16].length = 122;
    strips[16].reverse_direction = false;
    strips[16].led_array_ptr = pin5_leds;
    strips[17].physical_pin = 18;
    strips[17].pin_index = 4;
    strips[17].start_offset = 366;
    strips[17].length = 122;
    strips[17].reverse_direction = false;
    strips[17].led_array_ptr = pin5_leds;

    // Pin 6 (12) - 4 strips (strips 18-21)
    strips[18].physical_pin = 12;
    strips[18].pin_index = 5;
    strips[18].start_offset = 0;
    strips[18].length = 122;
    strips[18].reverse_direction = false;
    strips[18].led_array_ptr = pin6_leds;
    strips[19].physical_pin = 12;
    strips[19].pin_index = 5;
    strips[19].start_offset = 122;
    strips[19].length = 122;
    strips[19].reverse_direction = false;
    strips[19].led_array_ptr = pin6_leds;
    strips[20].physical_pin = 12;
    strips[20].pin_index = 5;
    strips[20].start_offset = 244;
    strips[20].length = 122;
    strips[20].reverse_direction = false;
    strips[20].led_array_ptr = pin6_leds;
    strips[21].physical_pin = 12;
    strips[21].pin_index = 5;
    strips[21].start_offset = 366;
    strips[21].length = 122;
    strips[21].reverse_direction = false;
    strips[21].led_array_ptr = pin6_leds;

    // Apply direction configuration
    configureStripDirections();

    // Initialize FastLED sets for each strip
    for (uint8_t i = 0; i < 22; i++) {
        StripConfig& strip = strips[i];

        // Verify pin index is valid
        if (strip.pin_index > 5) {
            Serial.printf("ERROR: Strip %d has invalid pin_index %d\n", i, strip.pin_index);
            continue;
        }

        // Verify the pin matches PinConfig
        if (strip.physical_pin != pin_configs[strip.pin_index].pin) {
            Serial.printf("WARNING: Strip %d pin mismatch - strip:%d vs pinconfig:%d\n", i, strip.physical_pin,
                pin_configs[strip.pin_index].pin);
        }

        // Verify LED array pointer matches
        if (strip.led_array_ptr != pin_configs[strip.pin_index].led_array) {
            Serial.printf("WARNING: Strip %d LED array pointer mismatch\n", i);
        }

        // Note: CRGBSets will be created on-demand in getStripSet() function

        Serial.printf("Strip %d: Pin %d, Offset %d, Length %d, Direction: %s\n", i, strip.physical_pin,
            strip.start_offset, strip.length, strip.reverse_direction ? "REVERSED" : "FORWARD");
    }

    // Add debug output to verify addressing for problematic strips
    Serial.println("=== DEBUG: Verifying problematic strip addressing ===");

    // Test Pin 1 Strip 2 (strip_id = 2)
    Serial.printf("Pin 1 Strip 2 (strip_id=2): ");
    CRGBSet test_set_2 = getStripSet(2);
    Serial.printf("CRGBSet size=%d, ptr=%p\n", test_set_2.size(), &test_set_2[0]);
    Serial.printf("Strip config: pin=%d, offset=%d, length=%d, array_ptr=%p\n", strips[2].physical_pin,
        strips[2].start_offset, strips[2].length, strips[2].led_array_ptr);
    Serial.printf("Calculated start address: %p\n", strips[2].led_array_ptr + strips[2].start_offset);

    // Test Pin 4 Strip 2 (strip_id = 12)
    Serial.printf("Pin 4 Strip 2 (strip_id=12): ");
    CRGBSet test_set_12 = getStripSet(12);
    Serial.printf("CRGBSet size=%d, ptr=%p\n", test_set_12.size(), &test_set_12[0]);
    Serial.printf("Strip config: pin=%d, offset=%d, length=%d, array_ptr=%p\n", strips[12].physical_pin,
        strips[12].start_offset, strips[12].length, strips[12].led_array_ptr);
    Serial.printf("Calculated start address: %p\n", strips[12].led_array_ptr + strips[12].start_offset);

    // Note: Array size verification removed due to extern declaration limitations

    Serial.println("Strip configuration initialized successfully");

    // Perform a simple LED addressing test
    // testStripAddressing();
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
                random_strips[i] = random(0, 13); // Random strip 0-21
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
    // demoFlashBulb();

    // Run the queued pattern program
    runQueuedPattern();
}
