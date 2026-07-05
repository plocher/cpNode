// *************************************************
// CMRI/Net example Sketch: Xiao I2C-only
// Onboard I/O:  None
// IOX I/O:      2x IOX-16s (one IOX16 in, other IOX16 out)
// *************************************************
//
//  Reserved I/O pins are:
//   D7 - RX  CMRI RS485 Receive
//   D6 - TX  CMRI RS485 Transmit
//   D4 - SDA I2C Data
//   D5 - SCL I2C Clock
//   D3 - TXEN RS422/485 Transmit Enable


// In JMRI's "Configure C/MRI Nodes" pane, the node address field is 
// entered in base 10 (decimal) and represents the pre-offset raw hardware node ID.

// The field in the JMRI GUI accepts standard decimal numbers (ranging from 0 to 127).
// Protocol Offset: 
//     The actual CMRInet wire protocol requires the node address 
//     to be offset by a decimal value of 65 (which is 0x41 in hexadecimal, 
//     corresponding to the ASCII character 'A').

// JMRI handles this offset automatically behind the scenes. It takes your decimal entry, 
// adds 65 to it, and transmits that resulting byte value in the packet header.

// Example: Entering "40" in JMRI Node Address field
//     In JMRI you track it simply as Node 40.
//     On the Wire (Calculated): JMRI calculates the protocol byte as: 
//         40 (Your Input) + 65 (Protocol Offset) = 105
//     The first data byte transmitted in the packet header will be 105 in base 10
//     (0x69 in base 16, corresponding to the lowercase ASCII letter 'i').
//
// NodeID below should be the same value as used in JMRI, in this example, 40

// JMRI manages C/MRI (Computer/Model Railroad Interface) cpNode hardware by 
// mapping physical I/O pins to a continuous software array of 8-bit bytes. It 
// translates logical layout objects (sensors, turnouts, lights) into specific bits 
// and periodically transmits these states using the CMRInet polling protocol.
//
// Polling and Data Model
//     Communication between JMRI and a C/MRI node operates on a master/slave poll 
//     architecture. JMRI acts as the master and periodically sends a command asking 
//     the remote node to report its current input bits.
// Memory Structure: 
//    Internally, JMRI maintains two arrays for each node: an inputArray and an 
//    outputArray. Every array element is exactly 8 bits.
//
// Byte and Bit Translation (Under the Hood)
//   When an action occurs on the layout (e.g., a turnout is thrown), JMRI must calculate 
//   the exact byte index and bit offset to update.
// Calculating the Byte: JMRI uses the following zero-indexed formula to find the specific 
//   byte in its array for a given layout address:  byteNumber = ( bitNumber - 1 ) / 8
// Calculating the Bit: Once the byte is identified, JMRI uses a bitwise shift to modify 
//   the target bit within that specific byte:      bit = 1 << (( bitNumber - 1 ) mod 8 )
//
// Handling Input and Output Signals
//   JMRI binds physical pins to layout logic.
//   Inputs (Sensors): A physical sensor that trips on the layout goes from the cpNode to JMRI. 
//     Physical hardware is defined in the JMRI Sensor Table as CS variables (e.g., CS1011 represents 
//     node 1, bit 11). When JMRI polls the node, the node responds with a byte array. JMRI parses 
//     the byte, evaluates the bit state, and updates the state of the corresponding JMRI Sensor object.
//   Outputs (Turnouts/Lights): Outputs flow from JMRI to the node. These are registered as CT (Turnout) 
//     or CL (Light) objects. When a state changes inside JMRI, the outputArray is updated and JMRI marks 
//     the node's byte as "dirty." On the next poll request, the updated byte is transmitted over the 
//     RS422/RS485 stream.
//
// Hardware and Microcontroller (cpNode) Execution
//   When the microcontroller node receives a CMRI SET/TRANSMIT packet, it extracts the 8-bit bytes from the 
//   payload and maps them to physical Arduino pins using the unpack() routine below. Similarly, 
//   when the node receives a POLL, it uses pack() to construct the 8-bit byte arrays that 
//   the JMRI software will read.

#include "cpNode.h"
#include <Wire.h>  // for the I/O expander
#include <elapsedMillis.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

cpNode cmri;    // Processing logic for handling CMRINet packets
IOX  iox;       // I2C i/o routines for 23017 IO expander

#define CMRI_NODE_DESCRIPTION  "XiaoC6"

const int  nodeID = 30;                           // 0...63 just as in JMRI
const long CMRINET_SPEED = 28800;                 // 9600, 19200, 28800 ...
const int  TXEN_PIN = D3;

