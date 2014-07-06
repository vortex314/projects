#include "Mqtt.h"
#include "Sys.h"
#include "Logger.h"
extern char* getDeviceName();
Mqtt::Mqtt( Tcp* tcp)
{
    _tcp=tcp;
    _tcp->addListener(this);
    _timer =  new Timer();
    _timer->addListener(this);
    _mqttOut = new MqttOut(256);
    _mqttOut->prefix(getDeviceName());

}
void Mqtt::onTcpConnect(Tcp* src)
{
    _mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION,
                      "ikke/alive", "false", "", "", 1000);
    _tcp->send(_mqttOut);
}
void Mqtt::onTcpDisconnect(Tcp* src)
{
    MqttListener* l;
    for(l=firstListener(); l!=0; l=nextListener(l))
        l->onMqttDisconnect(this);
}
void Mqtt::onTcpMessage(Tcp* src,Bytes* data)
{
    MqttIn *packet=(MqttIn*)data;
    if ( packet->type() == MQTT_MSG_CONNACK)
    {
        if ( packet->_returnCode == MQTT_RTC_CONN_ACC)
        {
            MqttListener* l;
            for(l=firstListener(); l!=0; l=nextListener(l))
                l->onMqttConnect(this);
        }
        else
        {
            Sys::getLogger().append("Mqtt Connect failed , return code : ").append( packet->_returnCode);
            Sys::getLogger().flush();
            _tcp->disconnect();
        }
    }
    else
    {
        MqttListener* l;
        for(l=firstListener(); l!=0; l=nextListener(l))
            l->onMqttMessage(this,packet);
    }
};
void Mqtt::onTimerExpired(Timer* timer)
{
}

Erc Mqtt::send(Bytes* bytes)
{
    return _tcp->send(bytes);
}
