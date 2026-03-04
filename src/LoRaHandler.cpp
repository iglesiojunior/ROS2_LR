#include "LoRaHandler.h"

LoRaHandler::LoRaHandler(int ss, int rst, int dio0, uint8_t myId) {
    _ss = ss; _rst = rst; _dio0 = dio0;
    _myId = myId;
    _onPacketReceived = nullptr;
    _taskHandle = NULL;
}

bool LoRaHandler::begin(long frequency) {
    LoRa.setPins(_ss, _rst, _dio0);
    if (!LoRa.begin(frequency)) return false;

    // Cria a Task fixada no Core 0 (Deixa o Core 1 para o Loop principal/ROS)
    xTaskCreatePinnedToCore(
        startTaskImpl, 
        "LoRaTask", 
        4096, 
        this, 
        1, 
        &_taskHandle, 
        0 
    );
    return true;
}

void LoRaHandler::startTaskImpl(void* _this) {
    LoRaHandler* handler = (LoRaHandler*)_this;
    handler->taskLoop();
}

void LoRaHandler::taskLoop() {
    for (;;) {
        int packetSize = LoRa.parsePacket();
        if (packetSize == sizeof(LoRaPacket)) {
            LoRaPacket packet;
            LoRa.readBytes((uint8_t*)&packet, sizeof(LoRaPacket));

            // Filtra pelo ID: Aceita se for pra mim ou se for Broadcast (255)
            if (packet.target_id == _myId || packet.target_id == 255) {
                if (_onPacketReceived != nullptr) {
                    _onPacketReceived(packet);
                }
            }
        } else if (packetSize > 0) {
            // Limpa o buffer se chegar lixo
            while(LoRa.available()) LoRa.read();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}

void LoRaHandler::sendPacket(uint8_t targetId, uint8_t type, void* payloadData, size_t payloadSize) {
    LoRaPacket packet;
    packet.target_id = targetId;
    packet.sender_id = _myId;
    packet.msg_type = type;
    
    // Zera a memória e copia os dados
    memset(&packet.data, 0, sizeof(packet.data));
    memcpy(&packet.data, payloadData, payloadSize);
    
    packet.checksum = 0; // Pode implementar lógica real de CRC aqui depois

    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(LoRaPacket));
    LoRa.endPacket();
    
    // Volta a ouvir
    LoRa.receive(); 
}

void LoRaHandler::setCallback(PacketCallback callback) {
    _onPacketReceived = callback;
}