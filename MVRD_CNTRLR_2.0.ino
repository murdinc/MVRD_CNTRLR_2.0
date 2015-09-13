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
PacketObj packet;

TxStatusResponse txStatus = TxStatusResponse();

char ca, cb, cx, cy;
  
void setup() {
  controller.initOLED();
  controller.initUSB();
  controller.initXBee();

  delay(1000);
  OzOled.clearDisplay();
}

///////////////////////////////////
// LOOP
///////////////////////////////////

void loop() {

  ///////////////////////////////////
  // MODE 1 - Node Scanning and Selection
  ///////////////////////////////////
  while (mode == 1) {
    int n = 0;
    int selected_n = 1;
        
    NodeObj nodeObjs[10]; // Max 10 nodes, there may be a better way to do this? 
    controller.scanNodes(nodeObjs);

    if (controller.nodeCount > 0) {
      if (controller.nodeCount == 1 ) {
        // Only one Node was found, so pick it for pairing
        node = nodeObjs[0];
        mode = 2;
        OzOled.clearDisplay();
        break;
      } else {
        // More than one Node was found, so allow selection of one for pairing. 
        
        while (mode == 1) {
          if (selected_n != n) {
            selected_n = n;
            node = nodeObjs[selected_n];
    
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
          }

          // Get our button presses
          Usb.Task();

          // Rescan Nodes (BACK)
          if (Xbox.getButtonClick(BACK)) {
            mode = 1;
            break;
          }
          
          // Page Left (LEFT)
          if (Xbox.getButtonClick(LEFT)) {
            n--;
            if (n < 0) {
              n = 0;
            }
          }

          // Page Right (RIGHT)
          if (Xbox.getButtonClick(RIGHT)) {
            n++;
            if (n > controller.nodeCount - 1) {
              n = controller.nodeCount - 1;
            }
          }
          
          // Select Node (START)
          if (Xbox.getButtonPress(START) && !Xbox.getButtonPress(BACK)) {
            mode = 2;
            OzOled.clearDisplay();
            break;
          }
        }
      }
      
    } else {
      OzOled.clearDisplay();
      OzOled.printString("Node Count:", 0, 0);
      OzOled.printNumber((long)controller.nodeCount, 1, 1);
     }
  }

  ///////////////////////////////////
  // MODE 2 - Normal Display
  ///////////////////////////////////
  while (mode == 2) {
    
    // Mode 2 display

    // Get our button presses
    Usb.Task();

    // Go to Debug Display
    if (Xbox.getButtonClick(BACK)) {
      mode = 3;
      OzOled.clearDisplay();
      break;
    }
  
    // Rescan Nodes (BACK + START)
    if (Xbox.getButtonPress(BACK) && Xbox.getButtonPress(START)) {
      mode = 1;
      break;
    }
    
    controller.buildPacket();
    controller.sendPayload();

    // Status of last transmission
    if (controller.success) {
      OzOled.printString("*", 0, 0);
    } else {
      OzOled.printString("-", 0, 0);
    }

    // Identity
    sprintf (buf, "%s", node.Identity);
    OzOled.printString(buf, 2, 0);
    
  }

  ///////////////////////////////////
  // MODE 3 - Debug Mode
  ///////////////////////////////////
  while (mode == 3) {
    OzOled.printString("Debug Mode", 3, 0);

    // Status of last transmission
    if (controller.success) {
      OzOled.printString("*", 0, 0);
    } else {
      OzOled.printString("-", 0, 0);
    }

    // Get our button presses
    Usb.Task();

    // Back to Main Display
    if (Xbox.getButtonClick(BACK)) {
      mode = 2;
      OzOled.clearDisplay();
      break;
    }
  
    // Rescan Nodes (BACK + START)
    if (Xbox.getButtonPress(BACK) && Xbox.getButtonPress(START)) {
      mode = 1;
      break;
    }

    controller.buildPacket();
    controller.sendPayload();

    // Package Test
    sprintf (buf, "LX:%6d", controller.packet.leftX);
    OzOled.printString(buf, 0, 1);
    sprintf (buf, "LY:%6d", controller.packet.leftY);
    OzOled.printString(buf, 0, 2);
    sprintf (buf, "RX:%6d", controller.packet.rightX);
    OzOled.printString(buf, 0, 3);
    sprintf (buf, "RY:%6d", controller.packet.rightY);
    OzOled.printString(buf, 0, 4);

    sprintf (buf, "L1:%d L2:%3d L3:%d", controller.packet.l1, controller.packet.l2, controller.packet.l3);
    OzOled.printString(buf, 0, 5);
    sprintf (buf, "R1:%d R2:%3d R3:%d", controller.packet.r1, controller.packet.r2, controller.packet.r3);
    OzOled.printString(buf, 0, 6);

    if (controller.packet.a) { ca = 'A'; } else { ca = '-'; }
    if (controller.packet.b) { cb = 'B'; } else { cb = '-'; }
    if (controller.packet.x) { cx = 'X'; } else { cx = '-'; }
    if (controller.packet.y) { cy = 'Y'; } else { cy = '-'; }

    sprintf (buf, "%c%c%c%c", ca, cb, cx, cy);
    OzOled.printString(buf, 10, 1);

    if (controller.packet.d_up) {
      OzOled.printString("UP   ", 10, 3);
    } else if (controller.packet.d_down) {
      OzOled.printString("DOWN ", 10, 3);
    } else {
      OzOled.printString("-----", 10, 3);
    }

    if (controller.packet.d_left) {
      OzOled.printString("LEFT ", 10, 4);
    } else if (controller.packet.d_right) {
      OzOled.printString("RIGHT", 10, 4);
    } else {
      OzOled.printString("-----", 10, 4);
    }

  }
  
}



