#ifndef LINK_H
#define LINK_H

#include "Event.h"

class Link
{
    public:
	static const int CONNECTED,DISCONNECTED,RXD,MESSAGE,ERROR;
        Link();
        virtual ~Link();
        bool isConnected() { return _isConnected; }
        void isConnected(bool val) { _isConnected = val; }
        virtual int32_t read()=0;
        virtual Erc send(Bytes& bytes)=0;
        virtual Bytes* recv()=0;
        virtual Erc connect()=0;
        virtual Erc disconnect()=0;
    protected:
    private:
        bool _isConnected;
};

#endif // LINK_H
