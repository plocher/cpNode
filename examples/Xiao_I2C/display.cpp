#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C   // 0x3C for 128x32, 0x3D for 128x64

static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

// ---- live view layout metrics (128x64, 6x8 base font) ----
static const uint8_t FONT_W     = 6;    // base font advance
static const uint8_t HEADER_Y   = 0;    // node identity + activity spinners
static const uint8_t GRID_TOP   = 10;   // first expander row
static const uint8_t ROW_PITCH  = 5;    // vertical px per expander row
static const uint8_t CELL_SIZE  = 4;    // bit cell width/height
static const uint8_t CELL_PITCH = 6;    // horizontal px per bit cell
static const uint8_t PORT_LEFT_X  = 0;    // left port group glyph  (PCB: Port B)
static const uint8_t PORT_RIGHT_X = 60;   // right port group glyph (PCB: Port A)
static const uint8_t STATUS_Y   = 56;   // network status line

static const unsigned long OTA_ERROR_HOLD_MS = 5000;

// How many refresh cycles a changed bit stays highlighted (~100 ms each)
static const uint8_t CHANGE_HALO_CYCLES = 3;

// -----------------------------------------------------------------------
//  Live view drawing
// -----------------------------------------------------------------------

void NodeDisplay::drawHeader(void) {
    oled.setCursor(0, HEADER_Y);
    oled.print(_name);

    // Quantized spinners: at most one step per refresh (see setTX/setRX),
    // so rotation is smooth regardless of how fast packets arrive.
    oled.setCursor(96, HEADER_Y);
    oled.printf("r%c", _spinner[_txFrame % (sizeof(_spinner) - 1)]);
    oled.setCursor(114, HEADER_Y);
    oled.printf("t%c", _spinner[_rxFrame % (sizeof(_spinner) - 1)]);
}

// Tiny 3x4 'i' / 'o' direction glyphs that fit the 5 px row pitch
static void glyphI(uint8_t x, uint8_t y) {
    oled.drawPixel(x + 1, y, WHITE);              // dot
    oled.drawFastVLine(x + 1, y + 2, 2, WHITE);   // stem
}

static void glyphO(uint8_t x, uint8_t y) {
    oled.drawPixel(x + 1, y,     WHITE);          // tiny ring
    oled.drawPixel(x,     y + 1, WHITE);
    oled.drawPixel(x + 2, y + 1, WHITE);
    oled.drawPixel(x,     y + 2, WHITE);
    oled.drawPixel(x + 2, y + 2, WHITE);
    oled.drawPixel(x + 1, y + 3, WHITE);
}

// One port group: direction glyph + 8 bit cells (filled=1, hollow=0).
// Bits in `halo` get a box drawn around them (recent-change highlight).
// UNUSED ports render as a dashed placeholder through the cell band.
void NodeDisplay::drawPortCells(uint8_t x, uint8_t y, Dir dir, byte val, byte halo) {
    const uint8_t cellsX = x + 2 * CELL_SIZE;                       // glyph, then gap
    const uint8_t bandW  = 8 * CELL_PITCH - (CELL_PITCH - CELL_SIZE);

    if (dir == UNUSED) {
        for (uint8_t dx = 0; dx < bandW; dx += 4) {
            oled.drawFastHLine(cellsX + dx, y + CELL_SIZE / 2, 2, WHITE);
        }
        return;
    }

    // direction: 'i' = input (sensors), 'o' = output (turnouts/lights)
    if (dir == IN) glyphI(x, y);
    else           glyphO(x, y);

    for (uint8_t pos = 0; pos < 8; pos++) {   // PCB order: bit 7 leftmost .. bit 0 rightmost
        uint8_t b  = 7 - pos;                  // visual position -> bit index
        uint8_t cx = cellsX + pos * CELL_PITCH;
        if ((val >> b) & 1) oled.fillRect(cx, y, CELL_SIZE, CELL_SIZE, WHITE);
        else                oled.drawRect(cx, y, CELL_SIZE, CELL_SIZE, WHITE);
        if ((halo >> b) & 1) {          // recent change: ring the cell
            oled.drawRect(cx - 1, y - 1, CELL_SIZE + 2, CELL_SIZE + 2, WHITE);
        }
    }
}

void NodeDisplay::drawGrid(void) {
    // PCB/wiring order: Port B on the left, Port A on the right,
    // with bits shown 7..0 left-to-right within each port.
    for (uint8_t e = 0; e < DISP_ROWS; e++) {
        uint8_t y = GRID_TOP + e * ROW_PITCH;
        drawPortCells(PORT_LEFT_X,  y, _dirs[e][1], _data[e][1], _delta[e][1]);  // Port B
        drawPortCells(PORT_RIGHT_X, y, _dirs[e][0], _data[e][0], _delta[e][0]);  // Port A
    }
}

void NodeDisplay::drawStatus(void) {
    oled.setCursor(0, STATUS_Y);
    switch (_net) {
        case NET_OFF:      // network disabled: leave the line blank
            break;
        case NET_CONNECTING:
            oled.printf("WiFi %c", _spinner[_anim++ % (sizeof(_spinner) - 1)]);
            break;
        case NET_READY:
            // longest IPv4 is 15 chars; "xxx.xxx.xxx.xxx OTA" = 19 fits 21-char line
            oled.printf("%s OTA", _ip.toString().c_str());
            break;
    }
}

// Compose the full live view into the framebuffer, then push one frame.
// (Single display() per frame = no visible flashing.)
void NodeDisplay::render(void) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    drawHeader();
    drawGrid();
    drawStatus();
    oled.display();
}

