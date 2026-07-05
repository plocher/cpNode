#include "ota.h"

#include <WiFi.h>
#include <ArduinoOTA.h>

// Map ArduinoOTA error codes to short human-readable names
static const char* errorName(ota_error_t err) {
    switch (err) {
        case OTA_AUTH_ERROR:    return "Auth Failed";
        case OTA_BEGIN_ERROR:   return "Begin Failed";
        case OTA_CONNECT_ERROR: return "Connect Failed";
        case OTA_RECEIVE_ERROR: return "Receive Failed";
        case OTA_END_ERROR:     return "End Failed";
        default:                return "Unknown Error";
    }
}

void OtaManager::begin(const char* hostname, const char* ssid, const char* password) {
    _hostname = hostname;

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);   // modem power-save stalls OTA TCP transfers on C6:
                            // the invitation succeeds but connect/data times out.
                            // Keep the radio awake for reliable updates.
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    _state = CONNECTING;   // poll() promotes to READY once connected
}

void OtaManager::arm(void) {
    ArduinoOTA.setHostname(_hostname);

    ArduinoOTA.onStart([this]() {
        _state = UPDATING;
        if (onStart) onStart();
    });
    ArduinoOTA.onProgress([this](unsigned int received, unsigned int total) {
        if (onProgress) onProgress(received, total);
    });
    ArduinoOTA.onEnd([this]() {
        if (onEnd) onEnd();   // device reboots after this returns
    });
    ArduinoOTA.onError([this](ota_error_t err) {
        if (onError) onError(errorName(err));
    });                       // control returns to loop() after an error

    ArduinoOTA.begin();
    _armed = true;
}

void OtaManager::poll(void) {
    if (_state == OFF) return;

    bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected && !_armed) {
        arm();
    }
    _state = connected ? READY : CONNECTING;

    if (_armed && connected) {
        ArduinoOTA.handle();   // blocks here for the whole transfer
    }
}

IPAddress OtaManager::ip(void) const {
    return WiFi.localIP();
}
