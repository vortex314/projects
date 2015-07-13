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

#include "Timer.h"
#include "Msg.h"
#include "Handler.h"
#include "Usb.h"
#include "Mqtt.h"

extern Usb usb;

class MqttClient: public Handler {
	Mqtt* _mqtt;
	Str sub;
public:
	MqttClient(Mqtt* mqtt) :sub(30){
		_mqtt = mqtt;

	}
	bool dispatch(Msg& event) {
		PT_BEGIN()
		START: {

			PT_WAIT_UNTIL(event.is(_mqtt, SIG_CONNECTED));
			sub.clear();
			sub << "limero1/+";
			_mqtt->subscribe(sub);
		}
		END: {
			PT_WAIT_UNTIL(event.is(_mqtt, SIG_DISCONNECTED));
			goto START;
		}

	PT_END()
	return true;
}

};
#include "Prop.h"
static uint64_t bootTime;
class UptimeTopic: public Prop {
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags )
					{ T_UINT64, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(Sys::upTime());
	}
};

UptimeTopic uptime;
class RealTimeTopic: public Prop {
public:

	RealTimeTopic() :
			Prop("system/now", (Flags )
					{ T_UINT64, M_RW, T_1SEC, QOS_0, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(bootTime + Sys::upTime());
	}
	void fromBytes(Bytes& message) {
		Json json(message);
		double d;
		uint64_t now;
		if (json.get(d)) {
			now = d;
			bootTime = now - Sys::upTime();
		}
	}
};
RealTimeTopic rt;
class SystemOnlineTopic: public Prop {
public:
	SystemOnlineTopic() :
			Prop("system/online", (Flags )
					{ T_BOOL, M_READ, T_10SEC, QOS_1, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(true);
	}
};
SystemOnlineTopic systemOnline;

int main(void) {
	initBoard();

	Mqtt mqtt(usb);
	MqttClient mqc(&mqtt);
	mqtt.setPrefix("limero1/");
	PropMgr propMgr;
	propMgr.setMqtt(&mqtt);
	propMgr.setPrefix("limero1/");



	uint64_t clock = Sys::upTime() + 100;
	MsgQueue::publish(0, SIG_INIT, 0, 0);				// kickoff all engines
	Msg msg;

	while (1) {
		if (Sys::upTime() > clock) {
			clock += 10;		// 10 msec timer tick
			MsgQueue::publish(0, SIG_TICK, 0, 0); // check timeouts every 10 msec
		}
		if (usb.hasData())	// if UART has received data alert uart receiver
			MsgQueue::publish(&usb, SIG_RXD);
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
