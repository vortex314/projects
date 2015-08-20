/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @version V4.0.0
 * @date    21-January-2013
 * @brief   Virtual Com Port Demo main file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
//#include "hw_config.h"
// #include "usb_lib.h"
// #include "usb_desc.h"
// #include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : main.
 * Description    : Main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void initBoard();
#include <Handler.h>
#include <Usb.h>
#include <Mqtt.h>
#include <Prop.h>

typedef struct {
	uint32_t value;
	const char* symbol;
} SymbolEntry;

#define SZT(aaa) ( sizeof(aaa)/sizeof(SymbolEntry))

class SymbolList {
	const uint32_t _count;
	const SymbolEntry* _list;
public:
	SymbolList(const SymbolEntry* list, uint32_t count) :
			_count(count) {
		_list = list;
//		_count = count;
	}
	bool findString(uint32_t& value, Str& s) {
		for (uint32_t i = 0; i < _count; i++) {
			if (strcmp(_list[i].symbol, s.c_str()) == 0) {
				value = _list[i].value;
				return true;
			}
		}
		return false;
	}
	bool findStringEndsWith(uint32_t& value, Str& s) {
		for (uint32_t i = 0; i < _count; i++) {
			if (s.endsWith(_list[i].symbol)) {
				value = _list[i].value;
				return true;
			}
		}
		return false;
	}
	bool findValue(uint32_t value, Str& s) {
		for (uint32_t i = 0; i < _count; i++) {
			if (_list[i].value == value) {
				s = _list[i].symbol;
				return true;
			}
		}
		return false;
	}
};

enum KeywordsEnum {
	NOT_FOUND, IN, OUT, DIR, MODE, HIGH, LOW, OUTPUT_OD, OUTPUT_PP, INPUT
} EE;
const SymbolEntry keywordTable[] = { { HIGH, "HIGH" }, //
		{ LOW, "LOW" }, //
		{ IN, "IN" }, //
		{ OUT, "OUT" }, //
		{ DIR, "DIR" }, //
		{ INPUT, "INPUT" }, //
		{ MODE, "MODE" }, //
		{ OUTPUT_OD, "OUTPUT_OD" }, //
		{ OUTPUT_PP, "OUTPUT_PP" } //
};

//SymbolList keywords(keywordTable, SZT(keywordTable));
#include "Gpio.h"



class GpioOut: public Prop {
	Gpio& _gpio;
public:
	GpioOut(Gpio& gpio, const char* name) :
			Prop(name, (Flags )
					{ T_OBJECT, M_WRITE, T_1SEC, QOS_1, NO_RETAIN }), _gpio(
					gpio) {
	}
	void toBytes(Str& topic, Bytes& bytes) {
	}
	void fromBytes(Str& topic, Bytes& bytes) {
		Str& str = (Str&) bytes;
		if (str.equals("HIGH"))
			_gpio.write(1);
		else if (str.equals("LOW"))
			_gpio.write(0);

	}
};

class GpioIn: public Prop {
	Gpio& _gpio;
public:
	GpioIn(Gpio& gpio, const char* name) :
			Prop(name, (Flags )
					{ T_OBJECT, M_READ, T_1SEC, QOS_1, NO_RETAIN }), _gpio(gpio) {
	}
	void toBytes(Str& topic, Bytes& bytes) {
		Str& str = (Str&) bytes;
		if (_gpio.read()) {
			str.append("HIGH");
		} else {
			str.append("LOW");
		}
	}
	void fromBytes(Str& topic, Bytes& bytes) {
	}
};

