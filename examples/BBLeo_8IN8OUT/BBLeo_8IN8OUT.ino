// *************************************************
// CMRI/Net example Sketch: BBLEO
// Onboard I/O:   8 bits of INPUT
//                8 bits of OUTPUT
// IOX I/O:      None
// *************************************************
//
//  Reserved I/O pins are:
//   D0 - RX  CMRI RS485 Receive (Serial)
//   D1 - TX  CMRI RS485 Transmit
//   USB Serial1 (debugging)

#include "cpNode.h"

cpNode cmri;    // Processing logic for handling CMRINet packets

const int  nodeID = 0;                            // 0...63 (nodeID + ord('A') => 'A'..chr(127))
const long CMRINET_SPEED = 19200;                 // 9600, 19200 ...

// the following need to match the code in setup(), pack() and unpack()...
const int  InputBytes  = 2 + 0;                    // 2x onboard  (no IOX...)
const int  OutputBytes = 2 + 0;                    // 2x onboard

//-----------------------
//  Get the available ram
//-----------------------
int freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setup(void) {
    // *************************************************
    // *******          Setup CMRI            **********
    // *************************************************

    Serial1.begin(CMRINET_SPEED);          // Set up and Open the CMRInet port
    while(!Serial1) { };

    cmri.setCMRIPort(&Serial1);           // for CMRI/Net protocol
    cmri.setDebugPort(&Serial);           // for debugging

    cmri.setNodeAddress(nodeID);          // Set the node address
    cmri.setNumInputBytes(InputBytes);    // reflected in pack()
    cmri.setNumOutputBytes(OutputBytes);  // reflected in unpack()

    cmri.invertInputs(  true );           // invert bits?
    cmri.invertOutputs( true );

    // *************************************************
    // *******   Setup  Onboard I/O           **********
    // *************************************************
    pinMode( 4, INPUT_PULLUP);  // D4
    pinMode( 5, INPUT_PULLUP);  // D5
    pinMode( 6, INPUT_PULLUP);  // D6
    pinMode( 7, INPUT_PULLUP);  // D7
    pinMode( 8, INPUT_PULLUP);  // D8
    pinMode( 9, INPUT_PULLUP);  // D9
    pinMode(10, INPUT_PULLUP);  // D10
    pinMode(11, INPUT_PULLUP);  // D11

    pinMode(12, OUTPUT);        // D12
    pinMode(13, OUTPUT);        // D13
    pinMode(A0, OUTPUT);        // A0
    pinMode(A1, OUTPUT);        // A1
    pinMode(A2, OUTPUT);        // A2
    pinMode(A3, OUTPUT);        // A3
    pinMode(A4, OUTPUT);        // A4
    pinMode(A5, OUTPUT);        // A5

    Serial.println(F("\nCMRI Node configuration: BBLEO (8IN 8OUT)\n"));
    Serial.print(F("    Baud Rate:        ")); Serial.println(CMRINET_SPEED, DEC);
    Serial.print(F("    Node ua:          ")); Serial.println(cmri.getNodeAddress);
    Serial.print(F("    Memory Available: ")); Serial.println(freeRam());
}

// ---------------------------------------------------------------------------
// pack() is called whenever there is a need to read bits from the layout
//        in response to a poll request
//
//  The pack input routine collects the bits from the onboard IO ports and
//  external I/O expanders and puts the them into the correct IB array bytes
//  for transmission back to the control host.
//
//  len bytes need to be read:
//  Onboard I/O goes in the first two bytes, IB[0] and IB[1]
//  The rest of the bytes are used by the optional IO expanders
// ---------------------------------------------------------------------------

void pack(byte *IB, int len) {
    if (len >= 1) {
        IB[0] = 0;
        IB[0] |= (!digitalRead(4)  << 0);
        IB[0] |= (!digitalRead(5)  << 1);
        IB[0] |= (!digitalRead(6)  << 2);
        IB[0] |= (!digitalRead(7)  << 3);
        IB[0] |= (!digitalRead(8)  << 4);
        IB[0] |= (!digitalRead(9)  << 5);
        IB[0] |= (!digitalRead(10) << 6);
        IB[0] |= (!digitalRead(11) << 7);
    }
    if (len >= 2) {
        IB[1] = 0;
    }
}

// ---------------------------------------------------------------------------
// unpack() is called whenever there is a need to write bits out to the layout
//
//  The unpack output routine needs to take the received bits from the OB
//  ouput buffer and write them to the correct output ports using either
//  digitalWrite() or the IO expanders
//
//  len bytes are available to be written
//  Onboard I/O comes from the first two bytes, followed by IO expander bytes
//----------------------------------------------------------------------------

void unpack(byte *OB, int len) {
    if (len >= 1) {
        digitalWrite(12, (( OB[0] >> 0) &  0x01) );
        digitalWrite(13, (( OB[0] >> 1) &  0x01) );
        digitalWrite(A0, (( OB[0] >> 2) &  0x01) );
        digitalWrite(A1, (( OB[0] >> 3) &  0x01) );
        digitalWrite(A2, (( OB[0] >> 4) &  0x01) );
        digitalWrite(A3, (( OB[0] >> 5) &  0x01) );
        digitalWrite(A4, (( OB[0] >> 6) &  0x01) );
        digitalWrite(A5, (( OB[0] >> 7) &  0x01) );
    }
}

void loop(void) {
    cmri.proceess();   // process any C/MRI packets
}