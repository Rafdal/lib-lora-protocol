#ifndef CONTROL_H
#define CONTROL_H

typedef void (*void_event_t)(void);

// #ifndef DEBUG_PRINT
// #define DEBUG_PRINT(string) (Serial.println(string))
// #endif

#ifndef ESP_H
const uint8_t NSS = 10;
const uint8_t RST = 9;
const uint8_t DI0 = 2;
#else
const uint8_t NSS = 5;
const uint8_t RST = 17;
const uint8_t DI0 = 16;
#endif


class Control
{
private:
    void_event_t realTimeLoop, loop_1s, loop_100ms;
    unsigned long lastMillis;
    uint8_t lpsCounter;
    bool started = false;
    // uint8_t inactiveTime, timeout;
	bool state;

public:
    #ifndef ESP_H
    uint8_t state_led = 3;
    uint8_t trx_led = 8;
    #else
    uint8_t trx_led = 12;
    uint8_t state_led = 14;
    #endif
    unsigned int timeout = 1500;
    
    bool led_fast_blinking = false; // Parpadear 0.1s en vez de 1s
    bool blink = true; // Parpadeo activado

    void pFE_check();

    void begin();
    void start_lora();
    void setRealTimeLoop(void_event_t rtl);
    void setLoopPerSec(void_event_t lxS);
    void setLoop_100ms(void_event_t l100ms);
    void run();

    void delay(unsigned int t);

    void wait();
}ctrl;


// Chequea si quedo choteado el LoRa
void Control :: pFE_check()
{
    long pFE = LoRa.packetFrequencyError();
	if (pFE > 2800l && pFE < 2900l)
	{
		Serial.println(F("[Error] packetFrequencyError")); 
		// LoRa.setPins(NSS, RST, DI0);
		begin();
	}
}

// INICIA LoRa
void Control :: begin()
{
    pinMode(state_led, OUTPUT);

    LoRa.setPins(NSS, RST, DI0);
	if (!LoRa.begin(433E6))
	{
		Serial.println(F("[Error] Init failed. Check your connections."));
		// asm("jmp 0x0000");
        started = false;
	}
    else
    {
        started = true;
        // Serial.println(F("Lora Started")); 
    }
}

// Chequea si esta iniciado, en caso que no, lo inicia
void Control :: start_lora()
{
    if (!started)
	{
		begin();
	}
}

// Set real time loop (NO PONER NADA DE LORA ADENTRO)
void Control :: setRealTimeLoop(void_event_t rtl)
{
    realTimeLoop = rtl;
}
// Set loop * seg
void Control :: setLoopPerSec(void_event_t lxS)
{
    loop_1s = lxS;
}
// Set loop * 0.1s
void Control :: setLoop_100ms(void_event_t l100ms)
{
    loop_100ms = l100ms;
}

// COLOCAR EN CUALQUIER BUCLE o LOOP()
void Control :: run()
{
    if (realTimeLoop != NULL)
        (*realTimeLoop)();

    if (millis() - lastMillis >= 100)
    {
        if (state && led_fast_blinking)
            digitalWrite(state_led, LOW);

        if (loop_100ms != NULL)
            (*loop_100ms)();

        lpsCounter++;
        
        if (lpsCounter == 10)
        {
            if (loop_1s != NULL)
                (*loop_1s)();

            state = !state;
            digitalWrite(state_led, state && blink);

            lpsCounter=0;
        }
        lastMillis = millis();
    }
    
    pFE_check();
}

// delay local (No afecta a RTL)
void Control :: delay(unsigned int t)
{
    unsigned long stamp = millis();
    while (stamp + (unsigned long)t >= millis())
    {
        run();
    }
}

// Esperar a recibir un mensaje
void Control :: wait()
{
    unsigned long lastSend = millis();
    if (LoRa.beginPacket() == 0)
    {
        while (LoRa.beginPacket() == 0)
        {
            if((LoRa.packetFrequencyError() > 2800 && LoRa.packetFrequencyError() < 2900) || (millis()-lastSend) >= timeout*2 )
            {
                delay(50);
                #ifndef ESP_H
                asm("jmp 0x0000");
                #else
                ESP.restart();
                #endif
            }
            this->delay(1);
        }
        this->delay(1); // delay fijo para dar margen
    }
}
#endif