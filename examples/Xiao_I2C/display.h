#pragma once

#include <Arduino.h>
#include <IPAddress.h>

// Grid dimensions: max I2C expanders (0x20–0x27) × ports per chip (A, B)
const uint8_t DISP_ROWS = 8;
const uint8_t DISP_COLS = 2;

// =====================================================================
//  NodeDisplay - all SSD1306 rendering for the cpNode
//
//  Pure renderer: no CMRI, IOX, WiFi or OTA semantics.
//  Two screen groups:
//    - live view: expander bit grid + activity spinners + net status
//    - OTA views: progress bar / success / error (full-screen takeover)
//  A screen-ownership latch keeps loop()'s show() from painting over
//  an OTA result; an error screen holds ~5 s, then live view resumes.
//  begin() failure degrades to headless: every call becomes a no-op.
// =====================================================================

class NodeDisplay {
public:
    enum Dir      : uint8_t { UNUSED = 0, OUT = 1, IN = 2 };
    enum NetState : uint8_t { NET_OFF = 0, NET_CONNECTING, NET_READY };

    // Init; returns false and goes headless on failure (never hangs)
    bool begin(const char* name);

    // ---- live view state (dirty-flagged; cheap to call every loop) ----
    void setPort(uint8_t index, Dir dirA, byte dataA, Dir dirB, byte dataB);
    void setTX(unsigned long count);
    void setRX(unsigned long count);
    void setNet(NetState state, IPAddress ip);

    // Render if anything changed and the live view owns the screen
    void show(void);

    // ---- OTA screens (call from OTA hooks; take over the screen) ----
    void otaStart(void);
    void otaProgress(unsigned int received, unsigned int total);
    void otaSuccess(void);
    void otaError(const char* name);

private:
    enum Mode : uint8_t { MODE_LIVE, MODE_OTA, MODE_HOLD };

    void render(void);
    void drawHeader(void);
    void drawGrid(void);
    void drawStatus(void);
    void drawPortCells(uint8_t x, uint8_t y, Dir dir, byte val, byte halo);
    void drawCentered(const char* text, uint8_t y);

    bool          _alive = false;

    Dir           _dirs[DISP_ROWS][DISP_COLS] = {};
    byte          _data[DISP_ROWS][DISP_COLS] = {};
    byte          _delta[DISP_ROWS][DISP_COLS]   = {};  // bits recently changed
    uint8_t       _haloAge[DISP_ROWS][DISP_COLS] = {};  // highlight cycles left
    unsigned long _txcount = 0, _rxcount = 0;
    uint8_t       _txFrame = 0, _rxFrame = 0;   // quantized spinner phases
    NetState      _net = NET_OFF;
    IPAddress     _ip;

    bool          _dirty = true;
    Mode          _mode  = MODE_LIVE;
    unsigned long _holdUntil = 0;
    uint8_t       _lastPct   = 255;   // OTA progress redraw throttle
    uint8_t       _anim      = 0;     // status-line spinner frame

    const char*   _name = "";

    char _spinner[5] = "-\\|/";
};
