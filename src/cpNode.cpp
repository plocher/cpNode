//==================================================================================
//
//  cpNode - Control Point CMRI Node
//
//  ==================================================================================

#include <Wire.h>  // for the I/O expander
#include "cpNode.h"

/*
     **************************************************************
     **********        CMRI Protocol Message Format      **********
     **************************************************************
        - Initialization Message (I)  HOST to NODE
          SYN SYN STX <UA> <I><NDP><dH><dL><NS><CT(1)><CT(NS)> ETX

        - Poll for Data (P)  HOST to NODE
          SYN SYN STX <UA> <P> ETX

        - Read Data (R)  NODE To HOST  (Response to Poll)
          SYN SYN STX <UA> <R><IB(1)><IB(NS)> ETX

        - Transmit Data (T)  HOST to NODE
          SYN SYN STX <UA> <T><OB(1)><OB(NS)> ETX
*/

/*************************************************************
 *********            Library Header Files           *********
 *************************************************************/
#include <Arduino.h>
#include <Wire.h>  // for the I/O expander

// User defined callbacks

//----------------------------------------------------------------------
//  The pack input routine collects the bits from the digitalRead() calls
//  and puts the them into the correct bytes for transmission.
//
//  Two bytes are generally stored for onboard I/O, IB[0] and IB[1]
//-----------------------------------------------------------------------
extern "C" {
void pack(  byte *IB, int len);
}

// --------------------------------------------------------------------------
//  The output routine takes received bits from the output buffer and
//  writes them to the correct output ports using digitalWrite()
//
//  Two bytes are generally read for onboard I/O, OB[0] and OB[1]
//---------------------------------------------------------------------------
extern "C" {
  void unpack(byte *OB, int len);
}


cpNode::cpNode(void) {
    UA  = 0;
    nIB = 0;
    nOB = 0;
    DL  = 0;    // CMRINet per-char delay
    debugging = 0;
    invert_in = false;
    invert_out = false;
    Monitor = NULL;

}


byte cpNode::setNodeAddress(byte nodeAddr) {  // Node ID (0..64)
    //----------------------------------------------------------------
    // Valid addresses are 0..64
    // Set node address to 64 if an invalid decimal address is passed
    //----------------------------------------------------------------
    if (nodeAddr > 64) {
        nodeAddr = 64;
    }

    UA = nodeAddr + UA_Offset;  // 0..64 -> 'A'..DEL
    return nodeAddr;  // in case it changed...
}

// ***************************************************
// *******      Packet Processing Loop      **********
// ***************************************************
void cpNode::proceess(void) {
    //----------------------------------------------
    //  Check for any messages from the host
    //----------------------------------------------
    switch( getPacket() ) {
      case Packet_None:     break;                           // No data received, ignore

      case Packet_Init:     callback_initialize_cpNode();    // "I" Initialize        HOST -> NODE, set configuration parameters
                            break;

      case Packet_Poll:                                      // "P" Poll              HOST -> NODE request for input data
                            callback_pack_Node_Inputs();     // Read the input bits and latch for poll response
                            callback_CMRI_Poll_Response();   // "R" Receive           NODE -> HOST send input port data to host
                            break;

      case Packet_Transmit: callback_unpack_Node_Outputs();  // "T" Transmit (Write)  HOST -> NODE, set output bits
                            break;

      case Packet_Err:      // FALLTHROUGH
      case Packet_Ignore:   // FALLTHROUGH
      default:              callback_flush_CMRInet_to_ETX(); // Flush input buffer to ETX for various reasons
                            break;

     }
}




//----------------------------------------------------------------------
//  The input routines collect the bits from the onboard I/O and IO Expander
//  reads and put the them into the correct IB bytes for transmission.
//
//  The ports are read twice with a delay between for input debounce.
//  plocher: This comment does not match the actual code:
//       no debounce checking is done!
//       instead, the inputs were simply read twice, with the first
//       values discarded.
//
//  Two bytes are stored for "onboard IO bits", IB[0] and IB[1]
//  The rest of the bytes are used by the optional IO expanders
//-----------------------------------------------------------------------
void cpNode::callback_pack_Node_Inputs() {
    pack(IB, nIB);    // links against function found in user's sketch

    if (invert_in) {
        for (byte i = 0; i < nIB; i++) {
                IB[i] = ~(IB[i]);
        }
    }
}

