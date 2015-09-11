#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <XBOXUSB.h>
#include <XBee.h>
#include <OzOLED.h>

#include "MVRD_CNTRLR_2.0.h"

USB Usb;
XBOXUSB Xbox(&Usb);
SoftwareSerial SoftSerial(7, 6); // RX, TX
XBee xbee = XBee();

///////////////////////////////////
// SETUP
///////////////////////////////////
char buf[25];
NodeObj node;
int mode = 1;
  
void setup() {
  controller.initOLED();
  controller.initUSB();
  controller.initXBee();

  //delay(1000);
  OzOled.clearDisplay();
}

///////////////////////////////////
// LOOP
///////////////////////////////////

void loop() {
  // Mode 0 - Node Discovery and Selection

  //MODE 1 - Node Scanning and Selection
  while (mode == 1) {

    NodeObj nodeObjs[10];
    
    controller.scanNodes(nodeObjs);

    if (controller.nodeCount > 0) {

      for (int n = 0; n < controller.nodeCount; n++) {
        node = nodeObjs[n];

        OzOled.clearDisplay();

        OzOled.printNumber((long)n + 1, 5, 0);
        OzOled.printString("/", 8, 0);
        OzOled.printNumber((long)controller.nodeCount, 10, 0);
  
        OzOled.printString("Name", 0, 1);
        sprintf (buf, "%s", node.Identity);
        OzOled.printString(buf, 1, 2);

        OzOled.printString("Address:", 0, 3);
        sprintf (buf, "0x%04X", node.Address);
        OzOled.printString(buf, 1, 4);

        OzOled.printString("Strength:", 0, 5);
        sprintf (buf, "-%ddBm", node.Strength);
        OzOled.printString(buf, 1, 6);
        
        delay(2000);
      }

      if (controller.nodeCount == 1 ) {
        // Only one Node was found, so pick it for pairing
        mode = 2;
      } else {
        // More than one Node was found, so allow selection of one for pairing. 

        delay(2000);
      }
      
    } else {
      OzOled.clearDisplay();
      OzOled.printString("Node Count:", 0, 0);
      OzOled.printNumber((long)controller.nodeCount, 1, 1);
      delay(500);
     }
  }

  // MODE 2
  while (mode == 2) {
    OzOled.clearDisplay();
    OzOled.printString("MODE 2", 0, 0);

    OzOled.printString("Name", 0, 1);
    sprintf (buf, "%s", node.Identity);
    OzOled.printString(buf, 1, 2);

    OzOled.printString("Address:", 0, 3);
    sprintf (buf, "0x%04X", node.Address);
    OzOled.printString(buf, 1, 4);
    
    delay(500);
    
  }
}



///////////////////////////////////
// CONTROLLER FUNCTIONS
///////////////////////////////////

void Controller::scanNodes(NodeObj nodeObjs[]) {
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
        controller.timeout = controller.response.getValue()[response.getValueLength() - 1] * 100;
      }
    }
  }

  request.setCommand(ND);
  xbee.send(request);
  
  while (xbee.readPacket(controller.timeout)) {
    // should be receiving AT command responses
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      OzOled.clearDisplay();
      OzOled.printString("Scanning...", 0, 0);
      xbee.getResponse().getAtCommandResponse(controller.response);

      node = controller.Node();
      if (node.Valid) {
        nodeObjs[controller.nodeCount] = node;
        controller.nodeCount++;
      }
    }
  }
}

// Parses the latest response and builds out the node objects 
NodeObj Controller::Node() {
  NodeObj n_node;
  n_node.Valid = false;
  n_node.Length = controller.response.getValueLength();

  if (controller.response.isOk() && n_node.Length > 2) {

    //node.Raw = controller.response.getValue();
    //memcpy(controller.response.getValue(), node.Raw, 7);
    //
    for (int i = 11; i < n_node.Length; i++) {
      n_node.Identity[i-11] = controller.response.getValue()[i];
    }

    n_node.Valid = true;
    n_node.Address = controller.response.getValue()[0] << 8 | controller.response.getValue()[1];
    n_node.SH = controller.response.getValue()[2] << 32 | controller.response.getValue()[3] << 16 | controller.response.getValue()[4] << 8 | controller.response.getValue()[5];
    n_node.SL = controller.response.getValue()[6] << 32 | controller.response.getValue()[7] << 16 | controller.response.getValue()[8] << 8 | controller.response.getValue()[9];
    n_node.Strength = controller.response.getValue()[10];
    return n_node;
  }
  return n_node;
}

// Selects the node to transmit to/from
void Controller::selectNode() {
  
}

// Prints the Menu when not transmitting
void Controller::printMenu() {
  
}

// Prints the current Stats while transmitting
void Controller::printStats() {
  
}

// Wrapper for sending messages to the Node
void Controller::sendPacket() {
  
}

// Wrapper for recieving messages from the Node
void Controller::recievePacket() {
  
}



///////////////////////////////////
// INIT FUNCTIONS
///////////////////////////////////

void Controller::initOLED() {
  OzOled.init();
  OzOled.setNormalDisplay();       //Set display to Normal mode
  OzOled.setHorizontalMode();      //Set addressing mode to Horizontal Mode

  // charge pump ON
  OzOled.sendCommand(0x8d);
  OzOled.sendCommand(0x14);

  OzOled.clearDisplay();
  OzOled.setInverseDisplay();
  OzOled.drawBitmap(MVRDLogo, 0, 0, 16, 8);
  //delay(2000);

  OzOled.clearDisplay();
  OzOled.setNormalDisplay();
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
