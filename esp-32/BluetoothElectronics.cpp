#define DEBUG 0
#define DEBUG_BAUD_RATE 57600

#include "BluetoothElectronics.h"
//#include "BluetoothSerial.h"
#include <HardwareBLESerial.h>
HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();
BluetoothElectronics::BluetoothElectronics(){}


void BluetoothElectronics::registerCommand(const String& receiveChar, void (*action)(const String&)) {
  Command* newCommand = new Command{ receiveChar, action, nullptr };
  if (!commandHead) {
    commandHead = newCommand;
  } else {
    Command* temp = commandHead;
    while (temp->next) {
      temp = temp->next;
    }
    temp->next = newCommand;
  }
}

void BluetoothElectronics::begin() {
#if DEBUG
  Serial.begin(DEBUG_BAUD_RATE);
#endif
  bleSerial.beginAndSetupBLE("Boooh");
}

void BluetoothElectronics::handleInput() {
  while (bleSerial.availableLines() > 0) {
    char line[128]; bleSerial.readLine(line, 128);
// may have to do this    if (c == '\n') {
//      inputBuffer.trim();
#if DEBUG
      Serial.println("Received: " + line);
#endif
      processInput(line);
#if DEBUG
      bleSerial.println("Echo: " + line);
#endif
  }
}

void BluetoothElectronics::processInput(String input) {
#if DEBUG
  Serial.println("Processing trimmed input: " + input);
#endif
  Command* current = commandHead;
  while (current) {
    if (input.startsWith(current->receiveChar)) {
#if DEBUG
      Serial.println("Matched receiveChar: " + current->receiveChar);
#endif
      String parameter = "";
      int receiveCharLength = current->receiveChar.length();
      if (input.length() > receiveCharLength) {
        parameter = input.substring(receiveCharLength);
      }
#if DEBUG
      Serial.println("Parameter: " + parameter);
#endif
      current->action(parameter);
      break;
    }
    current = current->next;
  }
#if DEBUG
  Serial.println("Finished processing input.");
#endif
}

void BluetoothElectronics::sendKwlString(String value, String receiveChar) {
  String cmd = "*" + receiveChar + value + "*";
#if DEBUG
  Serial.println("Sending via BT: " + cmd);
#endif
  bleSerial.println(cmd.c_str());
}

void BluetoothElectronics::sendKwlValue(int value, String receiveChar) {
  sendKwlString(String(value), receiveChar);
}

void BluetoothElectronics::sendKwlCode(String code) {
  String cmd = String(KWL_BEGIN) + "\n" + code + "\n" + String(KWL_END);
#if DEBUG
  Serial.println("Sending via BT: " + cmd);
#endif
  bleSerial.println(cmd.c_str());
}


  bool BluetoothElectronics::beginAndSetupBLE(const char *name){
    bleSerial.beginAndSetupBLE(name);
  }
  void BluetoothElectronics::poll(){
    bleSerial.poll();
  }