// --------------------------------------------------------------------------
//  The output routines takes received bits from the ouput buffer and
//  writes them to the correct output port using digitalWrite() based
//  upon the value of cpNode_ioMap
//---------------------------------------------------------------------------
void cpNode::callback_unpack_Node_Outputs() {
    // Move the received bytes to the output buffer
    //---------------------------------------------
    for (byte i=0; i < nOB; i++) {
        OB[i] = CMRInet_Buf[i];
        if (invert_out) {
            OB[i] = ~(OB[i]);   // invert the bits if not active-low
        }
    }

    unpack(OB, nOB);    // links against function found in user's sketch
}

//-----------------------------------
//CMRInet Option Bit ProcessING
//-----------------------------------
void cpNode::callback_process_cpNode_Options() {
    // CPNODE IGNORES OPTION BITS
}

//-----------------------------------------------------------------------------------------
//  Perform any initialization and setup using the initialization message
//  NDP must be a "C" (cpNode) for initialization to be done
//
//    - cpNode Initialization Message (I)
//      SYN SYN STX <UA> <I><NDP> <DLH><DLL> <opts1><opts2> <NIN><NOUT> <000000><ETX>
//-----------------------------------------------------------------------------------------
void cpNode::callback_initialize_cpNode() {
    int DLH = 0,
        DLL = 0;

    // Set up transmit delay
    // 1 unit of delay(DL) is 10 microseconds
    //----------------------------------------
    DLH = CMRInet_Buf[1];
    DLL = CMRInet_Buf[2];
    DL  = (DLH * 256) + DLL;    // Transmit character delay value in 10 us increments
    DL  = DL * 10;

    if ((Monitor) && ((debugging) & (DEBUG_INIT))) {
        sprintf(debug_buffer, "INIT: DLH=%d, DLL=%d, DL/10=%ld DL=%ld\n",
                        DLH, DLL, DL/10, DL );
        Monitor->print(debug_buffer);
    }

    // Check if initialize message is for a cpNode
    // if so, process any options
    //--------------------------------------------
    if (cpNODE_NDP == CMRInet_Buf[0]) {
        callback_process_cpNode_Options();
    }
}


// -----------------------------------------------------
//  FLUSH the serial input buffer until an ETX
//  is seen or the input serial buffer is empty.
//
//  Used to ignore any inbound messages
//  not addressed to the node or to re-SYNc the protocol
//  parser if a garbled message found is.
// -----------------------------------------------------
void cpNode::callback_flush_CMRInet_to_ETX() {
    boolean done = false;
    while (!done) {
        if (cmriNet->available()) {
            if (cmriNet->read() == ETX) {
                done = true;
            }
         } else {
            done = true;
         }
    }
}


