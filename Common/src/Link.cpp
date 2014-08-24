#include "Link.h"

const int Link::CONNECTED = Event::nextEventId("Link::CONNECTED");
const int Link::DISCONNECTED = Event::nextEventId("Link::DISCONNECTED");
const int Link::MESSAGE = Event::nextEventId("Link::MESSAGE");

Link::Link()
{
   _isConnected=false;
}

Link::~Link()
{
    //dtor
}
