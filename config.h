#ifndef _LORA_CONFIG_H_
#define _LORA_CONFIG_H_

#define MAX_PACKET_BUFFER 128 // Max packet total size

#define MAX_DATA_BUFFER 48 // Max data buffer size
#define TIMEOUT 1500
#define MAX_ATTEMPS 5

#define CHECK_INTERVAL 0
#define MAX_CHECK_FAILS 4
#define SIMPLE_CHECK_MODE true

#ifndef ESP_H
const uint8_t NSS = 10;
const uint8_t RST = 9;
const uint8_t DI0 = 2;
#else
const uint8_t NSS = 5;
const uint8_t RST = 17;
const uint8_t DI0 = 16;
#endif

#ifndef DEBUG
#define DEBUG(a)
#endif

#ifdef _HELTEC_H_
    #define LORA_FREQ   915E6
    #warning "LoRa at 915 MHz"
    // #define LoRa Heltec.LoRa // No hace falta
#else
    #define LORA_FREQ   433E6
    #warning "LoRa at 433 MHz"
    // #define LoRa LoRa // No hace falta
#endif


#endif