// ----------------------------------------------------------
// Send the input bytes to the host in response to a poll message.
// Bytes are moved from the IB(n) buffer to the transmit buffer.
// DLE characters are inserted for data values which are also
// protocol characters.
//
//    - Read Data (R) Message
//      SYN SYN STX <UA> <R><IB(1)><IB(NS)> ETX
//
//------------------------------------------------------------*/
void cpNode::callback_CMRI_Poll_Response() {
    int i=0;
    int   c;            // Handy character variable

    // Packet Header
    //--------------
    CMRInet_Buf[i++] = SYN;
    CMRInet_Buf[i++] = SYN;
    CMRInet_Buf[i++] = STX;

    // Message Header
    //---------------
    CMRInet_Buf[i++] = UA;
    CMRInet_Buf[i++] = 'R';

    // Load the onboard input bytes into the output buffer
    //----------------------------------------------------
    if (nIB > 0) {
        for (byte j=0; j < nIB; j++) {
            c = IB[j];  // Insert a DLE if the output byte value is a protocol character
            switch(c)  {
            //  case SYN:   // SYNcs are ignored to conform to the published protocol
                case STX:
                case ETX:
                case DLE:
                          CMRInet_Buf[i++] = DLE;
                          break;
            }
            CMRInet_Buf[i++] = c;
            IB[j] = 0;   // Clear the latched inputs
        }
    }



    // Add the ETX and send the complete buffer
    //-----------------------------------------
    CMRInet_Buf[i++] = ETX;

    // Send the packet to the host
    //----------------------------
    for (byte j=0; j<i; j++) {
        cmriNet->write(CMRInet_Buf[j]);

        // If a transmit delay was set, delay microseconds
        //------------------------------------------------
        if (DL > 0) {
            delayMicroseconds( DL );   // value in microseconds
        }
    }

    if ((Monitor) && ((debugging) & (DEBUG_POLL))) {
        sprintf(debug_buffer, "Poll Response nIB=%d [\n", nIB );
        const char *sep = "";
        for (byte j=0; j<i; j++) {
            char item[8];
            sprintf(item, "%s0x%02x", sep, CMRInet_Buf[j]);
            strcat(debug_buffer, item);
            sep = ", ";
        }
        strcat(debug_buffer, "]\n");
        Monitor->print(debug_buffer);
    }
}


// --------------------------------------------------------
//  Read a byte from the specified serial port.
//  Return the character read
// --------------------------------------------------------
char cpNode::callback_read_CMRI_Byte() {
    while (true) {
        if (cmriNet->available() > 0) {
            return char(cmriNet->read());
        }
    }
}


// ----------------------------------------------------------------------
//  Read the message from the Host and determine if the message is
//  for this node.  The whole message is read, any data to be processed
//  is stored in CMRInet_Buf[], stripped of protocol characters.
//
//  If the node address does not match, the data is ignored.
//
//  The data message body is processed by an appropriate message handler.
//
//    - Initialization Packet (I)
//      SYN SYN STX <UA> <I> <NDP> <dH><dL> <NS> <CT(1)><CT(NS)> ETX
//
//    - Poll for Data (P)
//      SYN SYN STX <UA> <P> ETX
//
//    - Read Data (R)
//      SYN SYN STX <UA> <R> <IB(1)><IB(NS)> ETX
//----------------------------------------------------------------------

