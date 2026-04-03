#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <algorithm>

#define BAUD 115200
#define LED_PIN 5
#define NUMPIXELS 16
#define I2C_WIRE Wire
#define SERVICE_UUID "670893ef-69df-46a1-a949-ef4dcf4ab86a"
#define TX_UUID "7cf3cee9-602d-44fb-aa8b-6fe1008ed54c"
#define RX_UUID "c9d06073-3eed-4792-94f7-ee9ef03e4ef7"


extern uint8_t commandArgs[16]; 

void printSerials(String msg);
void parseCommand(String input);