class GpioMode: public Prop {
	Gpio& _gpio;
public:
	GpioMode(Gpio& gpio, const char* name) :
			Prop(name, (Flags )
					{ T_OBJECT, M_READ, T_1SEC, QOS_1, NO_RETAIN }), _gpio(gpio) {
	}
	void toBytes(Str& topic, Bytes& bytes) {
		Str& str = (Str&) bytes;
		if ( _gpio.getMode()==Gpio::INPUT ) str.append("INPUT");
		if ( _gpio.getMode()==Gpio::OUTPUT_OD ) str.append("OUTPUT_OD");
		if ( _gpio.getMode()==Gpio::OUTPUT_PP ) str.append("OUTPUT_PP");
	}
	void fromBytes(Str& topic, Bytes& bytes) {
		Str& str = (Str&) bytes;
		if (str.equals("OUTPUT_OD"))
			_gpio.setMode(Gpio::OUTPUT_OD);
		else if (str.equals("OUTPUT_PP"))
			_gpio.setMode(Gpio::OUTPUT_PP);
		else if (str.equals("INPUT"))
			_gpio.setMode(Gpio::INPUT);
	}
};

class GpioProperty {
	Gpio& _gpio;
public:
	GpioProperty(Gpio& gpio, const char* name) : _gpio(gpio) {
		Str str(40);
		str.set(name).append("/out");
		new GpioOut(gpio,str.c_str());
		str.set(name).append("/in");
		new GpioIn(gpio,str.c_str());
		str.set(name).append("/mode");
		new GpioMode(gpio,str.c_str());
	}
};

GpioProperty propPC13(*(new Gpio(Gpio::PORT_C, 13)),"GPIO/PC13");
GpioProperty propPA1(*(new Gpio(Gpio::PORT_A, 1)),"GPIO/PA1");
GpioProperty propPA2(*(new Gpio(Gpio::PORT_A, 2)),"GPIO/PA2");
GpioProperty propPA3(*(new Gpio(Gpio::PORT_A, 3)),"GPIO/PA3");


extern Usb usb;

class MqttClient: public Handler {
	Mqtt* _mqtt;
	Str sub;
public:
	MqttClient(Mqtt* mqtt) :
			sub(30) {
		_mqtt = mqtt;

	}
	bool dispatch(Msg& event) {
		PT_BEGIN()
		START: {

			PT_YIELD_UNTIL(event.is(_mqtt, SIG_CONNECTED));
			sub.clear();
			sub << "limero1/#";
			_mqtt->subscribe(sub);
		}
		{
			PT_YIELD_UNTIL(event.is(_mqtt, SIG_DISCONNECTED));
			goto START;
		}

	PT_END()
	return true;
}
};



#include "Uart.h"
Gpio _reset(Gpio::PORT_B, 12);
Gpio _gpio0(Gpio::PORT_B, 6);
Gpio _gpio2(Gpio::PORT_B, 7);
Uart _uart(1);

class ESP8266 {

public:
	ESP8266(){
		_reset.init();
		_gpio0.setMode(Gpio::INPUT);
	}
};

PropMgr propMgr;
Mqtt mqtt(usb);

extern "C" void resumeUsb();

int main(void) {
	initBoard();

	MqttClient mqc(&mqtt);
	mqtt.setPrefix("limero1/");
	propMgr.setMqtt(&mqtt);
	propMgr.setPrefix("limero1/");

	uint64_t clock = Sys::upTime() + 100;
	MsgQueue::publish(0, SIG_INIT, 0, 0);				// kickoff all engines
	Msg msg;

	uint64_t usbClock =Sys::upTime() + 5000;

	while (1) {
		if (Sys::upTime() > clock) {
			clock += 10;		// 10 msec timer tick
			MsgQueue::publish(0, SIG_TICK, 0, 0);// check timeouts every 10 msec
		}
		if (Sys::upTime() > usbClock) {
					usbClock += 5000;
//					resumeUsb();// check timeouts every 10 msec
				}
		/*		if (usb.hasData())	// if UART has received data alert uart receiver
		 MsgQueue::publish(&usb, SIG_RXD);*/
		// _________________________________________________________________handle all queued messages
		while (MsgQueue::get(msg)) {
			Handler::dispatchToChilds(msg);

		}

	}
}
#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
