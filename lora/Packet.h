#ifndef PACKET_H
#define PACKET_H

#define BUFFER_SIZE 20

#define ASCII 255

class _ptype
{
public:
	class _control
	{
	public:
		enum{ // Control packets
			key_req = 1,
			key_answ,
			key_confirm,
			ping,
			pong,
			ok,
			error,

			data_request
		};
	}control;	

	class _net
	{
	public:
		enum{ // Network packets IMPLEMENTAR ID DE RED para evitar colisiones entre redes
			id_req = 32, // ! Solicitud de registro en la red. DATA = [devType]
			id_offer, // @ Ofrece un ID. DATA = [ID, devType]
			network_full, // @ Avisa que no queda espacio en la red
			id_confirm, // ! Confirma el ID asignado. DATA = [devType]
			unregister, // @ Indica al SLAVE que se borre su ID
		};
	}net;

	class _requests
	{
	public:
		enum{ // 
			get = 100,
			post,
			
		};
	}request;

	class _data
	{
	public:
		enum{ // 
			setState = 128, // Actualiza el estado del Slave. DATA = [state, ...]
		};
	}data;
}type;


class packet_t
{
private:
	void _error_();
	
public:
	// uint8_t from;
	uint8_t id;
	uint8_t type;
	uint8_t size;
	uint8_t data[BUFFER_SIZE];
	packet_t();
	~packet_t(){}

	void crypt(char Pass[], uint8_t Pass_Size, uint8_t Key);
	void crypt(String Pass, uint8_t Key);

	void set(uint8_t _id, uint8_t _type, uint8_t singleData); // Setea ID destino y Type
	void add(uint8_t _byte); // Agrega un byte

	void parse(String str);
	void parse(uint8_t _data[], uint8_t size);
	#ifdef ARDUINO_OBJECT
	void parse(ArduinoObject obj);
	ArduinoObject toArduinoObject();
	#endif

	void print(unsigned char mode);
	String toStr();
};

packet_t::packet_t()
{
	size = 0;
	type = 0;
	id = 255;
}

// Mensaje de parse error
void packet_t :: _error_()
{
	Serial.println(F("[Error] Parse fail "));
}

void packet_t::crypt(char Pass[], uint8_t Pass_Size, uint8_t Key = 0)
{
	Serial.print(F("pass_size: ")); 
	Serial.println(Pass_Size);
    Key ^= size;
    for (uint8_t i = 0; i < Pass_Size; i++)
        Key ^= Pass[i] ^ i;
    for (uint8_t i = 0; i < size; i++)
    {
        uint8_t Changing_Byte =  (uint8_t)Pass[i%Pass_Size] ^ i/Pass_Size;
        uint8_t Swift_Byte = ((uint8_t)Pass[i%Pass_Size] ^ (i+1)*Pass_Size) ^ ((Changing_Byte*Key) + i);
        data[i] = data[i] ^ Swift_Byte;
    }
}

void packet_t::crypt(String Pass, uint8_t Key = 0)
{
	uint8_t size = Pass.length() + 1;
	char c_pass[size];
	Pass.toCharArray(c_pass,size);
	crypt(c_pass, size , Key);
}

// Setea ( ID , Type , Data )
void packet_t::set(uint8_t _id, uint8_t _type, uint8_t singleData = 0)
{
	id = _id;
	type = _type;
	if (singleData != 0)
	{
		data[0] = singleData;
		size = 1;
	}
}

// Agrega un byte al paquete de datos
void packet_t::add(uint8_t _byte) 
{
	data[size]= _byte;
	size++;
}

void packet_t::parse(String str)
{
	if (str.length() <= BUFFER_SIZE)
	{
		size = str.length();
		for (uint8_t i = 0; i < size; i++)
			data[i] = (uint8_t)((char)str[i]);
	}
	else
		_error_();
}

// Meter array de bytes en el Data del paquete
void packet_t::parse(uint8_t _data[], uint8_t _size)
{
	if (size <= BUFFER_SIZE)
	{
		size = _size;
		for (uint8_t i = 0; i < size; i++)
			data[i] = _data[i];
	}
	else
		_error_();
}

#ifdef ARDUINO_OBJECT
void packet_t::parse(ArduinoObject obj)
{
	if (obj.length() <= BUFFER_SIZE)
	{
		size = obj.length();
		for (uint8_t i = 0; i < size; i++)
		{
			data[i] = obj.data(i);
		}
	}
	else
	{
		_error_();
	}
}

ArduinoObject packet_t::toArduinoObject()
{
	ArduinoObject out(size, data);
	return out;
}

#endif

// Mostrar informacion del paquete
void packet_t::print(unsigned char mode = DEC)
{
	char buf[30];
	sprintf(buf, "{id:%u, type:%u, size:%u}", id, type, size);
	Serial.print(buf); 
	Serial.print(F(" = [")); 
	for (uint8_t i = 0; i < size; i++)
	{
		if (mode == ASCII)
			Serial.print((char)data[i]); 
		else
		{
			Serial.print(String(data[i], mode));
			Serial.print(',');
		}
	}
	Serial.println(']');
}

// return Data as String object 
String packet_t::toStr()
{
	String out;
	out.reserve(16);
	for (uint8_t i = 0; i < size; i++)
	{
		out += (char)data[i];
	}
	return out;
}

#endif