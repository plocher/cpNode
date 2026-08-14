// *****************************************************************
// CMRI/Net example Sketch: Xiao I2C-only cpNode
// Onboard I/O:  None
// IOX I/O:      Up to 8x IOX-16s (MCP23017 I2C expanders)
// Display:      SSD1306 OLED status panel (optional via USE_OLED)
// OTA:          WiFi firmware updates     (optional via USE_OTA)
// *****************************************************************
//
//  Reserved I/O pins are:
//   D7 - RX  CMRI RS485 Receive
//   D6 - TX  CMRI RS485 Transmit
//   D4 - SDA I2C Data
//   D5 - SCL I2C Clock
//   D3 - TXEN RS422/485 Transmit Enable
//
// =================================================================
// ==  WHAT THIS NODE DOES                                        ==
// =================================================================
//
// This sketch turns the Xiao into a C/MRI (Computer/Model Railroad
// Interface) I/O node. JMRI, running on a computer, is the master on
// an RS485 party line; this node is one of its slaves. Many times a
// second:
//
//   - JMRI asks "what do your inputs see?" and the node replies with
//     the current state of its sensors (block detectors, pushbuttons,
//     turnout feedback, ...).
//   - JMRI says "set your outputs" and the node updates its output
//     pins (turnouts, signals, panel lamps, ...).
//
// All I/O lives on IOX-16 expander boards. Each board has two 8-bit
// ports (A and B); each port is either all inputs or all outputs.
// Edit the expanders[] table below to describe your hardware -- the
// sketch and the display adapt automatically.
//
// =================================================================
// ==  INPUTS ARE ACTIVE-LOW (a feature, not a bug)               ==
// =================================================================
//
// Input ports follow the long-standing C/MRI wiring convention:
//
//   - Every input pin has a built-in pull-up resistor. Left alone,
//     the pin idles at a high voltage and JMRI sees 0 (inactive).
//   - A sensor "activates" an input by connecting the pin to ground.
//     A grounded pin is reported to JMRI as 1 (active).
//
// The inversion happens inside the MCP23017 chip itself (its IPOL
// polarity register, configured by the IOX library), so there is
// nothing extra to wire or code: connect each sensor between its
// input pin and ground, and you are done.
//
// Bench-testing tip: if you jumper an output pin straight to an
// input pin, the display shows them as OPPOSITES. An output of 1
// drives the wire high, and a high wire is an idle (0) input.
// Output 0 grounds the wire, which reads as input 1. That is the
// active-low convention working exactly as intended.
//
// =================================================================
// ==  READING THE OLED DISPLAY                                   ==
// =================================================================
//
//   Top line:     the node's name (NODE_NAME), plus two spinners:
//                 "r" spins while JMRI is polling us for inputs,
//                 "t" spins while JMRI is sending us outputs.
//                 Both frozen means no CMRI traffic is arriving.
//   Middle rows:  one row per expanders[] table entry (E0..E7; the
//                 default table maps these to 0x20..0x27), port A on
//                 the left, port B on the right. 'i' marks an input
//                 port, 'o' an output port; each small square is one
//                 bit -- filled = 1, hollow = 0. A bit that just
//                 changed is briefly boxed to catch your eye. A
//                 dashed line marks an unused port.
//   Bottom line:  network status -- a spinner while WiFi connects,
//                 then the node's IP address once OTA is ready.
//
// During an OTA firmware update the screen switches to a full-width
// progress bar, then shows success (before rebooting) or a failure
// notice (held a few seconds, then back to normal operation).
//
// =================================================================
// ==  TECHNICAL REFERENCE (for the curious)                      ==
// =================================================================
//
// Node addressing: enter the same decimal address (0..63) in JMRI's
// "Configure C/MRI Nodes" pane and in NODE_ID below. On the wire the
// protocol offsets every address by 65 (0x41, ASCII 'A'); JMRI does
// this automatically -- entering 40 in JMRI transmits byte 105 (0x69).
//
// Data model: JMRI keeps an inputArray and an outputArray of 8-bit
// bytes for every node and maps layout objects onto them:
//   byteNumber = ( bitNumber - 1 ) / 8
//   bit        = 1 << (( bitNumber - 1 ) mod 8 )
// Inputs appear in the JMRI Sensor Table as CS objects (CS1011 =
// node 1, bit 11); outputs are CT (Turnout) and CL (Light) objects.
//
// This sketch: when a POLL packet arrives, pack() reads every port
// declared IN and builds the reply bytes; when a TRANSMIT packet
// arrives, unpack() writes the received bytes to every port declared
// OUT. JMRI's CPNODE card type presumes 2 bytes of onboard I/O, so
// the first two bytes in each direction are placeholders here and
// the IOX bytes follow them.

