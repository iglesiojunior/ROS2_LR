#include "LoRaHandler.h"

LoRaHandler::LoRaHandler(int ss, int rst, int dio0, uint8_t myId) {
    _ss = ss; _rst = rst; _dio0 = dio0;
    _myId = myId;
    _onPacketReceived = nullptr;
    _taskHandle = NULL;
    
    // Cria o Mutex de segurança no construtor
    _spiMutex = xSemaphoreCreateMutex(); 
}

bool LoRaHandler::begin(long frequency) {
    LoRa.setPins(_ss, _rst, _dio0);
    if (!LoRa.begin(frequency)) return false;

    // Cria a Task de receção fixada no Core 0
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
        bool hasPacket = false;
        LoRaPacket packet;

        // Pede autorização (Mutex) para usar o barramento SPI
        if (xSemaphoreTake(_spiMutex, portMAX_DELAY)) {
            int packetSize = LoRa.parsePacket();
            
            if (packetSize == sizeof(LoRaPacket)) {
                LoRa.readBytes((uint8_t*)&packet, sizeof(LoRaPacket));
                hasPacket = true;
            } else if (packetSize > 0) {
                // Limpa o buffer se chegar lixo/ruído
                while(LoRa.available()) LoRa.read(); 
            }
            
            // Devolve o acesso ao SPI imediatamente
            xSemaphoreGive(_spiMutex); 
        }

        // Se apanhou um pacote válido, processa-o fora da área bloqueada pelo Mutex
        if (hasPacket) {
            if (packet.target_id == _myId || packet.target_id == 255) {
                if (_onPacketReceived != nullptr) {
                    _onPacketReceived(packet);
                }
            }
        }
        
        // Dá um respiro para o Watchdog do FreeRTOS
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}

void LoRaHandler::sendPacket(uint8_t targetId, uint8_t type, void* payloadData, size_t payloadSize) {
    LoRaPacket packet;
    packet.target_id = targetId;
    packet.sender_id = _myId;
    packet.msg_type = type;
    
    // Zera a memória e copia a carga útil (Evita lixo na union)
    memset(&packet.data, 0, sizeof(packet.data));
    memcpy(&packet.data, payloadData, payloadSize);
    
    packet.checksum = 0;

    // Pede autorização (Mutex) para usar o SPI e enviar
    if (xSemaphoreTake(_spiMutex, portMAX_DELAY)) {
        LoRa.beginPacket();
        LoRa.write((uint8_t*)&packet, sizeof(LoRaPacket));
        LoRa.endPacket();
        
        // Volta a colocar o rádio em modo de escuta
        LoRa.receive(); 
        
        // Devolve o acesso ao SPI
        xSemaphoreGive(_spiMutex); 
    }
}

void LoRaHandler::setCallback(PacketCallback callback) {
    _onPacketReceived = callback;
}