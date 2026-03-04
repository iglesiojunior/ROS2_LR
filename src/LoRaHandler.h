#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "PacketDef.h"

typedef void (*PacketCallback)(LoRaPacket& packet);

class LoRaHandler {
private:
    int _ss, _rst, _dio0;
    uint8_t _myId;
    PacketCallback _onPacketReceived;
    TaskHandle_t _taskHandle;
    
    static void startTaskImpl(void* _this);
    void taskLoop();

public:
    LoRaHandler(int ss, int rst, int dio0, uint8_t myId);
    
    bool begin(long frequency);
    void sendPacket(uint8_t targetId, uint8_t type, void* payloadData, size_t payloadSize);
    void setCallback(PacketCallback callback);
};