# MRCS CMRI protocol handling for 'duino Nodes

## License: Creative Commons Attribution-ShareAlike 3.0 Unported License

MRCS cpNode CMRI Kernel library

CMRI serial protocol node implemented in an Arduino style system board.
The Modern Devices BBLeo, Bare Bones Leonardo (ATMega32u4) is the target Arduino style system board.
v1.6 adds support for an Arduino Pro-Mini, although without the monitor serial port.

Implements the CMRI Serial Protocol designed by Dr. Bruce Chubb and published publicly in various books, magazines, and articles.
  * The physical link is RS485, 4-wire, half duplex, serial.
  * Each node has a one byte address in the range of 0-127.
  * The Host polls the CMRI nodes for data.
  * Port assignments represent the defined capability of a cpNode.
  * The base node data configuration is two bytes in, two bytes out.
  * All needed signal pins are connected via pin headers to the node board.

See
  * Eagle Project [MRCS cpNode](https://www.spcoast.com/pages/MRCS-cpNode.html)
  * Eagle Project [MRCS MRCS-cpNode-ProMini](https://www.spcoast.com/pages/MRCS-cpNode-ProMini.html)
  * Eagle Project [MRCS MRCS-BBProMini](https://www.spcoast.com/pages/MRCS-BBProMini.html)

## Usage

The example sketches follow a common template:
  * initialize the input and output ports
  * define pack() and unpack() routines to read and write data to the ports, and
  * call the CMRINet protocol handler in loop()

For compatibility with Chuck's original cpNode_kernel sketch, each example sketch is built around 
slightly different definitions of the processor's onboard I/O pins, such as 8-in/8-out, 16-in, etc.  
In addition, these sketches presume that the first two bytes of both the input and output buffers (IB and OB)
are for the onboard I/O pins, even if they are not used.
Any remaining bytes in those buffers are for the optional I2C expanders.

Pick the example that most closely matches your needs, and modify it to suit.  

### Customization

The protocol handler in the cpNode library (.process()) is designed to be 
generic, with operational parameters defined in your sketch:
  * The Node ID (0 is 'A', 1 is 'B'...)
  * The serial port to use (since you set it up, you get to set its speed)
  * What serial port to use (if any) for debugging,
  * How many Input and Output bytes to expect, 
  * Whether to invert input and/or output bits automatically

```c++
// Example with all pins being INPUTS, and no I2C Expanders...

const int  nodeID = 0;                     // 0...63  => 'A', 'B", ...
const long CMRINET_SPEED = 19200;          // 9600, 19200 ...
const int  InputBytes  = 2;                // 2x onboard  (no IOX...)
const int  OutputBytes = 2;                // 2x onboard

...

    Serial1.begin(CMRINET_SPEED);          // Set up and Open the CMRInet port
    while(!Serial1) { };

    cmri.setCMRIPort(&Serial1);           // for CMRI/Net protocol
    cmri.setDebugPort(&Serial);           // for debugging

    cmri.setNodeAddress(nodeID);          // Set the node address
    cmri.setNumInputBytes(InputBytes);    // reflected in pack()
    cmri.setNumOutputBytes(OutputBytes);  // reflected in unpack()

    cmri.invertInputs(  true );           // invert bits?
    cmri.invertOutputs( true );

```
In addition, all I/O port setup, reading and writing is done with code that lives in your sketch,
in particular, the setup(), pack() and unpack() routines

```c++
void setup() {
...
    // *************************************************
    // *******   Setup  Onboard I/O           **********
    // *************************************************
    pinMode( 2, INPUT_PULLUP);        // D2 - ProMini
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
...
}

```

```c++
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
```

```c++
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
}
```

### I2C expander support

Any sketch can be extended to add support for IOX I2C I/O expanders by adding the following:
  * Include Wire.h
  * Define an IOX variable to give access to the IOX-16 and IOX-32 devices
  * increase the declared quantity of Input or Output bytes
  * call Wire.begin()
  * initialize each device port as an input or output
  * add write or read commands to the unpack and pack routines

Look at the basic BBLeo and ProMini examples for inspiration.

```c++
#include <Wire.h>
...
IOX  iox;

const int  InputBytes  = 2 + 2;  // 2x onboard plus 1x IOX-16
const int  OutputBytes = 2 + 2;  // 2x onboard plus 1x IOX-16
...
... in setup():
...
    Wire.begin();
    iox.init( 0x20, IOX::PORT_A, IOX::IN);  // first card is all inputs
    iox.init( 0x20, IOX::PORT_B, IOX::IN);

    iox.init( 0x21, IOX::PORT_A, IOX::OUT); // second is all outputs
    iox.init( 0x21, IOX::PORT_B, IOX::OUT);

    iox.write(0x21, IOX::PORT_A, 0x00);  // if desired, set initial output state
    iox.write(0x21, IOX::PORT_B, 0x00);
    
...
... in pack():
...

    if (len >= 3) IB[2] = iox.read(0x20, IOX::PORT_A);
    if (len >= 4) IB[3] = iox.read(0x20, IOX::PORT_B);

...
... and in unpack():
...
    if (len >= 3) iox.write(0x21, IOX::PORT_A, OB[2]);
    if (len >= 4) iox.write(0x21, IOX::PORT_B, OB[3]);
```


## Debugging

  * The "main" hardware serial port on the LOE and ProMini MCUs is used for CMRINet.  If the MCU you are using
    supports multiple serial ports (such as the Leonardo ATMega32u4, with Serial and Serial1), the USB port can be
    used to display status and debug print statements on the Arduino IDE Monitor window.  
    To enable this feature, use the following code to your setup() routine:
    ```c++
    Serial1.begin(CMRINET_SPEED);   // Set up and Open the CMRInet port
    while(!Serial1) { };

    cmri.setCMRIPort(&Serial1);     // for CMRI/Net protocol
    cmri.setDebugPort(&Serial);     // for debugging on the USB port
    ```
  * The JMRI node definition (number of inputs and/or outputs, baud rate, node ID...) must match what is defined in
    your sketch, as there is no runtime validation or verification that they are the same.
  * The pack() and unpack() routines have a length parameter.  This is the value you provided in setup():
    ```c++
    cmri.setNumInputBytes(InputBytes);    // reflected in pack()
    cmri.setNumOutputBytes(OutputBytes);  // reflected in unpack()
    ```
    These lengths MUST match the node definition you configured in JMRI (or whatever control host software you are using).
    
    They are passed into these routines so that the code can protect itself from accessing I/O buffer locations that
    might not have data from or going back to the control host.
    * if you tell JMRI that you have 1x IOX-16 expander, with 1 byte input and 1 byte output, 
      JMRI will only send/expect 3x bytes in its input and output routines:
      * IB[0], IB[1] and IB[2] : 2 bytes for onboard I/O + 1 byte for one of the IOX-16's ports
      * OB[0], OB[1] and OB[2] : 2 bytes for onboard I/O + 1 byte for the other IOX-16 port

## Release Notes:
### Authors:
  * Chuck Catania, 2013-2016
  * John Plcoher, 2021

### Revision History:
#### v2.0   10/08/2021  Plocher:
  * Refactor into an Ardiono Library with cpNode and IOX classes
  * Simplified IOX handling (removed several layers of abstraction && assumptions)
  * Modularized sketch code with the intent that all I/O handling would be in the sketch itself and 
    not hidden or abstracted in a library
  * Made example sketches for all of the historical variations to encourage 
    individual I/O customizations that can be maintained without interference from 
    core protocol handler (library) changes

  * Code Sizes:  
    * v2.0 Pro Mini without IOX
      * Sketch uses 7002 bytes (22%) of program storage space. Maximum is 30720 bytes.
      * Global variables use 950 bytes (46%) of dynamic memory, leaving 1098 bytes for local variables. Maximum is 2048 bytes.
    * v2.0 Pro Mini with IOX
      * Sketch uses 8070 bytes (26%) of program storage space. Maximum is 30720 bytes.
      * Global variables use 1016 bytes (49%) of dynamic memory, leaving 1032 bytes for local variables. Maximum is 2048 bytes.                      

#### v1.6   05/25/2021  Plocher:
  * Significant code cleanup, refactoring and simplification
  * Add debugging flags and associated print statements...
  * Removed APortMap array and convoluted indexing in favor of unrolled digitalRead/Writes
  * Uses less FLASH and RAM
  * Added PROMINI_8OUT8IN for MRCS cpNode Control Point Pro Mini
  * Renamed BASE_NODE* to BBLEO*
  * Use runtime constants to take advantage of compiler optimizer to remove unused and unreachable code
  * Add support for active low or active high as a default
  * Add support for inverting inputs and outputs

  * Code sizes:
    * 1.6 BBLeo
      * Sketch uses 9114 bytes (31%) of program storage space. Maximum is 28672 bytes.
      * Global variables use 1117 bytes (43%) of dynamic memory, leaving 1443 bytes for local variables. Maximum is 2560 bytes.
    * 1.6 ProMini (no debug serial port)
      * Sketch uses 6100 bytes (19%) of program storage space. Maximum is 30720 bytes.
      * Global variables use 975 bytes (47%) of dynamic memory, leaving 1073 bytes for local variables. Maximum is 2048 bytes.

####   v1.5   09/12/2016  Changed digital pin name mnemonics to hard coded pin numbers to keep the pre-processor happy.
  * Code size:
    * 1.5 BBLeo Only
      * Sketch uses 9418 bytes (32%) of program storage space. Maximum is 28672 bytes.
      * Global variables use 1169 bytes (45%) of dynamic memory, leaving 1391 bytes for local variables. Maximum is 2560 bytes.

####   v1.4.4 03/28/2016  Removed #define BASE_NODE_SERVO as there was no support for the servo library

####   v1.4.2 05/27/2015  (TVerberg) Corrected typo errors in Base_Node_12out_4in output unpacking,
  * (TVerberg) Corrected Base_Node_Servo input packing routines.
  * Rearranged CMRInet protocol responses to put non-message functions at the front of the list.
  * Added Init message DL/DH delay processing for Classic node compatability.
  * Added Init_CA_Ports_OFF boolean.  If set to true, all ports driving Common Anode LEDs will be forced OFF at bootup.
  * Changed CMRInet_BufSize to int and increased length to 260 to handle the max SUSIC + 4 pad buffer
  * Changed Flush_CMRInet_To_ETX() to exit on either seeing ETX or empty serial buffer
  * Changed DEBOUNCE_DELAY from 10 ms to 2 ms
  * Added BASE_NODE_RSMC_LOCK configuration to support locking an RSMC controlled turnout with Dennis Drury's switch lock board.
  * Added BASE_NODE_8OUT8IN.  Sets D4-D11 as outputs, D12-A5 as inputs.

####   v1.4.1 04/15/2015  Changed CMRINET_SPEED definition from int to long for network speeds greater than 28800 bps

####   v1.4   06/25/2014  Added the 12 output, 4 input standard configuration per Dick Johannes of the NMRA HUB Division
  * Moved debug option and SN variables out of Node Configuration Parameters area.

####   v1.3   04/06/2014  Fixed issue with BASE_NODE_8IN8OUT, BASE_NODE_16OUT high bit B8 output not moved to A5.
  * Bit extraction loop ended one bit early.  Other routines worked because loop limit was less than
  * maximum port map index.

####   v1.2   03/06/2014  Fixed issue with BASE_NODE_8IN8OUT where port setup did not match specification.
  * This was an implementation deviation from the design specification.

####   v1.1   03/01/2014  Fixed issue with BASE_NODE_8IN8OUT where the byte assignment was flipped.

####   v1.0   01/04/2014  Released

####    0.0d  08/24/2013  Initial template definition


