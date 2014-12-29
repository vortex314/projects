#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H
#include "Handler.h"

class MqttHandler : public Handler
{
public:
    MqttHandler();
    virtual ~MqttHandler();
    virtual void onInit() {};
    virtual void onTimeout() {};
    virtual void onMqttMessage(MqttIn& msg) {};
    virtual void onOther(Msg& msg) {};
    void dispatch(Msg& msg);
protected:
private:
};

#endif // MQTTHANDLER_H