// -----------------------------------------------------------------------
//  Public interface: live view
// -----------------------------------------------------------------------

bool NodeDisplay::begin(const char* name) {
    _name = name;

    // Degrade to headless on failure -- never hang the node over a display
    _alive = oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    if (!_alive) return false;

    oled.clearDisplay();
    oled.dim(true);
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    drawHeader();
    oled.display();
    _dirty = true;
    return true;
}

void NodeDisplay::setPort(uint8_t index, Dir dirA, byte dataA, Dir dirB, byte dataB) {
    if (!_alive || index >= DISP_ROWS) return;

    byte diffA = dataA ^ _data[index][0];
    byte diffB = dataB ^ _data[index][1];

    if (_dirs[index][0] != dirA || _dirs[index][1] != dirB || diffA || diffB) {
        _dirs[index][0] = dirA;  _data[index][0] = dataA;
        _dirs[index][1] = dirB;  _data[index][1] = dataB;

        // Highlight just-changed bits for the next few refresh cycles
        if (diffA) { _delta[index][0] |= diffA; _haloAge[index][0] = CHANGE_HALO_CYCLES; }
        if (diffB) { _delta[index][1] |= diffB; _haloAge[index][1] = CHANGE_HALO_CYCLES; }

        _dirty = true;
    }
}

void NodeDisplay::setTX(unsigned long count) {
    if (!_alive || count == _txcount) return;
    _txcount = count;
    _txFrame++;        // quantized: one spinner step per refresh with traffic
    _dirty   = true;
}

void NodeDisplay::setRX(unsigned long count) {
    if (!_alive || count == _rxcount) return;
    _rxcount = count;
    _rxFrame++;        // quantized: one spinner step per refresh with traffic
    _dirty   = true;
}

void NodeDisplay::setNet(NetState state, IPAddress ip) {
    if (!_alive) return;
    if (state != _net || !(ip == _ip)) {
        _net   = state;
        _ip    = ip;
        _dirty = true;
    }
    if (state == NET_CONNECTING) _dirty = true;   // keep the spinner animating
}

void NodeDisplay::show(void) {
    if (!_alive) return;

    if (_mode == MODE_OTA) return;            // OTA owns the screen
    if (_mode == MODE_HOLD) {
        if (millis() < _holdUntil) return;    // error screen still holding
        _mode  = MODE_LIVE;                   // release back to the live view
        _dirty = true;
    }

    // Age the change highlights; keep rendering while any are visible
    // so expired halos get erased.
    for (uint8_t e = 0; e < DISP_ROWS; e++) {
        for (uint8_t p = 0; p < DISP_COLS; p++) {
            if (_haloAge[e][p]) {
                if (--_haloAge[e][p] == 0) _delta[e][p] = 0;
                _dirty = true;
            }
        }
    }

    if (!_dirty) return;                      // nothing changed: no push

    render();
    _dirty = false;
}

// -----------------------------------------------------------------------
//  Public interface: OTA screens
// -----------------------------------------------------------------------

void NodeDisplay::drawCentered(const char* text, uint8_t y) {
    int16_t x = (SCREEN_WIDTH - (int16_t)strlen(text) * FONT_W) / 2;
    if (x < 0) x = 0;
    oled.setCursor(x, y);
    oled.print(text);
}

void NodeDisplay::otaStart(void) {
    if (!_alive) return;
    _mode    = MODE_OTA;
    _lastPct = 255;
    otaProgress(0, 1);   // draw the empty bar immediately
}

void NodeDisplay::otaProgress(unsigned int received, unsigned int total) {
    if (!_alive) return;
    _mode = MODE_OTA;

    uint8_t pct = (total > 0) ? (uint8_t)((uint64_t)received * 100 / total) : 0;
    if (pct == _lastPct) return;   // repaint only when the percent changes
    _lastPct = pct;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);

    drawCentered("FIRMWARE UPDATE", 4);

    // bordered progress bar, 1 px per percent inside
    const uint8_t barX = 13, barY = 22, barW = 102, barH = 12;
    oled.drawRect(barX, barY, barW, barH, WHITE);
    if (pct > 0) oled.fillRect(barX + 1, barY + 1, pct, barH - 2, WHITE);

    char buf[24];
    snprintf(buf, sizeof(buf), "%u%%", pct);
    drawCentered(buf, 40);
    snprintf(buf, sizeof(buf), "%u / %u KB", received / 1024, total / 1024);
    drawCentered(buf, 52);

    oled.display();
}

void NodeDisplay::otaSuccess(void) {
    if (!_alive) return;
    _mode = MODE_OTA;   // reboot follows; keep ownership

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);

    // circle + check mark
    oled.drawCircle(64, 20, 12, WHITE);
    oled.drawLine(58, 20, 62, 25, WHITE);
    oled.drawLine(62, 25, 70, 15, WHITE);

    drawCentered("UPDATE OK", 40);
    drawCentered("rebooting...", 52);
    oled.display();
}

void NodeDisplay::otaError(const char* name) {
    if (!_alive) return;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);

    // circle + cross
    oled.drawCircle(64, 20, 12, WHITE);
    oled.drawLine(58, 14, 70, 26, WHITE);
    oled.drawLine(70, 14, 58, 26, WHITE);

    drawCentered("UPDATE FAILED", 38);
    drawCentered(name, 48);
    oled.display();

    // Hold the error on screen briefly; show() then resumes the live view
    _mode      = MODE_HOLD;
    _holdUntil = millis() + OTA_ERROR_HOLD_MS;
}
