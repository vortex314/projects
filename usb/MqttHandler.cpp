#include "MqttHandler.h"

MqttHandler::MqttHandler()
{
    //ctor
}

MqttHandler::~MqttHandler()
{
    //dtor
}

void MqttHandler::dispatch(Msg& msg)
{
    Signal signal = msg.sig();
    if ( accept(signal ) )
    {
        if (signal == SIG_TIMER_TICK)
        {
            if (timeout())
                onTimeout();
        }
        else if (signal == SIG_MQTT_MESSAGE)
        {
            Bytes bytes(0);
            msg.get(bytes);
            MqttIn mqttIn(&bytes);
            bytes.offset(0);
            if (mqttIn.parse())
                onMqttMessage(mqttIn);
            else
                Sys::warn(EINVAL, "MQTT");
        }
        else if ( signal == SIG_INIT )
        {
            onInit();
        }
        else
            onOther(msg);
    }
}
