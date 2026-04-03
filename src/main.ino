#include <Adafruit_NeoPixel.h>
#include "pixelController.h"
#include "sensorController.h"
#include "main.h"

uint8_t commandArgs[16];
static bool disable_spells = false;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;


Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRBW + NEO_KHZ800);
PixelController controller(pixels);
SensorController sensor(controller);


class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    printSerials("Client connected");
    controller.setState(BLUETOOTH_CONNECT);
  }

  void onDisconnect(BLEServer* pServer) {
    printSerials("Client disconnected");
    controller.setState(BLUETOOTH_DISCONNECT);
    // Restart advertising
    pServer->getAdvertising()->start();
  }
};


// Callback triggered when client writes data
class RXCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    
    if (rxValue.length() > 0) {
      String input = String(rxValue.c_str());
      parseCommand(input);
    }
  }
};


void setup() {
  I2C_WIRE.begin();
  I2C_WIRE.setClock(400000); // 400kHz I2C
  Serial.begin(BAUD);
  sensor.begin();
  controller.begin();

  BLEDevice::init("StaffController");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //Transmit Characteristic
  pTxCharacteristic = pService->createCharacteristic(
    TX_UUID, BLECharacteristic::PROPERTY_NOTIFY );
  pTxCharacteristic->addDescriptor(new BLE2902());

  //Receive Characteristic
  pRxCharacteristic = pService->createCharacteristic(
    RX_UUID, BLECharacteristic::PROPERTY_WRITE );
  pRxCharacteristic->setCallbacks(new RXCallback());

  pService->start();
  pServer->setCallbacks(new ServerCallbacks());
  pServer->getAdvertising()->start();
  printSerials("BLE Service Started, waiting for clients...");
  
}

void parseCommand(String input) {
  input.trim();
  
  int equalsIndex = input.indexOf('=');
  if (equalsIndex == -1) {
    equalsIndex = input.length();
  }
  String command = input.substring(0, equalsIndex);
  String value = input.substring(equalsIndex + 1);
  
  command.trim();
  value.trim();
  command.toUpperCase();
  value.toUpperCase();
  
  if (command == "STATE") {
    if(equalsIndex == input.length()) {
      printSerials("Error: STATE command requires a value");
      return;
    }
    if (value == "IDLE") {
      controller.setState(IDLE);
      printSerials("State: IDLE");
    }
    else if (value == "TEST_ONE") {
      controller.setState(TEST_ONE);
      printSerials("State: TEST_ONE");
    }
    else if (value == "TEST_TWO") {
      controller.setState(TEST_TWO);
      printSerials("State: TEST_TWO");
    }
    else if (value == "FIREBALL") {
      controller.setState(FIREBALL);
      printSerials("State: Fireball");
    }
    else if (value == "NECROBALL") {
      controller.setState(NECROBALL);
      printSerials("State: Necroball");
    }

    else if (value.startsWith("SOLID_WRGB")) {

   
      int paramIndex = 0;
      int lastComma = value.indexOf(',');
      int start = value.indexOf(' ') + 1;
      
      while (lastComma != -1 && paramIndex < 4) {
        String paramStr = value.substring(start, lastComma);
        commandArgs[paramIndex++] = paramStr.toInt();
        start = lastComma + 1;
        lastComma = value.indexOf(',', start);
      }
      if (paramIndex < 4 && start < value.length()) {
        String paramStr = value.substring(start);
        commandArgs[paramIndex++] = paramStr.toInt();
      }
      
      if (paramIndex == 4) {
        controller.setState(COMMAND_SOLID_WRGB);
        printSerials("State: COMM_SOLID_WRGB with W=" + String(commandArgs[0]) + 
                       ", R=" + String(commandArgs[1]) + 
                       ", G=" + String(commandArgs[2]) + 
                       ", B=" + String(commandArgs[3]));
      } else {
        printSerials("Error: SOLID_WRGB requires 4 parameters: W,R,G,B");
      }
    }
    else {
      printSerials("Error: Unknown state");
    }
  }
  else if (command = "currentState") {
    String stateStr;
    switch (controller.getCurrentState()) {
      case IDLE: stateStr = "IDLE"; break;
      case TEST_ONE: stateStr = "TEST_ONE"; break;
      case TEST_TWO: stateStr = "TEST_TWO"; break;
      case FIREBALL: stateStr = "FIREBALL"; break;
      case NECROBALL: stateStr = "NECROBALL"; break;
      case SOLID_WRGB: stateStr = "SOLID_WRGB"; break;
      case COMMAND_SOLID_WRGB: stateStr = "COMMAND_SOLID_WRGB"; break;
      case BEACON: stateStr = "BEACON"; break;
      case BLUETOOTH_CONNECT: stateStr = "BLUETOOTH_CONNECT"; break;
      case BLUETOOTH_DISCONNECT: stateStr = "BLUETOOTH_DISCONNECT"; break;
      default: stateStr = "UNKNOWN"; break;
    }
    printSerials("Current State: " + stateStr);
  } else if (command = "toggleSpell"){
    disable_spells = !disable_spells;
    printSerials("Spells Disabled:" + String(disable_spells));
  }

}

void loop() {
  sensor.readSensor();

  //TODO single gesture
  if(sensor.gestureDetected()) {
  }

  //Sequences
  if(!disable_spells && sensor.sequenceDetected()) {
    SequenceType sequence = sensor.getLastSequence();
    switch(sequence){
      case SEQ_FIREBALL:
        if(controller.getCurrentState() == FIREBALL){
          controller.setState(IDLE);
        } else {
          controller.setState(FIREBALL);
        }
        break;
      case SEQ_NECROBALL:
        if(controller.getCurrentState() == NECROBALL){
          controller.setState(IDLE);
        } else {
          controller.setState(NECROBALL);
        }
        break;
    }
    sensor.clearSequence();
  }

  controller.update();
  

  // Check USB Serial
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    parseCommand(input);
  }
  

}

void printSerials(String msg) {
  Serial.println(msg);
  if (pTxCharacteristic) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
}