// =============================================
// ====   Feature toggles                   ====
// =============================================
#define USE_OLED   // Comment out to run headless (no SSD1306 OLED display)
#define USE_OTA    // Comment out to disable WiFi + OTA firmware updates

#include <Wire.h>  // for the I/O expander

#include "cpNode.h"
#include "display.h"

#ifdef USE_OTA
#include "ota.h"
#ifndef WIFI_SSID
#include "secrets.h"   // WIFI_SSID / WIFI_PASSWORD (per-site; not committed).
                       // CLI builds may inject both instead via
                       // --build-property "compiler.cpp.extra_flags=-DWIFI_SSID=... -DWIFI_PASSWORD=..."
#endif
#endif  // USE_OTA

#define CMRI_NODE_DESCRIPTION  "XiaoC6"
#ifndef NODE_ID                // overridable from a CLI build (-DNODE_ID=...)
#define NODE_ID                30                     // 0...63 just as in JMRI
#endif
const long CMRINET_SPEED     = 28800;                 // 9600, 19200, 28800 ...

// The node's name: shown on the OLED header and used as the mDNS
// hostname for OTA discovery. Defaults to description-nodeID (e.g.
// "XiaoC6-30"); override with a name of your own meaning here or
// from a CLI build (-DNODE_NAME='"yard-throat"').
#define STRINGIFY_(x)  #x
#define STRINGIFY(x)   STRINGIFY_(x)
#ifndef NODE_NAME
#define NODE_NAME      CMRI_NODE_DESCRIPTION "-" STRINGIFY(NODE_ID)
#endif

// =============================================
// ====   I/O Expander Configuration        ====
// =============================================
// Up to 8 MCP23017 expanders at I2C addresses 0x20-0x27.
// Each expander has two 8-bit ports (A=GPIO 0-7, B=GPIO 8-15).
// Declare each port as IN, OUT, or UNUSED.

enum Direction : uint8_t {
    UNUSED = 0,
    OUT    = 1,
    IN     = 2
};

struct IOX_Config {
    uint8_t   address;   // I2C address (0x20-0x27)
    Direction portA;     // GPIO 0-7  direction
    Direction portB;     // GPIO 8-15 direction
};

/* ** You only need to edit THIS ARRAY to change the IOX configuration ** */
IOX_Config expanders[DISP_ROWS] = {
    { 0x20, IN,     OUT    },
    { 0x21, IN,     OUT    },
    { 0x22, IN,     OUT    },
    { 0x23, OUT,    IN     },
    { 0x24, OUT,    IN     },
    { 0x25, UNUSED, UNUSED },
    { 0x26, UNUSED, UNUSED },
    { 0x27, UNUSED, UNUSED },
};

/* ********************************* */
/* Boilerplate code below this line  */
/* ********************************* */

const int  TXEN_PIN = D3;           // specific to the cpNode-Xiao board.

#ifdef USE_OLED
#include <elapsedMillis.h>
NodeDisplay oled;
elapsedMillis displayTimer;
#define DISPLAY_REFRESH_MS 100   // input bits change slowly; ~10 fps is plenty
#endif  // USE_OLED

#ifdef USE_OTA
OtaManager ota;
#endif  // USE_OTA

cpNode cmri;    // Processing logic for handling CMRINet packets
IOX  iox;       // I2C i/o routines for 23017 IO expander

byte port_state[DISP_ROWS][DISP_COLS];  // cached bits from last pack/unpack

// Computed from expanders[] at startup
// (CPNODE card type has 2 phantom onboard bytes, with all the IOX bytes added to them)
uint8_t input_bytes  = 2;
uint8_t output_bytes = 2;

// =============================================
// ====   Packet counters & Display         ====
// =============================================

unsigned long rxcount = 0L;   // incremented by unpack()
unsigned long txcount = 0L;   // incremented by pack()

