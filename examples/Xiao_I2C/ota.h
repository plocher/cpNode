#pragma once

#include <Arduino.h>
#include <IPAddress.h>

// =====================================================================
//  OtaManager - non-blocking WiFi + ArduinoOTA lifecycle
//
//  Owns all network semantics; knows nothing about displays.
//  Wire the optional hooks (null-safe) to route OTA events to a UI.
//
//    OFF -> CONNECTING -> READY <-> CONNECTING   (wifi drops/reconnects)
//                         READY -> UPDATING -> reboot | error -> READY
//
//  begin() never blocks: CMRI processing starts immediately even when
//  WiFi is unavailable; OTA arms itself when the connection comes up.
// =====================================================================

class OtaManager {
public:
    enum State : uint8_t { OFF = 0, CONNECTING, READY, UPDATING };

    // Hook signatures (capture-less lambdas or free functions)
    typedef void (*StartHook)(void);
    typedef void (*ProgressHook)(unsigned int received, unsigned int total);
    typedef void (*EndHook)(void);
    typedef void (*ErrorHook)(const char* name);

    // Optional UI hooks -- assign before begin(); leave null for headless
    StartHook    onStart    = nullptr;
    ProgressHook onProgress = nullptr;
    EndHook      onEnd      = nullptr;
    ErrorHook    onError    = nullptr;

    // Start WiFi (non-blocking) and arm OTA when the connection comes up
    void begin(const char* hostname, const char* ssid, const char* password);

    // Call every loop(): advances connection state, services OTA requests.
    // Blocks only while an actual update transfer is in progress.
    void poll(void);

    State     state(void) const { return _state; }
    IPAddress ip(void) const;

private:
    void arm(void);   // register ArduinoOTA callbacks + begin, once connected

    State       _state    = OFF;
    const char* _hostname = nullptr;
    bool        _armed    = false;
};
