#include "Link.h"

Link::Link()
{
    //ctor
}

Link::~Link()
{
    //dtor
}
const int  Link::RXD=Event::nextEventId(( char* const )"Link::RXD");
const int Link::CONNECTED=Event::nextEventId(( char* const )"Link::CONNECTED");
const int Link::DISCONNECTED=Event::nextEventId(( char* const )"Link::DISCONNECTED");