void setup(void) {
    Serial.begin(115200);   // cpNode protocol debug channel (cmri Monitor)
    delay(10);

#ifdef USE_OLED
    oled.begin(NODE_NAME);   // degrades to headless on failure
#endif

#ifdef USE_OTA
#ifdef USE_OLED
    // Route OTA lifecycle events to the display (capture-less lambdas)
    ota.onStart    = []() { oled.otaStart(); };
    ota.onProgress = [](unsigned int received, unsigned int total) {
        oled.otaProgress(received, total);
    };
    ota.onEnd      = []() { oled.otaSuccess(); };
    ota.onError    = [](const char* name) { oled.otaError(name); };
#endif
    // Non-blocking: CMRI starts immediately; OTA arms when WiFi comes up
    ota.begin(NODE_NAME, WIFI_SSID, WIFI_PASSWORD);
#endif

    // CMRI Serial device
    Serial1.begin(CMRINET_SPEED, SERIAL_8N2, RX /* 7 */, TX /* 6 */);

    cmri.setCMRIPort(&Serial1, TXEN_PIN);
    // cmri.setDebugPort(&Serial);
    cmri.setNodeAddress(NODE_ID);
    cmri.invertInputs(false);
    cmri.invertOutputs(false);

    // *************************************************
    // *******   Setup  I/O Expander (IOX)    **********
    // *************************************************

    Wire.begin();

    // Derive input/output byte counts and init all used ports
    for (uint8_t e = 0; e < DISP_ROWS; e++) {
        if (expanders[e].portA != UNUSED) {
            iox.init(expanders[e].address, IOX::PORT_A,
                     expanders[e].portA == IN);
            if (expanders[e].portA == IN)  input_bytes++;
            if (expanders[e].portA == OUT) output_bytes++;
        }
        if (expanders[e].portB != UNUSED) {
            iox.init(expanders[e].address, IOX::PORT_B,
                     expanders[e].portB == IN);
            if (expanders[e].portB == IN)  input_bytes++;
            if (expanders[e].portB == OUT) output_bytes++;
        }
    }

    cmri.setNumInputBytes(input_bytes);
    cmri.setNumOutputBytes(output_bytes);
}

// ---------------------------------------------------------------------------
// pack() is called whenever there is a need to read bits from the layout
//        in response to a poll request
//
//  Collects input bits from all I/O expander ports declared as IN,
//  packs them into contiguous IB[] bytes for transmission to JMRI.
// ---------------------------------------------------------------------------

void pack(byte *IB, int len) {
    txcount++;
    IB[0] = 0;  // phantom onboard byte (cpNode card type)
    IB[1] = 0;  // phantom onboard byte
    uint8_t idx = 2;
    for (uint8_t e = 0; e < DISP_ROWS; e++) {
        if (expanders[e].portA == IN) {
            port_state[e][0] = iox.read(expanders[e].address, IOX::PORT_A);
            IB[idx++] = port_state[e][0];
        }
        if (expanders[e].portB == IN) {
            port_state[e][1] = iox.read(expanders[e].address, IOX::PORT_B);
            IB[idx++] = port_state[e][1];
        }
    }
}

// ---------------------------------------------------------------------------
// unpack() is called whenever there is a need to write bits out to the layout
//
//  Takes output bytes from the OB[] buffer and writes them to all I/O
//  expander ports declared as OUT.
// ---------------------------------------------------------------------------

void unpack(byte *OB, int len) {
    rxcount++;
    uint8_t idx = 2;  // skip phantom onboard bytes
    for (uint8_t e = 0; e < DISP_ROWS; e++) {
        if (expanders[e].portA == OUT) {
            port_state[e][0] = OB[idx];
            iox.write(expanders[e].address, IOX::PORT_A, OB[idx++]);
        }
        if (expanders[e].portB == OUT) {
            port_state[e][1] = OB[idx];
            iox.write(expanders[e].address, IOX::PORT_B, OB[idx++]);
        }
    }
}

#if defined(USE_OLED) && defined(USE_OTA)
// Map network semantics to the display's status states
static NodeDisplay::NetState netStateFor(OtaManager::State s) {
    switch (s) {
        case OtaManager::READY:
        case OtaManager::UPDATING:   return NodeDisplay::NET_READY;
        case OtaManager::CONNECTING: return NodeDisplay::NET_CONNECTING;
        default:                     return NodeDisplay::NET_OFF;
    }
}
#endif

void loop(void) {
    cmri.process();   // process any C/MRI packets
#ifdef USE_OTA
    ota.poll();       // service WiFi/OTA (blocks only during a transfer)
#endif
#ifdef USE_OLED
    if (displayTimer >= DISPLAY_REFRESH_MS) {
        displayTimer = 0;
        for (uint8_t e = 0; e < DISP_ROWS; e++) {
            oled.setPort(e,
                (NodeDisplay::Dir)expanders[e].portA, port_state[e][0],
                (NodeDisplay::Dir)expanders[e].portB, port_state[e][1]);
        }
        oled.setTX(txcount);
        oled.setRX(rxcount);
#ifdef USE_OTA
        oled.setNet(netStateFor(ota.state()), ota.ip());
#endif
        oled.show();   // pushes a frame only when something changed
    }
#endif
}
