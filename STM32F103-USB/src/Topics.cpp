#include "Timer.h"
#include "Msg.h"
#include "Handler.h"
#include "Usb.h"
#include "Mqtt.h"
#include <stdlib.h>
#include "Gpio.h"
#include "Prop.h"

typedef struct {
	uint32_t uiValue;
	const char *szValue;
} EnumEntry;

bool strToEnum(uint32_t& iValue, EnumEntry* et, uint32_t count, Str& value) {
	uint32_t i;
	for (i = 0; i < count; i++)
		if (value.startsWith(et[i].szValue)) {
			iValue = et[i].uiValue;
			return true;
		}
	return false;
}

bool enumToStr(Str& str, EnumEntry* et, uint32_t count, uint32_t value) {
	uint32_t i;
	for (i = 0; i < count; i++)
		if (et[i].uiValue == value) {
			str << et[i].szValue;
			return true;
		}
	return false;
}
//_____________________________________________________________________________
//

//_____________________________________________________________________________
//
typedef uint64_t (*pfu64)(uint64_t val);
class UInt64Topic: public Prop {
	pfu64 _fp;
public:
	UInt64Topic(const char *name, pfu64 fp) :
			Prop(name, (Flags )
					{ T_UINT64, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
		_fp = fp;
	}
	void toBytes(Bytes& message) {
		Str& str = (Str&) message;
		str.append(_fp(0));
	}
	void fromBytes(Bytes& message) {
		Str& str = (Str&) message;
		_fp(atoll(str.c_str()));
	}
};
//_____________________________________________________________________________
//
class StringTopic: public Prop {
	const char *_s;
public:
	StringTopic(const char *name, const char *s) :
			Prop(name, (Flags )
					{ T_STR, M_READ, T_10SEC, QOS_0, NO_RETAIN }) {
		_s = s;
	}
	void toBytes(Bytes& message) {
		((Str&) message).append(_s);
	}
};
//_____________________________________________________________________________
//
// GET SERIAL NUMBER
class SerialTopic: public Prop {
public:
	SerialTopic() :
			Prop("system/id", (Flags )
					{ T_STR, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
	}
	void toBytes(Bytes& message) {
		Str& str = (Str&) message;
		uint8_t* start = (uint8_t*) (0x1FFFF7E8);
		for (int i = 0; i < 12; i++)
			str.appendHex(*(start + i));

	}
};
//_____________________________________________________________________________
//
class RealTimeTopic: public Prop {
	uint64_t bootTime;
public:
	RealTimeTopic() :
			Prop("system/now", (Flags )
					{ T_UINT64, M_RW, T_1SEC, QOS_0, NO_RETAIN }) {
		bootTime = 0;
	}

	void toBytes(Bytes& message) {
		Str& str = (Str&) message;
		str.append(bootTime + Sys::upTime());
	}
	void fromBytes(Bytes& message) {
		Str& str = (Str&) message;
		uint64_t now = atoll(str.c_str());
		bootTime = now - Sys::upTime();
	}
};
//_____________________________________________________________________________
//
class SystemOnlineTopic: public Prop {
public:
	SystemOnlineTopic() :
			Prop("system/online", (Flags )
					{ T_BOOL, M_READ, T_1SEC, QOS_1, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Str& str = (Str&) message;
		str.append(true);
	}
};
//_____________________________________________________________________________
//
EnumEntry gpioModeEnum[] = { //
		{ Gpio::OUTPUT_PP, "OUTPUT_PP" }, //
				{ Gpio::INPUT, "INPUT" }, //
				{ Gpio::OUTPUT_OD, "OUTPUT_OD" } //
		};
class GpioModeProp: public Prop {
EnumEntry* _pe;
uint32_t _count;
Gpio& _gpio;
public:
GpioModeProp(const char *name, Gpio& gpio, EnumEntry* pe, uint32_t count,
		Flags flags) :
		Prop(name, flags), _gpio(gpio) {
	_pe = pe;
	_count = count;

}
void toBytes(Bytes& message) {
	Str& str = (Str&) message;
	enumToStr(str, gpioModeEnum, sizeof(gpioModeEnum) / sizeof(EnumEntry),
			_gpio.getMode());
}
void fromBytes(Bytes& message) {
	Str& str = (Str&) message;
	Gpio::Mode mode;
	if (strToEnum((uint32_t&) mode, gpioModeEnum,
			sizeof(gpioModeEnum) / sizeof(EnumEntry), str)) {
		_gpio.setMode(mode);
	}
}
};
class GpioTopics {
Gpio& _gpio;
public:
GpioTopics(const char* s, Gpio& gpio) :
		_gpio(gpio) {
	Str str(30);
	str.set(s).append("/mode");
	new GpioModeProp(str.c_str(), gpio, gpioModeEnum, 1, (Flags ) { T_STR, M_RW,
					T_10SEC, QOS_1, NO_RETAIN });
}
};
//_____________________________________________________________________________
//
uint64_t memoryAllocated(uint64_t v) {
return mallinfo().arena;
}

uint64_t memoryFreeBlocks(uint64_t v) {
return mallinfo().ordblks;
}

static uint64_t bootTime;

uint64_t now() {
return bootTime + Sys::upTime();
}
uint64_t uptime(uint64_t) {
return Sys::upTime();
}

UInt64Topic uptimeProp("system/uptime", uptime);
UInt64Topic memProp("system/memory/allocated", memoryAllocated);
UInt64Topic memFreeProp("system/memory/freeBlocks", memoryFreeBlocks);
SystemOnlineTopic systemOnline;
StringTopic systemVersion("system/version", __DATE__ " " __TIME__);
SerialTopic st;
RealTimeTopic rt;
Gpio gpioLed(Gpio::PORT_C, 13);
//GpioTopics ledTopic("gpio/PC13", gpioLed);

class LedBlink: public Handler {
bool _isOn;
uint32_t _msecInterval;
Mqtt* _mqtt;
Gpio _gpio;
public:
LedBlink(Mqtt* mqtt, Gpio& gpio) :
		Handler("LedBlink"), _gpio(gpio) {
	_isOn = false;
	_msecInterval = 100;
	_mqtt = mqtt;
}

virtual ~LedBlink() {
}

bool dispatch(Msg& msg) {
	PT_BEGIN()
	_gpio.init();
	_gpio.setMode(Gpio::OUTPUT_PP);
	while (true) {
		timeout(_msecInterval);
		PT_YIELD_UNTIL(
				msg.is(_mqtt, SIG_CONNECTED) || msg.is(_mqtt, SIG_DISCONNECTED)
						|| timeout());
		switch (msg.signal) {
		case SIG_TICK: {
			_gpio.write(_isOn);
			_isOn = !_isOn;
			break;
		}
		case SIG_CONNECTED: {
			_msecInterval = 500;
			break;
		}
		case SIG_DISCONNECTED: {
			_msecInterval = 100;
			break;
		}
		default: {
		}
		}

	}
PT_END()
;
}

};
extern Mqtt mqtt;
LedBlink lebBlink(&mqtt, gpioLed);
