#ifndef PACKET_H
#define PACKET_H

#define BUFFER_SIZE 32

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
			data_request,
			callback,
		};
	}control;	

	class _net
	{
	public:
		enum{ // Network packets IMPLEMENTAR ID DE RED para evitar colisiones entre redes
			id_req = 50, // ! Solicitud de registro en la red. DATA = [devType]
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
			get = 64,
			post,
			
		};
	}request;

	class _data
	{
	public:
		enum{ // 
			setState = 100, // Actualiza el estado del Slave. DATA = [state, ...]
			string,
			arduino_object,
		};
	}data;
}type;


class Packet
{
private:
	void _error_();
	
public:
	// uint8_t from;
	uint8_t id;
	uint8_t type;
	uint8_t size;
	uint8_t data[BUFFER_SIZE];
	Packet();
	~Packet(){}

	void crypt(char Pass[], uint8_t Pass_Size, uint8_t Key);
	void crypt(String Pass, uint8_t Key);

	void to(uint8_t _id); // Setea destino
	void set(uint8_t _id, uint8_t _type, uint8_t singleData); // Setea ID destino y Type
	void add(uint8_t _byte); // Agrega un byte

	void callback(uint8_t call_id); // ( dev_id, call_id )

	void string(String str);
	void parse(uint8_t _data[], uint8_t size);
	
	#ifdef ARDUINO_OBJECT
	void parse(ArduinoObject* obj);
	ArduinoObject toArduinoObject();
	#endif

	void print(unsigned char mode);
	String toStr();
};

Packet::Packet()
{
	size = 0;
	type = 0;
	id = 255;
}

// Mensaje de parse error
void Packet :: _error_()
{
	Serial.println(F("[Error] Parse fail "));
}

void Packet::crypt(char Pass[], uint8_t Pass_Size, uint8_t Key = 0)
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

void Packet::crypt(String Pass, uint8_t Key = 0)
{
	uint8_t size = Pass.length() + 1;
	char c_pass[size];
	Pass.toCharArray(c_pass,size);
	crypt(c_pass, size , Key);
}

// Setea destino
void Packet::to(uint8_t _id)
{
	id = _id;
}

// Setea ( ID , Type , Data )
void Packet::set(uint8_t _id, uint8_t _type, uint8_t singleData = 0)
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
void Packet::add(uint8_t _byte) 
{
	data[size]= _byte;
	size++;
}

void Packet::callback(uint8_t call_id){
	id = dev_id;
	type = _ptype::_control::callback;
	size = 1;
	data[0] = call_id;
}

void Packet::string(String str)
{
	if (str.length() <= BUFFER_SIZE)
	{
		size = str.length();
		for (uint8_t i = 0; i < size; i++)
			{data[i] = (uint8_t)((char)str[i]);}

		type = _ptype::_data::string;
	}
	else
		_error_();
}

// Meter array de bytes en el Data del paquete
void Packet::parse(uint8_t _data[], uint8_t _size)
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
void Packet::parse(ArduinoObject *obj)
{
	if (obj->size() <= BUFFER_SIZE)
	{
		type = _ptype::_data::arduino_object;
		size = obj->size();
		for (uint8_t i = 0; i < size; i++)
		{
			data[i] = obj->data(i);
		}
	}
	else
	{
		_error_();
	}
}

ArduinoObject Packet::toArduinoObject()
{
	ArduinoObject out = ArduinoObject(data, size);
	return out;
}

#endif

// Mostrar informacion del paquete
void Packet::print(unsigned char mode = DEC)
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
String Packet::toStr()
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