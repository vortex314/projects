#ifndef MQTT_H
#define MQTT_H
#include "MqttIn.h"
#include "MqttOut.h"
#include "Timer.h"
#include "Tcp.h"
#include "EventSource.h"


class Mqtt;

class MqttListener
{
public:
    virtual void onMqttMessage(Mqtt* mqtt,MqttIn* msg)=0;
    virtual void onMqttConnect(Mqtt* mqtt)=0;
    virtual void onMqttDisconnect(Mqtt* mqtt)=0;
};

class Mqtt : public TcpListener,public TimerListener,public EventSource
{
private:
    Tcp* _tcp;
    MqttOut* _mqttOut;
    Timer* _timer;
    Str* _prefix;
public:
    Mqtt( Tcp* tcp);
    Erc send(Bytes* bytes);
    void onTcpConnect(Tcp* src);
    void onTcpDisconnect(Tcp* src);
    void onTcpMessage(Tcp* src,Bytes* data);
    void onTimerExpired(Timer* timer);
protected:
private:
};
//__________________________________________________________________________________
//
#endif // MQTT_H
