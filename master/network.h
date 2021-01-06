#ifndef LORA_NETWORK_H
#define LORA_NETWORK_H

// % Datos de la BD EEPROM
#define MAX_CLIENTS 8
#define DB_REG_SIZE 2

#include <EEPROMDatabase.h>
EEPROMDatabase clients(MAX_CLIENTS*DB_REG_SIZE, DB_REG_SIZE, 100, false);

#include <lora/lora.h>

typedef struct clients_network_status
{
	int8_t noResponses[MAX_CLIENTS]; // -1 = No existe cliente. 0 = Conectado. +1 = no responde
    uint8_t state[MAX_CLIENTS]; // 0 = En espera. !>0 = setState
}clients_net_status_t;

class Network
{
private:

    
public:
    Network() {}
    ~Network() {}

    clients_net_status_t clientStatus;

    // uint8_t max_noResponses=5; // ! sin usar

    void setState(uint8_t id, uint8_t _state); // ! Queda agregar cosas


    int available();

    void registerSlave(uint8_t id, uint8_t devType);

    void checkConnections();

    void clearClientList();

    void printClients();

    void runSecond();
};

// Actualizar el state del cliente
void Network::setState(uint8_t id, uint8_t _state = 0x1)
{
    if (id < clients.size())
    {
        clientStatus.state[id-3] = _state;
    }   
}


// mayor o igual a cero si hay espacio
int Network::available() 
{
    return clients.available();
}

void Network::registerSlave(uint8_t id, uint8_t devType)
{
    uint8_t reg[] = {devType, 0};
    clients.set(reg, id-3);
    
}

void Network::checkConnections()
{
    for (uint8_t i = 0; i < clients.size(); i++)
    {
        // Si existe un cliente
        if (!clients.available(i))
        {
            if(lora.trySend(lora.ping(i+3), 2, 80, type.control.pong))
            {
                // @ ONLINE
                // Serial.println("Client "+String(i+3)+" online");
                clientStatus.noResponses[i] = 0;
            }
            else
            {
                // ! OFFLINE
                // Serial.println("Client "+String(i+3)+" timeout. OF " + String(clientStatus.noResponses[i]) + " times");
                clientStatus.noResponses[i]++;
            }
        }
    }
}

// Borra la BD de clientes
void Network::clearClientList()
{
    clients.clear();
    for (uint8_t i = 0; i < MAX_CLIENTS; i++)
    {
        clientStatus.noResponses[i] = -1;
    }
    
}

void Network::printClients()
{
    Serial.println(F("[Client list]"));
    Serial.println(F("ID\tStat\tType\n{"));
    for (uint8_t i = 0; i < clients.size(); i++)
    {
        // if (!clients.available(i))
        // {
            Serial.print(i+3);
            Serial.print(F("\t"));
            Serial.print(clientStatus.noResponses[i]);
            Serial.print(F("\t"));
            Serial.println(clients.get(i)[0]);
        // }
    }
    Serial.println(F("}"));
    
}


#endif