// JMRI's CMRI CPNODE card type presumes 2 bytes of onboard I/O
// This sketch builds on that node type, and ignores these initial 2 bytes
// the following need to match the code in setup(), pack() and unpack()...
const int  InputBytes  = 4;                    // 1 IOX-16 board (2x 8-bit ports) (A)
const int  OutputBytes = 4;                    // 1 IOX-16 board (2x 8-bit ports) (B)

//==============================================
//====   I2C LCD Display    SSD1306         ====
//==============================================

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
elapsedMillis timeElapsed; 
#define MAX_timeElapsed 10 // ms

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT); 

unsigned long errorcount = 0L;
unsigned long rxcount    = 0L;
unsigned long txcount    = 0L;
int loopcount            = 0;
bool countup             = true;
char spinner[] = "-\\|/";

void display_info(void) {
    char r = spinner[rxcount % (sizeof(spinner) - 1)];
    char t = spinner[txcount % (sizeof(spinner) - 1)];
    display.setCursor(0,10); display.printf(F("Transmit: %c"), r);
    display.setCursor(0,20); display.printf(F("Poll:     %c"), t);

    // erase current progress sprite
    display.setCursor(loopcount,50); display.print(" ");
    if (countup) {
        loopcount = loopcount + 1;
        if (loopcount == 120) {
            countup = false;
        }
    } else {
        loopcount = loopcount - 1;
        if (loopcount == 0) {
            countup = true;
        }
    }

    // draw new
    display.setCursor(loopcount,50); display.print("*");

    // Update OLED screen
    display.display();
}

void setup(void) {
    // *************************************************
    // *******   Setup  Onboard I/O           **********
    // *************************************************

    // Arduino "Console"
    Serial.begin(115200);
    // while (!Serial) {
    //     ;  // wait for serial port to connect. Needed for native USB port only
    // }
    delay(10);
    Serial.printf("Setup:\n");

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.printf("Error: display begin\n");
        for(;;); // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.dim(true);
    display.setTextSize(1);             // Normal 1:1 pixel scale
    // Set background to black (erase) and foreground to white (draw)
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 0);  display.print("cpNode: "); display.print(nodeID); display.print(" "); display.print(CMRI_NODE_DESCRIPTION);
    display.display();

    // CMRI Serial device
    Serial1.begin(CMRINET_SPEED, SERIAL_8N2, RX /* 7 */, TX /* 6 */ );

    cmri.setCMRIPort(&Serial1, TXEN_PIN);
    cmri.setDebugPort(&Serial);     // for debugging on the USB port

    cmri.setNodeAddress(nodeID);          // Set the node address
    cmri.setNumInputBytes(InputBytes);    // reflected in pack()
    cmri.setNumOutputBytes(OutputBytes);  // reflected in unpack()

    cmri.invertInputs(  false );           // invert  bits?
    cmri.invertOutputs( false );

    // *************************************************
    // *******   Setup  I/O Expander (IOX)    **********
    // *************************************************

    Wire.begin();
    iox.init( 0x20, IOX::PORT_A, IOX::IN);
    iox.init( 0x20, IOX::PORT_B, IOX::OUT);

    iox.init( 0x21, IOX::PORT_A, IOX::IN);
    iox.init( 0x21, IOX::PORT_B, IOX::OUT);
    // iox.write(0x21, IOX::PORT_A, 0x00);  // if desired, set initial output state for each port
    // iox.write(0x21, IOX::PORT_B, 0x00);

    Serial.printf("Setup Done:\n");
}

// ---------------------------------------------------------------------------
// pack() is called whenever there is a need to read bits from the layout
//        in response to a poll request
//
//  The pack input routine collects the bits from the onboard IO ports and
//  external I/O expanders and puts the them into the correct IB array bytes
//  for transmission back to the control host.
//
//  len bytes (as set by setNumInputBytes above) need to be read
// ---------------------------------------------------------------------------

void pack(byte *IB, int len) {
    txcount++;
    IB[0] = 1;
    IB[1] = 2;
    IB[2] = iox.read(0x20, IOX::PORT_A);
    IB[3] = iox.read(0x21, IOX::PORT_A);
}

// ---------------------------------------------------------------------------
// unpack() is called whenever there is a need to write bits out to the layout
//
//  The unpack output routine needs to take the received bits from the OB
//  output buffer and write them to the correct output ports using either
//  digitalWrite() or the IO expanders
//
//  len bytes (as set by setNumOutputBytes above) are available to be written
//----------------------------------------------------------------------------

void unpack(byte *OB, int len) {
    rxcount++;
    // ignore OB[0]
    // ignore OB[1]
    iox.write(0x20, IOX::PORT_B, OB[2]);
    iox.write(0x21, IOX::PORT_B, OB[3]);
}

void loop(void) {
    cmri.process();   // process any C/MRI packets
    if (timeElapsed >= MAX_timeElapsed) {
        timeElapsed = 0;
        display_info();
    }
}
