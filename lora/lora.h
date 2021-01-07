#ifndef LORA_BASICS_H
#define LORA_BASICS_H

#ifndef _HELTEC_H_
#include <SPI.h>
#include <LoRa.h>
#else
#include <lora/LoRa.h>
#endif

#ifdef _HELTEC_H_
#include <crc16.h>
#else
#include <util/crc16.h>
#endif
#include <FastCRC.h>
FastCRC16 CRC16;

#include <config.h>

#include "Control.h"

#include "Packet.h"

// #define DEBUG_PRINT(string) (Serial.println(string))

class Lora
{
private:
	packet_t Pack; // buffer de paquete

	void _check_init_();
public:
	const int buffer_size = BUFFER_SIZE;

	uint8_t local_id = 255;
	
	// Iniciar LoRa con addr local
	void init(uint8_t address, uint8_t _trx_led);
	void runPerSec();

	uint8_t available();
	packet_t read();

	// Crea un paquete ping
	packet_t ping(uint8_t id); 
	
	void send(packet_t p);
	void send(uint8_t id, uint8_t type);
	bool trySend(packet_t p, uint8_t times, uint16_t interval, uint8_t expected_type, uint8_t error_type);
	void echo();
	void echof();

	void printID();
}lora;

// Muestra el ID Local
void Lora::printID()
{
	Serial.print(F("LocalID: "));
	Serial.println(local_id);	
}

void Lora :: _check_init_()
{
	if (local_id == 255)
	{
		Serial.println(F("[Error] Not initialized"));
		delay(3000);
		#ifndef ESP_H
		asm("jmp 0x0000");
		#else
		ESP.restart();
		#endif
	}
	ctrl.start_lora();
}


void Lora :: init(uint8_t address, uint8_t _trx_led = ctrl.trx_led)
{
	local_id = address;
	ctrl.start_lora();
	ctrl.trx_led = _trx_led;
	// Serial.print(F("Local ID:")); 
	// Serial.println(local_id);
	pinMode(ctrl.trx_led, OUTPUT);
}

void Lora :: runPerSec()
{
	
}

// Crea un paquete ping
packet_t Lora :: ping(uint8_t id)
{
	packet_t p;
	p.set(id, type.control.ping);
	return p;
}

// Reenviar un paquete con dato
void Lora :: echo()
{
	send(Pack);
}

// Reenviar un paquete de control
void Lora :: echof()
{
	send(Pack.id, Pack.type);
}

// Envia un paquete con datos
void Lora :: send(packet_t p)
{
	if (local_id == p.id)
	{
		Serial.println(F("[Error] You can't send to yourself"));
		return;
	}
	if (p.id == 255)
	{
		Serial.println(F("[Error] undef pkt id"));
		return;
	}
	digitalWrite(ctrl.trx_led, HIGH);
	#ifndef _HELTEC_H_
	while (LoRa.isTransmitting()){;} // ! TEST
	#else

	#endif
	LoRa.available();
	// ctrl.wait();
	#ifdef LORA_PRINT_DEBUG
	p.print();
	#endif

	_check_init_();
	
	uint8_t packet[4 + p.size];
	packet[0] = p.id;
	packet[1] = local_id;
	packet[2] = p.type;
	packet[3] = p.size;
	for (uint8_t i = 0; i < p.size; i++)
		packet[4 + i] = p.data[i];
	uint16_t crc = CRC16.kermit(packet, 4 + p.size);

	ctrl.wait();

	LoRa.beginPacket();

	LoRa.write((uint8_t)((crc & 0xFF00) >> 8));
	LoRa.write((uint8_t)(crc & 0xFF));
	
	for (uint8_t i = 0; i < p.size+4; i++)
		LoRa.write(packet[i]);

	// #DEBUG ONLY
	/* for (uint8_t i = 0; i < p.size+4; i++)
	{
		Serial.print(packet[i]);
		Serial.print(',');
	} Serial.println(); */

	LoRa.endPacket();
	digitalWrite(ctrl.trx_led, LOW);
}

