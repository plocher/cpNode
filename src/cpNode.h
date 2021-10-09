#pragma once

/*
    ==================================================================================
                  cpNode                Control Point CMRI Node
    ==================================================================================

    cpNode concept and design committed on 5/31/2013 by Chuck Catania and Seth Neumann
    Model Railroad Control Systems, LLP

    This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
    To view a copy of the license, visit http://creativecommons.org/licenses/by-sa/3.0/deed.en_US

    Authors:
        Chuck Catania - Model Railroad Control Systems
        John Plocher - SPCoast


    This is the core CMRI serial protocol node implementation for an Arduino style system board.

    Implements the CMRI Serial Protocol designed by Dr. Bruce Chubb and published publicly in various books, magazines, and articles.
    The physical link is RS485, 4-wire, half duplex, serial.
    Each node has a one byte address in the range of 0..64 ('A'..DEL).
    The Host polls the CMRI nodes for data.
    Port assignments represent the defined capability of a cpNode.
*/

#include <Arduino.h>    // for the 'duino infrastructure
#include <Wire.h>       // for the I/O expander

// User defined Input/Output handler callbacks
extern "C" {

    //----------------------------------------------------------------------
    //  The pack input routine collects the bits from the digitalRead() calls
    //  and puts the them into the correct bytes for transmission.
    //
    //  Two bytes are generally stored for onboard I/O, IB[0] and IB[1]
    //-----------------------------------------------------------------------
    void pack(  byte *IB, int len);


    // --------------------------------------------------------------------------
    //  The output routine takes received bits from the output buffer and
    //  writes them to the correct output ports using digitalWrite()
    //
    //  Two bytes are generally read for onboard I/O, OB[0] and OB[1]
    //---------------------------------------------------------------------------
    void unpack(byte *OB, int len);
}

class cpNode {
protected:
    // Library debugging ...
    enum {
        DEBUG_ANNOUNCE = 0x01,    // print config info at end of setup...
        DEBUG_PROTOCOL = 0x02,
        DEBUG_POLL     = 0x04,
        DEBUG_INIT     = 0x08,
    };
    unsigned int debugging;

    //--------------------
    // Protocol characters
    //--------------------
    static const char
               STX    = 0x02,
               ETX    = 0x03,
               DLE    = 0x10,
               SYN    = 0xFF;

    //-------------
    // Data buffers
    //-------------
    static const int CMRInet_BufSize  = 260;           //  Max SUSIC + 4 pad
    static const int IO_bufsize       = (2 + 16) + 4;  //  cpNode (2) + IOX Ports (8x2=16) + pad (Max expected for a cpNode)

    static const char cpNODE_NDP = 'C';     // Node Definition Parameter for a cpNode - Control Point Node
    static const byte UA_Offset  = 'A';     // Decimal 65, Hex 0x41 per CMRI protocol spec

    // Read responses
    //---------------
    static const int
               Packet_None    = 0,  //  No character received
               Packet_Err     = 1,  //  Error happened
               Packet_Ignore  = 2,  //  Packet not addressed for this node, flush to ETX
               Packet_Init    = 3,  //  "I" Message
               Packet_Poll    = 4,  //  "P" Message
               Packet_Read    = 5,  //  "R" Message
               Packet_Transmit= 6;  //  "T" Message
public:
    cpNode(void);

    void setCMRIPort(Stream *port)              { cmriNet = port; }
    void setDebugPort(Stream *port)             { Monitor = port; }
    byte setNodeAddress(byte nodeAddr);
    byte getNodeAddress(void)                   { return UA - UA_Offset; }
    void invertInputs(bool i)                   { invert_in = i; }
    void invertOutputs(bool i)                  { invert_out = i; }
    void setNumInputBytes(byte numInputBytes)   { nIB = numInputBytes; }
    byte getNumInputBytes(void)                 { return nIB; }
    void setNumOutputBytes(byte numOutputBytes) { nOB = numOutputBytes; }
    byte getNumOutputBytes(void)                { return nOB; }
    unsigned long getTXDelay(void)              { return DL; }
    void proceess(void);

private:

    void callback_pack_Node_Inputs(void);
    void callback_unpack_Node_Outputs(void) ;
    void callback_process_cpNode_Options(void);
    void callback_initialize_cpNode(void);
    void callback_flush_CMRInet_to_ETX(void) ;
    void callback_CMRI_Poll_Response(void);
    char callback_read_CMRI_Byte();
    int getPacket(void);


private:
    Stream *cmriNet;          // protocol...
    Stream *Monitor;          // debugging (optional, if not NULL...)
    char debug_buffer[128];

    int  invert_in;           // for inputs:  CMRI_ACTIVE_LOW or CMRI_ACTIVE_HIGH
    int  invert_out;          // for outputs: CMRI_ACTIVE_LOW or CMRI_ACTIVE_HIGH

    byte UA;                  // Node address, stored as UA + UA_Offset
    unsigned long DL;         // Transmit character delay value in 10 us increments

    byte nIB;                 //  Total configured onboard input bytes
    byte nOB;                 //  Total configured onboard output bytes

    byte CMRInet_Buf[CMRInet_BufSize];   // CMRI message buffer
    byte OB[IO_bufsize];                 // Output bits  HOST to NODE
    byte IB[IO_bufsize];                 // Input bits   NODE to HOST
};



class IOX {
private:
    //**********************************************************************
    //**********      I2C I/O Expander Support for MCP28017       **********
    //**********************************************************************

    // MCP28017 Register Map
    const static int MCP28017_IO              = 0x00;  // Port A, Port B = 0x01
    const static int MCP28017_ACTIVELOW       = 0x02;  // Port A, Port B = 0x03
    const static int MCP28017_PULLUP          = 0x0C;  // Port A, Port B = 0x0D
    const static int MCP28017_GPIO            = 0x12;  // Port A, Port B = 0x13

    // ... Per-Port config register initialization values
    const static int MCP28017_PORT_OUTPUT     = 0x00;
    const static int MCP28017_PORT_INPUT      = 0xFF;
    const static int MCP28017_PORT_PULLUPS    = 0xFF;
    const static int MCP28017_PORT_ACTIVELOW  = 0xFF;

public:
    const static bool OUT = 0;
    const static bool IN  = 1;

    const static int PORT_A  = 0;
    const static int PORT_B  = 1;

    static void init( int i2cAddress, byte port, bool isInput);
    static void write(int i2CAddress, byte port, byte data);
    static int  read( int i2CAddress, byte port);
};