///////////////////////////////////
// CONTROLLER FUNCTIONS
///////////////////////////////////

void Controller::scanNodes(NodeObj nodeObjs[]) {
  controller.nodeCount = 0;

  // get the Node Discover Timeout (NT) value and set to timeout
  controller.at_request.setCommand(NT);
  xbee.send(controller.at_request);
  if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
    xbee.getResponse().getAtCommandResponse(controller.at_response);
    if (controller.at_response.isOk()) {
      if (at_response.getValueLength() > 0) {
        // NT response range should be from 0x20 - 0xFF, but
        // I see an inital byte set to 0x00, so grab the last byte
        controller.timeout = controller.at_response.getValue()[at_response.getValueLength() - 1] * 100;
      }
    }
  }

  at_request.setCommand(ND);
  xbee.send(at_request);
  
  while (xbee.readPacket(controller.timeout)) {
    // should be receiving AT command responses
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      OzOled.clearDisplay();
      OzOled.printString("Scanning...", 0, 0);
      xbee.getResponse().getAtCommandResponse(controller.at_response);

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
  n_node.Length = controller.at_response.getValueLength();

  if (controller.at_response.isOk() && n_node.Length > 2) {

    for (int i = 11; i < n_node.Length; i++) {
      n_node.Identity[i-11] = controller.at_response.getValue()[i];
    }

    n_node.Valid = true;
    n_node.Address = controller.at_response.getValue()[0] << 8 | controller.at_response.getValue()[1];
    n_node.SH = controller.at_response.getValue()[2] << 32 | controller.at_response.getValue()[3] << 16 | controller.at_response.getValue()[4] << 8 | controller.at_response.getValue()[5];
    n_node.SL = controller.at_response.getValue()[6] << 32 | controller.at_response.getValue()[7] << 16 | controller.at_response.getValue()[8] << 8 | controller.at_response.getValue()[9];
    n_node.Strength = controller.at_response.getValue()[10];
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

void Controller::buildPacket() {
  // Get our button presses
  Usb.Task();

  if (Xbox.Xbox360Connected) {

    // Joysticks
    controller.packet.leftX = Xbox.getAnalogHat(LeftHatX);
    //controller.packet.leftX = map(Xbox.getAnalogHat(LeftHatX), -32770, 32770, -255, 255);
    controller.packet.leftY = map(Xbox.getAnalogHat(LeftHatY), -32768, 32767, -255, 255);
    controller.packet.rightX = map(Xbox.getAnalogHat(RightHatX), -32768, 32767, -255, 255);
    controller.packet.rightY = map(Xbox.getAnalogHat(RightHatY), -32768, 32767, -255, 255);

    if (Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500) {
      controller.packet.leftX = Xbox.getAnalogHat(LeftHatX);
    } else {
      controller.packet.leftX = 0;
    }
    if (Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500) {
      controller.packet.leftY = Xbox.getAnalogHat(LeftHatY);
    } else {
      controller.packet.leftY = 0;
    }
    if (Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500) {
      controller.packet.rightX = Xbox.getAnalogHat(RightHatX);
    } else {
      controller.packet.rightX = 0;
    }
    if (Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500) {
      controller.packet.rightY = Xbox.getAnalogHat(RightHatY);
    } else {
      controller.packet.rightY = 0;
    }

    // Triggers
    //////////
    controller.packet.l2 = Xbox.getButtonPress(L2);
    controller.packet.r2 = Xbox.getButtonPress(R2);

    // Buttons - Toggle
    //////////
    if (Xbox.getButtonClick(A))
      controller.packet.a = !controller.packet.a;
    if (Xbox.getButtonClick(B))
      controller.packet.b = !controller.packet.b;
    if (Xbox.getButtonClick(X))
      controller.packet.x = !controller.packet.x;
    if (Xbox.getButtonClick(Y))
      controller.packet.y = !controller.packet.y;

    // Buttons - Momentary
    //////////
    controller.packet.l1 = Xbox.getButtonPress(L1);
    controller.packet.r1 = Xbox.getButtonPress(R1);
    controller.packet.l3 = Xbox.getButtonClick(L3);
    controller.packet.r3 = Xbox.getButtonClick(R3);

    // D Pad - Momentary
    //////////
    controller.packet.d_up = Xbox.getButtonPress(UP);    
    controller.packet.d_down = Xbox.getButtonPress(DOWN);
    controller.packet.d_left = Xbox.getButtonPress(LEFT);
    controller.packet.d_right = Xbox.getButtonPress(RIGHT);

    // Back
    //////////
    if (Xbox.getButtonPress(BACK)) {
      
    }
    
    // Start
    //////////
    if (Xbox.getButtonPress(START)) {
      
    }
    
  }
}

// Wrapper for sending messages to the Node
void Controller::sendPayload() {

  // Joysticks - leftX (2), leftY (2), rightX (2), rightY (2)
  payload[0] = controller.packet.leftX >> 8;
  payload[1] = controller.packet.leftX;
  payload[2] = controller.packet.leftY >> 8;
  payload[3] = controller.packet.leftY;
  payload[4] = controller.packet.rightX >> 8;
  payload[5] = controller.packet.rightX;
  payload[6] = controller.packet.rightY >> 8;
  payload[7] = controller.packet.rightY;

  // Triggers - L2 R2
  payload[8] = controller.packet.l2;
  payload[9] = controller.packet.r2;

  // Buttons - toggle
  payload[10] = (controller.packet.a << 7) | (controller.packet.b << 6) | (controller.packet.x << 5) | (controller.packet.y << 4)
  // Buttons - momentary
  | (controller.packet.l1 << 3) | (controller.packet.r1 << 2) | (controller.packet.l3 << 1) | (controller.packet.r3);

  // D Pad - momentary
  payload[11] = (controller.packet.d_up << 7) | (controller.packet.d_down << 6) | (controller.packet.d_left << 5) | (controller.packet.d_right << 4);

  // 4 bits left! Lets figure out something cool to do with them. 

  controller.tx.setAddress16(node.Address);
  xbee.send(tx);

  controller.recievePacket();
}


// Wrapper for recieving messages from the Node
void Controller::recievePacket() {
  // wait up to 5 seconds for the status response
  if (xbee.readPacket(5000)) {
      // got a response!
             
    if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) {
       xbee.getResponse().getTxStatusResponse(txStatus);
      
         if (txStatus.getStatus() == SUCCESS) {
              controller.success = true;
         } else {
            // the remote XBee did not receive our packet. is it powered on?
              controller.success = false;
         }
      }      
  } else if (xbee.getResponse().isError()) {
    controller.success = false;
  } else {
    // local XBee did not provide a timely TX Status Response.  Radio is not configured properly or connected
    controller.success = false;
  }
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
  delay(2000);

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
