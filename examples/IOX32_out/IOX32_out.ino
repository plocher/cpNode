// *************************************************
// CMRI/Net example Sketch: Generic, IOX-32 only
// Onboard I/O:   0 bits of OUTPUT
//                0 bits of INPUT
// IOX I/O:      4x IOX-32  OUTPUTS 128 bits
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
const int  OutputBytes = 2 + 16;                   // 2x onboard plus 4x IOX-32 (8x devices, 2 bytes each)

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
    // NONE

    Wire.begin();
    iox.init( 0x20, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x20, IOX::PORT_B, IOX::OUT);
    iox.init( 0x21, IOX::PORT_A, IOX::OUT);
    iox.init( 0x21, IOX::PORT_B, IOX::OUT);

    iox.init( 0x22, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x22, IOX::PORT_B, IOX::OUT);
    iox.init( 0x23, IOX::PORT_A, IOX::OUT);
    iox.init( 0x23, IOX::PORT_B, IOX::OUT);

    iox.init( 0x24, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x24, IOX::PORT_B, IOX::OUT);
    iox.init( 0x25, IOX::PORT_A, IOX::OUT);
    iox.init( 0x25, IOX::PORT_B, IOX::OUT);

    iox.init( 0x26, IOX::PORT_A, IOX::OUT);     // IOX-32
    iox.init( 0x26, IOX::PORT_B, IOX::OUT);
    iox.init( 0x27, IOX::PORT_A, IOX::OUT);
    iox.init( 0x27, IOX::PORT_B, IOX::OUT);

    iox.write(0x20, IOX::PORT_A, 0x00);  // if desired, set initial output state for each port
    iox.write(0x20, IOX::PORT_B, 0x00);
    iox.write(0x21, IOX::PORT_A, 0x00);
    iox.write(0x21, IOX::PORT_B, 0x00);

    iox.write(0x22, IOX::PORT_A, 0x00);
    iox.write(0x22, IOX::PORT_B, 0x00);
    iox.write(0x23, IOX::PORT_A, 0x00);
    iox.write(0x23, IOX::PORT_B, 0x00);

    iox.write(0x24, IOX::PORT_A, 0x00);
    iox.write(0x24, IOX::PORT_B, 0x00);
    iox.write(0x25, IOX::PORT_A, 0x00);
    iox.write(0x25, IOX::PORT_B, 0x00);

    iox.write(0x26, IOX::PORT_A, 0x00);
    iox.write(0x26, IOX::PORT_B, 0x00);
    iox.write(0x27, IOX::PORT_A, 0x00);
    iox.write(0x27, IOX::PORT_B, 0x00);
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
    // NONE
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
    // onboard 16 bits (bytes 0 and 1) are inputs...
    // ... OB[0]
    // ... OB[1]

    // IOX32 #1 segments 1 & 2
    iox.write(0x20, IOX::PORT_A, OB[2]);
    iox.write(0x20, IOX::PORT_B, OB[3]);
    iox.write(0x21, IOX::PORT_A, OB[4]);
    iox.write(0x21, IOX::PORT_B, OB[5]);
    // IOX32 #2 segments 1 & 2
    iox.write(0x22, IOX::PORT_A, OB[6]);
    iox.write(0x22, IOX::PORT_B, OB[7]);
    iox.write(0x23, IOX::PORT_A, OB[8]);
    iox.write(0x23, IOX::PORT_B, OB[9]);
    // IOX32 #3 segments 1 & 2
    iox.write(0x24, IOX::PORT_A, OB[6]);
    iox.write(0x24, IOX::PORT_B, OB[7]);
    iox.write(0x25, IOX::PORT_A, OB[8]);
    iox.write(0x25, IOX::PORT_B, OB[9]);
    // IOX32 #4 segments 1 & 2
    iox.write(0x26, IOX::PORT_A, OB[6]);
    iox.write(0x26, IOX::PORT_B, OB[7]);
    iox.write(0x27, IOX::PORT_A, OB[8]);
    iox.write(0x27, IOX::PORT_B, OB[9]);
}

void loop(void) {
    cmri.proceess();   // process any C/MRI packets
}
