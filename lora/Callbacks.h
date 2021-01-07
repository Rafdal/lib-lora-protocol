#ifndef CALLBACKS_H
#define CALLBACKS_H

typedef void (*packet_callback_t)(Packet);
typedef void (*callback_t)(void);

class Callbacks
{
private:
    uint8_t max_callbacks;
    callback_t* callbacks;
public:
    Callbacks(uint8_t _max_callbacks = 10){
        max_callbacks = _max_callbacks;


        callbacks = new callback_t[max_callbacks];
        for (uint8_t i = 0; i < max_callbacks; i++)
            callbacks[i] = (callback_t)0; 
    }

    ~Callbacks(){
        if(callbacks)
            delete[] callbacks;
    }

    void setCallback(uint8_t id, callback_t callback){
        if (id < max_callbacks)
        {
            callbacks[id] = callback;
        }
    }

    void del(uint8_t id){
        if (id < max_callbacks)
        {
            callbacks[id] = (callback_t)0;
        }
    }

    void call(uint8_t id){
        if (callbacks[id] != NULL && id < max_callbacks)
        {
            (*callbacks[id])();
        }
    }
};

#endif