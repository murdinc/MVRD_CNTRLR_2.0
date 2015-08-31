#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <XBOXUSB.h>
#include <XBee.h>
#include <OzOLED.h>

#include "MVRD_CNTRLR.h"

USB Usb;
XBOXUSB Xbox(&Usb);
SoftwareSerial SoftSerial(7, 6); // RX, TX
XBee xbee = XBee();

void setup() {
  Serial.begin(9600);
  Serial.println("=== SETUP ===");
  controller.initOLED();
  controller.initUSB();
  controller.initXBee();

  delay(1000);
  OzOled.clearDisplay();
}

void loop() {
  // Mode 0 - Node Discovery and Selection
  Serial.println("=== Loop start ===");
  Serial.println("\n\n");
  //while (controller.mode == 0) {
  while (1 == 1) {
    Serial.println("=== While start ===");
    Serial.println("\n\n");
    controller.scanNodes();

    if (controller.nodeCount > 0) {
  
      NodeObj node;

      for (int i = 0; i < controller.nodeCount; i++) {
        node = controller.nodeObjs[i];
  
        OzOled.clearDisplay();
        OzOled.printString("Node Count:", 0, 0);
        OzOled.printNumber((long)controller.nodeCount, 1, 1);
        
        OzOled.printString("Node Number:", 0, 2);
        OzOled.printNumber((long)i+1, 1, 3);

        
        OzOled.printString("Addr:", 0, 4);
        char buf[6];
        sprintf (buf, "0x%04X", node.Address);
        OzOled.printString(buf, 1, 5);
        
        delay(500);

      }
    } else {
      OzOled.clearDisplay();
      OzOled.printString("Node Count:", 0, 0);
      OzOled.printNumber((long)controller.nodeCount, 1, 1);
    }
    //delay(500);
    
  }
}

void Controller::scanNodes() {
  NodeObj node;
  controller.nodeCount = 0;
  
  // get the Node Discover Timeout (NT) value and set to timeout
  controller.request.setCommand(NT);
  xbee.send(controller.request);
  if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
    xbee.getResponse().getAtCommandResponse(controller.response);
    if (controller.response.isOk()) {
      if (response.getValueLength() > 0) {
        // NT response range should be from 0x20 - 0xFF, but
        // I see an inital byte set to 0x00, so grab the last byte
        timeout = controller.response.getValue()[response.getValueLength() - 1] * 100;
      }
    }
  }
  
  request.setCommand(ND);
  xbee.send(request);
  
  while(xbee.readPacket(timeout)) {
    // should be receiving AT command responses
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(controller.response);

      node = controller.Node();
      if (node.Valid) {
          controller.nodeObjs[controller.nodeCount] = node;
          controller.nodeCount++;
      }
      
    }
  }
}

void Controller::initOLED() {
  OzOled.init();
  OzOled.setNormalDisplay();       //Set display to Normal mode
  OzOled.setHorizontalMode();      //Set addressing mode to Horizontal Mode

  // charge pump ON
  OzOled.sendCommand(0x8d);
  OzOled.sendCommand(0x14); 

  OzOled.clearDisplay();
  OzOled.drawBitmap(MVRDLogo, 0, 0, 16, 8);
  delay(1000);
  OzOled.setInverseDisplay();
  delay(1000);
  OzOled.setNormalDisplay();

  OzOled.clearDisplay();

  OzOled.printString("OLED        [OK]", 0, 0);
}


void Controller::initUSB() {
  // Link the xbox hardware
  if (Usb.Init() == -1) {
    OzOled.printString("CNTRLR      [!!]", 0, 1);
    while (1); //halt
  }
  OzOled.printString("CNTRLR      [OK]", 0, 1);

}


void Controller::initXBee() {
  SoftSerial.begin(9600);
  xbee.setSerial(SoftSerial);
  OzOled.printString("RF          [OK]", 0, 2);  

}


NodeObj Controller::Node() {
  NodeObj node;

  Serial.print("Length: ");
  Serial.print(controller.response.getValueLength() );
  Serial.println("");
  
  if (controller.response.isOk() && controller.response.getValueLength() > 2) {

          for (int i = 0; i < controller.response.getValueLength(); i++) {
            Serial.print("RAW: ");
            Serial.print(controller.response.getValue()[i], HEX);
            Serial.print(" ");
          }
          Serial.println(" ");

    
    node.Valid = true;
    node.Address = controller.response.getValue()[0] << 8 | controller.response.getValue()[1];
    //node.SH = controller.response.getValue()[2] <<32 | controller.response.getValue()[3] << 16 | controller.response.getValue()[4] << 8 | controller.response.getValue()[5];
    //node.SL = controller.response.getValue()[6] <<32 | controller.response.getValue()[7] << 16 | controller.response.getValue()[8] << 8 | controller.response.getValue()[9];
    //node.Strength = controller.response.getValue()[10];
    // identifier = response.getValue()[0] << 8 | response.getValue()[1]; // 11 to end
  }
  return node;

}
