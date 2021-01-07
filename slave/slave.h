#ifndef LORA_SLAVE_H
#define LORA_SLAVE_H

#include <EEPROM.h>
#include <lora/lora.h>
#include <lora/Packet.h>
#include <lora/Callbacks.h>

#define MASTER_ID 1
#define PUBLIC_ID 2

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

    String devInfo(){
        return "SLAVE     ID: " + String(lora.local_id);
    }

    // General Methods
    void beginDynamic(uint8_t dev_type);
    void beginStatic(uint8_t id, uint8_t dev_type);
    void runPerSec();
    void run(packet_callback_t callback);

    uint8_t state();

    Callbacks callbacks;
    void onCallback(uint8_t id, callback_t callback);

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
    
    #if defined( ESP_H ) || defined( _HELTEC_H_ )
    if (EEPROM.read(eepos_id) != _id)
    {
        EEPROM.write(eepos_id, _id);
        EEPROM.commit();
    }
    #else
    EEPROM.update(eepos_id, _id);
    #endif
}

// @ Public

// Inicializa el Slave con ID dinamica
void Slave::beginDynamic(uint8_t dev_type = 0)
{
    devType = dev_type;
    lora.init(_read_id());
}

// Inicializa el Slave con ID estatica
void Slave::beginStatic(uint8_t id, uint8_t dev_type = 0)
{
    devType = dev_type;
    lora.init(id);
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

void Slave::run(packet_callback_t callback = NULL)
{
    _state = 255;
    if (lora.available() == 0)
    {
        Packet p = lora.read();
        switch (p.type)
        {
        case type.net.unregister:
            {
                unregister();
            }
            break;

        case type.control.callback:
            {
                callbacks.call(p.data[0]);
            }

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
            {
                if (callback != NULL)
                {
                    (*callback)(p);
                }                
            }
            break;
        }
    }
    ctrl.run();
}

/*  Devuelve el estado del Slave (Para control). Debe ir despues del .run(); 
    255 = idle */
uint8_t Slave::state()
{
    return _state;
}

/*  Setea un callback en la posicion que se ejecutara cuando llegue un paquete
    que indique que este callback debe ejecutarse
 */

void Slave::onCallback(uint8_t id, callback_t callback){
	callbacks.setCallback(id, callback);
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
    
    Packet request;
    request.set(1, type.net.id_req, devType);

    if(lora.trySend(request, 3, 200, type.net.id_offer, type.net.network_full))
    {
        // Recepcion: DATA = {[ID],[devType]}
        if (lora.read().data[1] == devType)
        {
            Packet confirm;
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