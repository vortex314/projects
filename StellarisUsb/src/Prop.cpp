/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Prop.h"

#define PROPS_MAX   20
Prop* propList[PROPS_MAX];
uint32_t propListCount = 0;

Prop::Prop (const char* name, void* instance, Xdr xdr, Flags flags)
{
  _name = name;
  _instance = instance;
  _xdr = xdr;
  _flags = flags;
  propList[propListCount++] = this;
  _flags.publishValue = true;
  _flags.publishMeta = true;
}

Prop*
Prop::findProp (Str& name)
{
  uint32_t i;
  for (i = 0; i < propListCount; i++)
    {
      if (propList[i])
	if (name.equals (propList[i]->_name))
	  {
	    return propList[i];
	  }
    }
  return 0;
}

void
Prop::set (Str& topic, Strpack& message, uint8_t header)
{
  Str str (30);
//	str.substr(topic, getPrefix.length());
  Prop* p = findProp (str);
  if (p)
    {
      if (p->_xdr)
	p->_xdr (p->_instance, CMD_PUT, message);
      p->_flags.publishValue = true;
    }
  Sequence::publish (Prop::CHANGED);
}

const int Prop::CHANGED = Event::nextEventId ("Prop::CHANGED");

PropertyListener::PropertyListener (Mqtt& mqtt) :
    _mqtt (mqtt),_topic(30)
{
  PT_INIT(&t);
}

int
PropertyListener::handler (Event* event)
{
  Prop *p;
  Strpack strp (40);
  uint32_t i;
  PT_BEGIN ( &t )
  ;
  while (true)
    {
      while (_mqtt.isConnected ())
	{ // new things to publish found
	  for (i = 0; i < propListCount; i++)
	    {
	      p = propList[i];
	      if (p->_flags.publishValue)
		{

		  if (p->_xdr)
		    {
		      p->_flags.publishValue = false;
		      _topic.set (p->_name);

		      p->_xdr (p->_instance, CMD_GET, strp);
		      _mqtt.Publish (p->_flags, i, _topic, strp);
		      if (p->_flags.publishValue == false)
			{
			  timeout (50);
			  PT_YIELD_UNTIL(&t, timeout ());
			}
		    }
		}
	      else if (p->_flags.publishMeta)
		{
		  // convert flags to string
		  // add desc
		  p->_flags.publishMeta = false;
		}
	    }
	  timeout (5000); // sleep between scans
	  PT_YIELD_UNTIL(&t, timeout () || event->is (Prop::CHANGED));
	}
      PT_YIELD(&t); // yield during tcp disconnects
    }
PT_END ( &t );
}

