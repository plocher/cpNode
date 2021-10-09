// *************************************************
// CMRI/Net example Sketch: ProMini
// Onboard I/O:   0 bits of OUTPUT
//               16 bits of INPUT
// IOX I/O:      NO IOX-16s
// *************************************************
//
//  Reserved I/O pins are:
//   D0 - RX  CMRI RS485 Receive
//   D1 - TX  CMRI RS485 Transmit


#include "cpNode.h"

cpNode cmri;    // Processing logic for handling CMRINet packets

const int  nodeID = 0;                            // 0...63 (nodeID + ord('A') => 'A'..chr(127))
const long CMRINET_SPEED = 19200;                 // 9600, 19200 ...

// the following need to match the code in setup(), pack() and unpack()...
const int  InputBytes  = 2 + 0;                    // 2x onboard
const int  OutputBytes = 2 + 0;                    // 2x onboard

void setup(void) {
    // *************************************************
    // *******          Setup CMRI            **********
    // *************************************************

    Serial.begin(CMRINET_SPEED);          // Set up and Open the CMRInet port
    while(!Serial) { };

    cmri.setCMRIPort(&Serial);
    cmri.setNodeAddress(nodeID);          // Set the node address
    cmri.setNumInputBytes(InputBytes);    // reflected in pack(),   2x onboard plus IOX expander
    cmri.setNumOutputBytes(OutputBytes);  // reflected in unpack(), 2x onboard plus IOX expander

    cmri.invertInputs(  true );           // invert all bits?
    cmri.invertOutputs( true );

    // *************************************************
    // *******   Setup  Onboard I/O           **********
    // *************************************************
    pinMode( 2, OUTPUT);        // D2
    pinMode( 3, OUTPUT);        // D3
    pinMode( 4, OUTPUT);        // D4
    pinMode( 5, OUTPUT);        // D5
    pinMode( 6, OUTPUT);        // D6
    pinMode( 7, OUTPUT);        // D7
    pinMode( 8, OUTPUT);        // D8
    pinMode( 9, OUTPUT);        // D9

    pinMode(10, OUTPUT);        // D10
    pinMode(11, OUTPUT);        // D11
    pinMode(12, OUTPUT);        // D12
    pinMode(13, OUTPUT);        // D13
    pinMode(A0, OUTPUT);        // A0
    pinMode(A1, OUTPUT);        // A1
    pinMode(A2, OUTPUT);        // A2
    pinMode(A3, OUTPUT);        // A3

    digitalWrite( 2, 0 );       // if desired, set initial output state
    digitalWrite( 3, 0 );
    digitalWrite( 4, 0 );
    digitalWrite( 5, 0 );
    digitalWrite( 6, 0 );
    digitalWrite( 7, 0 );
    digitalWrite( 8, 0 );
    digitalWrite( 9, 0 );

    digitalWrite(10, 0 );
    digitalWrite(11, 0 );
    digitalWrite(12, 0 );
    digitalWrite(13, 0 );
    digitalWrite(A1, 0 );
    digitalWrite(A2, 0 );
    digitalWrite(A3, 0 );
    digitalWrite(A4, 0 );
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
    // lower8
        digitalWrite( 2, (( OB[0] >> 0) &  0x01) );
        digitalWrite( 3, (( OB[0] >> 1) &  0x01) );
        digitalWrite( 4, (( OB[0] >> 2) &  0x01) );
        digitalWrite( 5, (( OB[0] >> 3) &  0x01) );
        digitalWrite( 6, (( OB[0] >> 4) &  0x01) );
        digitalWrite( 7, (( OB[0] >> 5) &  0x01) );
        digitalWrite( 8, (( OB[0] >> 6) &  0x01) );
        digitalWrite( 9, (( OB[0] >> 7) &  0x01) );
    }
    if (len >= 2) {
        digitalWrite(10, (( OB[0] >> 0) &  0x01) );
        digitalWrite(11, (( OB[0] >> 1) &  0x01) );
        digitalWrite(12, (( OB[0] >> 2) &  0x01) );
        digitalWrite(13, (( OB[0] >> 3) &  0x01) );
        digitalWrite(A0, (( OB[0] >> 4) &  0x01) );
        digitalWrite(A1, (( OB[0] >> 5) &  0x01) );
        digitalWrite(A2, (( OB[0] >> 6) &  0x01) );
        digitalWrite(A3, (( OB[0] >> 7) &  0x01) );
    }
}

void loop(void) {
    cmri.proceess();   // process any C/MRI packets
}
