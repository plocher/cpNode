// *************************************************
// CMRI/Net example Sketch: ProMini
// Onboard I/O:   0 bits of OUTPUT
//               16 bits of INPUT
// IOX I/O:      2x IOX-32 OUTPUTS
// *************************************************
//
//  Reserved I/O pins are:
//   D0 - RX  CMRI RS485 Receive
//   D1 - TX  CMRI RS485 Transmit


#include "cpNode.h"
#include <Wire.h>  // for the I/O expander

cpNode cmri;    // Processing logic for handling CMRINet packets
IOX  iox;

const int  nodeID = 0;                            // 0...63 (nodeID + ord('A') => 'A'..chr(127))
const long CMRINET_SPEED = 19200;                 // 9600, 19200 ...

// the following need to match the code in setup(), pack() and unpack()...
const int  InputBytes  = 2 + 0;                    // 2x onboard
const int  OutputBytes = 2 + 4;                    // 2x onboard plus 2x IOX-32

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
    pinMode( 2, INPUT_PULLUP);        // D2
    pinMode( 3, INPUT_PULLUP);        // D3
    pinMode( 4, INPUT_PULLUP);        // D4
    pinMode( 5, INPUT_PULLUP);        // D5
    pinMode( 6, INPUT_PULLUP);        // D6
    pinMode( 7, INPUT_PULLUP);        // D7
    pinMode( 8, INPUT_PULLUP);        // D8
    pinMode( 9, INPUT_PULLUP);        // D9

    pinMode(10, INPUT_PULLUP);        // D10
    pinMode(11, INPUT_PULLUP);        // D11
    pinMode(12, INPUT_PULLUP);        // D12
    pinMode(13, INPUT_PULLUP);        // D13
    pinMode(A0, INPUT_PULLUP);        // A0
    pinMode(A1, INPUT_PULLUP);        // A1
    pinMode(A2, INPUT_PULLUP);        // A2
    pinMode(A3, INPUT_PULLUP);        // A3

    Wire.begin();
    iox.init( 0x20, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x20, IOX::PORT_B, IOX::OUT);
    iox.init( 0x21, IOX::PORT_A, IOX::OUT);
    iox.init( 0x21, IOX::PORT_B, IOX::OUT);

    iox.init( 0x22, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x22, IOX::PORT_B, IOX::OUT);
    iox.init( 0x23, IOX::PORT_A, IOX::OUT);
    iox.init( 0x23, IOX::PORT_B, IOX::OUT);

    iox.write(0x20, IOX::PORT_A, 0x00);  // if desired, set initial output state for each port
    iox.write(0x20, IOX::PORT_B, 0x00);
    iox.write(0x21, IOX::PORT_A, 0x00);
    iox.write(0x21, IOX::PORT_B, 0x00);

    iox.write(0x22, IOX::PORT_A, 0x00);
    iox.write(0x22, IOX::PORT_B, 0x00);
    iox.write(0x23, IOX::PORT_A, 0x00);
    iox.write(0x23, IOX::PORT_B, 0x00);
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
        IB[0] |= (!digitalRead( 2) << 0);
        IB[0] |= (!digitalRead( 3) << 1);
        IB[0] |= (!digitalRead( 4) << 2);
        IB[0] |= (!digitalRead( 5) << 3);
        IB[0] |= (!digitalRead( 6) << 4);
        IB[0] |= (!digitalRead( 7) << 5);
        IB[0] |= (!digitalRead( 8) << 6);
        IB[0] |= (!digitalRead( 9) << 7);
    }
    if (len >= 2) {
        IB[1] = 0;
        IB[1] |= (!digitalRead(10) << 0);
        IB[1] |= (!digitalRead(11) << 1);
        IB[1] |= (!digitalRead(12) << 2);
        IB[1] |= (!digitalRead(13) << 3);
        IB[1] |= (!digitalRead(A0) << 4);
        IB[1] |= (!digitalRead(A1) << 5);
        IB[1] |= (!digitalRead(A2) << 6);
        IB[1] |= (!digitalRead(A3) << 7);
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
    // onboard 16 bits (bytes 0 and 1) are inputs...
    if (len >= 3) iox.write(0x20, IOX::PORT_A, OB[2]);
    if (len >= 4) iox.write(0x20, IOX::PORT_B, OB[3]);
    if (len >= 5) iox.write(0x21, IOX::PORT_A, OB[4]);
    if (len >= 6) iox.write(0x21, IOX::PORT_B, OB[5]);
}

void loop(void) {
    cmri.proceess();   // process any C/MRI packets
}
