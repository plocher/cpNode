// *************************************************
// CMRI/Net example Sketch: BBLEO (RSMC_LOCK)
// Base Node with remote stall motor and lock
// Onboard I/O:  12 bits of OUTPUT
//                4 bits of INPUT
// IOX I/O:      None
// *************************************************
//
//  Reserved I/O pins are:
//   D0 - RX  CMRI RS485 Receive
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
    pinMode( 4, OUTPUT);        // D4  SG1A-R
    pinMode( 5, OUTPUT);        // D5  SG1A-Y
    pinMode( 6, OUTPUT);        // D6  SG1A-G
    pinMode( 7, OUTPUT);        // D7  SG1B-R
    pinMode( 8, OUTPUT);        // D8  SG1B-Y
    pinMode( 9, OUTPUT);        // D9  SG2-R
    pinMode(10, OUTPUT);        // D10 SG2-Y
    pinMode(11, OUTPUT);        // D11 SG2-G
    pinMode(12, OUTPUT);        // D12 SG3-R
    pinMode(13, OUTPUT);        // D13 SG3-Y
    pinMode(A0, OUTPUT);        // A0  SW Turnout throw input to RSMC
    pinMode(A1, OUTPUT);        // A1  SW Lock Control to Switch Lock controller

    pinMode(A2, INPUT_PULLUP);  // A2  Fascia switch control
    pinMode(A3, INPUT_PULLUP);  // A3  TC1  Track Circuit 1
    pinMode(A4, INPUT_PULLUP);  // A4  TC2  Track Circuit 2
    pinMode(A5, INPUT_PULLUP);  // A5  OS1  OS Circuit 1


    Serial.println(F("\nCMRI Node configuration: BBLEO_RSMC_LOCK\n"));
    Serial.print(F("    Baud Rate:        ")); Serial.println(CMRINET_SPEED, DEC);
    Serial.print(F("    Node ua:          ")); Serial.println(cmri.getNodeAddress());
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
//  len bytes (as set by setNumInputBytes above) need to be read:
//  Onboard I/O goes in the first two bytes, IB[0] and IB[1]
//  The rest of the bytes are used by the optional IO expanders
// ---------------------------------------------------------------------------

void pack(byte *IB, int len) {
    IB[0] = 0;
    IB[0] |= (!digitalRead(A2) << 0);
    IB[0] |= (!digitalRead(A3) << 1);
    IB[0] |= (!digitalRead(A4) << 2);
    IB[0] |= (!digitalRead(A5) << 3);

    IB[1] = 0; // NO OP
}

// ---------------------------------------------------------------------------
// unpack() is called whenever there is a need to write bits out to the layout
//
//  The unpack output routine needs to take the received bits from the OB
//  ouput buffer and write them to the correct output ports using either
//  digitalWrite() or the IO expanders
//
//  len bytes (as set by setNumOutputBytes above) are available to be written
//  Onboard I/O comes from the first two bytes, followed by IO expander bytes
//----------------------------------------------------------------------------

void unpack(byte *OB, int len) {
    digitalWrite(4,  (( OB[0] >> 0) &  0x01) );
    digitalWrite(5,  (( OB[0] >> 1) &  0x01) );
    digitalWrite(6,  (( OB[0] >> 2) &  0x01) );
    digitalWrite(7,  (( OB[0] >> 3) &  0x01) );
    digitalWrite(8,  (( OB[0] >> 4) &  0x01) );
    digitalWrite(9,  (( OB[0] >> 5) &  0x01) );
    digitalWrite(10, (( OB[0] >> 6) &  0x01) );
    digitalWrite(11, (( OB[0] >> 7) &  0x01) );

    digitalWrite(12, (( OB[1] >> 0) &  0x01) );
    digitalWrite(13, (( OB[1] >> 1) &  0x01) );
    digitalWrite(A0, (( OB[1] >> 2) &  0x01) );
    digitalWrite(A1, (( OB[1] >> 3) &  0x01) );
}

void loop(void) {
    cmri.proceess();   // process any C/MRI packets
}