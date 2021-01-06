#ifndef LORA_SLAVE_H
#define LORA_SLAVE_H

#include <EEPROM.h>
#include <lora/lora.h>
#include <lora/Packet.h>

class Slave
{
private:
    const int eepos_id = 0;

    uint8_t devType;

    uint8_t _read_id();
    void _write_id(uint8_t _id);

    uint8_t net_conn_counter;

    uint8_t _state; // Estado del slave (Usado para control)

public:
    Slave(){}
    ~Slave(){}

    // General Methods
    void begin(uint8_t dev_type);
    void runPerSec();
    void run();

    uint8_t state();

    // Network Methods
    void registerOnNetwork();
    void unregister();

    uint8_t net_conn_timeout = 6; // Segundos de inactividad para considerar desconectado
};

// ! Private

uint8_t Slave::_read_id()
{
    return EEPROM.read(eepos_id);
}
// Actualizo ID en LoRa y EEPROM
void Slave::_write_id(uint8_t _id)
{
    lora.local_id = _id;
    EEPROM.update(eepos_id, _id);
}

// @ Public


void Slave::begin(uint8_t dev_type)
{
    devType = dev_type;
    lora.init(_read_id());
}

void Slave::runPerSec()
{
    lora.runPerSec();
    ctrl.blink = lora.local_id != 0;
    if (net_conn_counter > 0)
    {
        net_conn_counter--;
        ctrl.led_fast_blinking = true; // @Connected
    }
    else
        ctrl.led_fast_blinking = false; // !Disconnected
}

void Slave::run()
{
    _state = 255;
    if (lora.available() == 0)
    {
        packet_t p = lora.read();
        switch (p.type)
        {
        case type.net.unregister:
            {
                unregister();
            }
            break;

        case type.control.ping:
            {
                lora.send(p.id, type.control.pong);
                net_conn_counter = net_conn_timeout;
            }
            break;

        case type.data.setState:
            if(p.id == 1 && lora.local_id != 0) // Solo Si esta registrado
            {
                _state = p.data[0];
            }
            break;
        default:
            break;
        }
    }
}

/*  Devuelve el estado del Slave (Para control). Debe ir despues del .run(); 
    255 = idle */
uint8_t Slave::state()
{
    return _state;
}

// Inicia proceso de registracion en red LoRa
void Slave::registerOnNetwork()
{
    if (lora.local_id != 0) // Si ya esta registrado, no se ejecuta
        return;    

    for (uint8_t i = 0; i < 3; i++)
    {
        digitalWrite(ctrl.state_led, HIGH);
        delay(100);
        digitalWrite(ctrl.state_led, LOW);
        delay(100);
    }
    
    packet_t request;
    request.set(1, type.net.id_req, devType);

    if(lora.trySend(request, 3, 200, type.net.id_offer, type.net.network_full))
    {
        // Recepcion: DATA = {[ID],[devType]}
        if (lora.read().data[1] == devType)
        {
            packet_t confirm;
            confirm.set(1, type.net.id_confirm, devType);

            for (uint8_t i = 0; i < 3; i++)
                lora.send(confirm);

            _write_id( lora.read().data[0] );
            
        }
    }
}

// Se elimina de la red
void Slave::unregister()
{
    _write_id(0);
    // & Proximamente implementar credenciales
}

#endif