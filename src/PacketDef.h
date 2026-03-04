#pragma once
#include <Arduino.h>

// 1. TIPOS DE MENSAGEM
enum MsgType {
  // Comandos (Gateway -> Robô)
  MSG_CMD_VEL   = 0xA0,
  MSG_LED_COR   = 0xB0,
  
  // Respostas (Robô -> Gateway)
  MSG_TELEMETRY = 0xC0,
  
  // Espaço Livre para o Usuário (Customizadas)
  MSG_USER_CUSTOM_1 = 0xE0
};

// 2. ESTRUTURAS DE CARGA ÚTIL
struct PayloadCmdVel {
  float linear_x;
  float angular_z;
};

struct PayloadLed {
  int32_t color_code;
  int32_t blink_rate;
};

struct PayloadTelemetry {
  float accel_x;
  float accel_y;
  uint8_t bumper_state; 
};

// 3. O PACOTE PRINCIPAL
typedef struct {
  uint8_t target_id;
  uint8_t sender_id;
  uint8_t msg_type;
  
  union {
    PayloadCmdVel    cmd;
    PayloadLed       led;
    PayloadTelemetry telemetry;
    uint8_t          raw_data[16]; // Coringa para structs customizadas
  } data;

  uint16_t checksum;
} __attribute__((packed)) LoRaPacket;