// Envia un paquete de control
void Lora :: send(uint8_t id, uint8_t type)
{
	digitalWrite(ctrl.trx_led, HIGH);
	
	uint8_t packet[3];
	packet[0] = id;
	packet[1] = local_id;
	packet[2] = type;
	
	uint16_t crc = CRC16.kermit(packet, 3);

	ctrl.wait();

	LoRa.beginPacket();

	LoRa.write((uint8_t)((crc & 0xFF00) >> 8));
	LoRa.write((uint8_t)(crc & 0xFF));
	
	for (uint8_t i = 0; i < 3; i++)
		LoRa.write(packet[i]);

	// #DEBUG ONLY
		/* for (uint8_t i = 0; i < p.size+4; i++)
		{
			Serial.print(packet[i]);
			Serial.print(',');
		} Serial.println(); */

	LoRa.endPacket(true);
	digitalWrite(ctrl.trx_led, LOW);
}

// Envia un paquete que espera una respuesta Y/N
bool Lora::trySend(packet_t p, uint8_t times, uint16_t interval, uint8_t expected_type, uint8_t error_type = type.control.error)
{
	for (uint8_t time = 0; time < times; time++)
	{
		send(p);
		unsigned long stamp = millis();
		while (millis() - stamp < interval)
		{
			// ctrl.run();
			if (available() == 0)
			{
				packet_t response = read();
				if (response.type == expected_type)
				{
					return true;
				}
				if(response.type == error_type)
				{
					return false;
				}
			}
		}
	}
	return false;
}

/*
0 = Nuevo paquete
1 = Nada
2 = Oversized
3 = CRC Error
4 = Not 4 me
5 = Size error */
uint8_t Lora :: available()
{
	_check_init_();
	if (LoRa.parsePacket() != 0)
	{
		if (LoRa.available())
		{
			uint16_t RemoteCRC = (LoRa.read() << 8) | LoRa.read();

			uint8_t size = 0;
			uint8_t packet[buffer_size+4];
			
			for (uint8_t pp = 0; pp < 255; pp++)
			{
				if (LoRa.available())
				{
					packet[size%(buffer_size+3)] = LoRa.read();
					size++;
				}
			}
			uint16_t Local_CRC = CRC16.kermit(packet, size);
			if (Local_CRC == RemoteCRC)
			{
				if(packet[0] == local_id)
				{
					if (size == 3) 
					{
						digitalWrite(ctrl.trx_led, HIGH);
						packet_t p;
						p.id = packet[1];
						p.type = packet[2];
						Pack = p;
						return 0; // OK
					}
					else if(buffer_size+4 >= size)
					{
						if(packet[3]+4 == size)
						{
							digitalWrite(ctrl.trx_led, HIGH);
							packet_t p;
							p.id = packet[1];
							p.type = packet[2];
							p.size = packet[3];

							for (uint8_t i = 0; i < p.size; i++)
								p.data[i] = packet[4+i];
							
							Pack = p;
							return 0; // OK
						}
						return 5; // Size error
					}
					return 2; // Oversized
				}
				return 4; // Not 4 me
			}			
			return 3; // CRC Error
		}
	}
	return 1; // Nothing to read
}

// Devuelve el paquete en buffer
packet_t Lora :: read()
{
	digitalWrite(ctrl.trx_led, LOW);
	packet_t pout = Pack;
	Pack.id = 255;
	Pack.type = 0;
	return pout;
}
/* void DataCrypter(uint8_t Data[], String Pass, uint8_t Key, uint8_t Size, uint8_t *Output)
{
    uint8_t Pass_Size = Pass.length();
    Key ^= Size;
    for (uint8_t i = 0; i < Pass_Size; i++)
        Key ^= Pass[i] ^ i;
    for (uint8_t i = 0; i < Size; i++)
    {
        uint8_t Changing_Byte =  (uint8_t)Pass[i%Pass_Size] ^ i/Pass_Size;
        uint8_t Swift_Byte = ((uint8_t)Pass[i%Pass_Size] ^ (i+1)*Pass_Size) ^ ((Changing_Byte*Key) + i);
        Output[i] = Data[i] ^ Swift_Byte;
    }
} */

#endif