int cpNode::getPacket() {
    byte resp = Packet_Err;
    int  inCnt;
    int   c;            // Handy character variable

    boolean reading = true,
            inData = false;

    //-----------------------------------
    // Check input buffer for a character
    //-----------------------------------
    if (cmriNet->available() <= 0) {
       return Packet_None;
    }

    //--------------------
    // Process the message
    //--------------------
    byte matchID = 0;
    inCnt = 0;

    do {
        c = callback_read_CMRI_Byte();  // read the byte

        switch( int(c) ) {
        case STX:   // Start of message header, start parsing protocol message
                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) { Monitor->print("STX "); }

                    // Read node address and message type
                    //-----------------------------------
                    matchID = callback_read_CMRI_Byte();  // Node Address

                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
                        sprintf(debug_buffer, " ua=%c (%d)", matchID, matchID-UA_Offset);
                        Monitor->print(debug_buffer);
                    }

                    // If node ID does not match, exit and flush to ETX in outer loop
                    //---------------------------------------------------------------
                    if (matchID != UA)  {
                          if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) { Monitor->print("Not for me\n"); }
                          resp = Packet_Ignore;
                          reading=false;
                    } else {
                         // Set response code based upon message type
                         //------------------------------------------
                         c = callback_read_CMRI_Byte();        // Message Type

                         if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
                             sprintf(debug_buffer, " MsgType=%c IB=", c);
                             Monitor->print(debug_buffer);
                         }

                         switch( c ) {
                           case 'I':          // Initialization
                                              resp = Packet_Init;      break;
                           case 'P':          // Poll
                                              resp = Packet_Poll;      break;
                           case 'R':          // Read
                                              resp = Packet_Read;      break;
                           case 'T':          // Write (Transmit)
                                              resp = Packet_Transmit;  break;
                           default:           // Unknown - Error
                                              resp = Packet_Err;
                                              reading = false;
                                              break;
                          }

                        // Completed the header, go into message data mode
                        //------------------------------------------------
                        inData = true;
                    }
                    break;

        case ETX:   // End of message, read complete
                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) { Monitor->print(" ETX "); }
                    reading = false;
                    break;

        case DLE:   // Read the next byte regarDLEss of value and store it
                    CMRInet_Buf[inCnt++] = callback_read_CMRI_Byte();

                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
                        sprintf(debug_buffer, "DLE(0x%02x) ", byte(CMRInet_Buf[inCnt-1]));
                        Monitor->print(debug_buffer);
                    }

                    break;

        case SYN:   // SYNc character Ignore it if not reading data
                    if (inData) CMRInet_Buf[inCnt++] = c;
                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) { Monitor->print(" SYN "); }
                    break;

        default:    // Stuff the data character into the receive buffer

                    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
                        sprintf(debug_buffer, "{0x%02x}", byte(c));
                        Monitor->print(debug_buffer);
                    }

                    CMRInet_Buf[inCnt++] = c;
                    break;
        }

        // Check for buffer overrun and terminate the read if true
        //--------------------------------------------------------
        if (inCnt > CMRInet_BufSize) {
            reading = false;
            resp = Packet_Err;

            if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
                sprintf(debug_buffer, "\nBuffer Overrun inCnt = %d\n", inCnt);
                Monitor->print(debug_buffer);
            }
        }

    } while (reading);

    // Null terminate the input buffer
    //--------------------------------
    CMRInet_Buf[inCnt] = 0;

    //---------------------------------------------------------
    // Match the node address in the message to the UA+65 value
    // if no match, ignore message, not addessed to this node
    //---------------------------------------------------------

    if ((Monitor) && ((debugging) & (DEBUG_PROTOCOL))) {
        sprintf(debug_buffer, "\n ->inCnt = %d]n", inCnt);
        Monitor->print(debug_buffer);
    }
    return resp;
}  // getPacket

void IOX::init(int i2cAddress, byte port, bool isInput) {
    if (isInput == IOX::IN) {

        Wire.beginTransmission(i2cAddress);       // Board Address (20...27)
        Wire.write(MCP28017_IO + port);       // A=0 or B=1 Port
        Wire.write(MCP28017_PORT_INPUT);      // Set port to Inputs
        Wire.endTransmission();

        Wire.beginTransmission(i2cAddress);
        Wire.write(MCP28017_PULLUP + port);
        Wire.write(MCP28017_PORT_PULLUPS);     // Use weak pullups
        Wire.endTransmission();

        Wire.beginTransmission(i2cAddress);
        Wire.write(MCP28017_ACTIVELOW + port);
        Wire.write(MCP28017_PORT_ACTIVELOW);   // Active low
        Wire.endTransmission();

    } else {  // set as outputs

        Wire.beginTransmission(i2cAddress);       // Board Address (20...27)
        Wire.write(MCP28017_IO + port);       // A=0 or B=1 Port
        Wire.write(MCP28017_PORT_OUTPUT);     // Set port to Output
        Wire.endTransmission();

    }
}


void IOX::write(int i2CAddress, byte port, byte data) {
    Wire.beginTransmission(i2CAddress);           // Board Address
    Wire.write(MCP28017_GPIO + port);         // A or B Port
    Wire.write(data);                             // Data to send
    Wire.endTransmission();
}

int IOX::read(int i2CAddress, byte port) {
    Wire.beginTransmission(i2CAddress);           // Board Address
    Wire.write(MCP28017_GPIO + port);         // A or B Port
    Wire.endTransmission();

    Wire.requestFrom(i2CAddress,1);               // Data to read
    return Wire.read();
}
