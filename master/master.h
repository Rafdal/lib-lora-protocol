#ifndef LORA_MASTER_H
#define LORA_MASTER_H

#include <config.h>
#include <lora/lora.h>
#include <master/network.h>

class Master
{
private:

    void _registerOnNetwork(packet_t p); // Gestiona la registracion en la red
    bool reg_open = true; // true si la registracion esta disponible

    uint8_t interval_check_counter=1; // Inicia en 2 porque si (para dar margen)
public:
    Master() {}
    ~Master() {}

    // General Methods
    void begin();
    void runPerSec();
    void run();

    // Button Methods
    void Click();
    void DoubleClick();
    void LongPress();

    // Network Methods
    Network network;

    // Parameters
    int8_t interval_check_time = 5; // -1 = Desactivado. (Segundos) Debe ser mayor a 1
};


void Master::begin()
{
    lora.init(1);
    for (uint8_t i = 0; i < clients.size(); i++)
    {
        /*
         * Inicio a todos en -1
         * Cuando empiece el Check Interval, los que
         * esten registrados se van a poner en 0 o mayor a 0
         */
        network.clientStatus.noResponses[i] = -1;
    }
}

void Master::runPerSec()
{
    lora.runPerSec();

    if (interval_check_time > 1)
    {
        if (interval_check_counter > 0)
            interval_check_counter--;
        else
        {
            network.checkConnections();
            interval_check_counter = interval_check_time;
        }
    }
}

// Ejecutar en LOOP no en RTL
void Master::run()
{
    if (lora.available() == 0)
    {
        packet_t p = lora.read();

        switch (p.type)
        {
        case type.net.id_req:
            {
                if (reg_open)
                {
                    _registerOnNetwork(p);
                }
            }
            break;
        
        default:
            break;
        }
    }
    for (uint8_t i = 0; i < clients.size(); i++)
    {
        // Si existe un cliente
        if(!clients.available(i))
        {
            if (network.clientStatus.state[i] != 0)
            {
                DEBUG("setState")
                packet_t p;
                p.set(i+3, type.data.setState, network.clientStatus.state[i]);
                lora.send(p);
                network.clientStatus.state[i] = 0;
            }
            
        }
    }
    
}

// Nothing
void Master::Click()
{
}

// Nothing
void Master::DoubleClick()
{
}

// Clear client list
void Master::LongPress()
{
    network.clearClientList();
}

// Proceso la solicitud de registro
void Master::_registerOnNetwork(packet_t p)
{
    // p.data = { [devType] }
    if (network.available() == -1)
    {
        lora.send(p.id, type.net.network_full);
    }
    else // Si hay espacio en la red
    {
        uint8_t id_offer = network.available() + 3; // Oferta de ID

        packet_t offer;
        offer.set(p.id, type.net.id_offer, id_offer);
        offer.add(p.data[0]);

        if(lora.trySend(offer, 3, 200, type.net.id_confirm))
        {
            if (lora.read().data[0] == p.data[0]) // devType
            {
                network.registerSlave(id_offer, p.data[0]);
                /* for (uint8_t i = 0; i < 3; i++)
                {
                    ! Sin usar por ahora
                    lora.send(id_offer, type.control.ok);
                } */
            }
        }
    }
}


#endif