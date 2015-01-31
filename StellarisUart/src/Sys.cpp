/*
 * Sys.cpp
 *
 *  Created on: 20-aug.-2014
 *      Author: lieven2
 */
#include "Sys.h"
#include "Board.h"
//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************

extern "C" void SysTickIntHandler(void) {
	Sys::_upTime++;
}

uint64_t Sys::_upTime=0;

uint64_t Sys::upTime(){
	return Sys::_upTime;
}
#include "Prop.h"
Str strLog(100);
Json jsonLog(strLog);

uint32_t Sys::_errorCount=0;
void Sys::warn(int err,const char* s){
_errorCount++;
jsonLog.clear().addMap(3);
jsonLog.addKey("uptime").add(Sys::upTime());
jsonLog.addKey("errorCount").add((uint64_t)_errorCount);
jsonLog.addKey("erc").add(err);
jsonLog.addKey("str").add(s);
jsonLog.addBreak();
Board::setLedOn(Board::LED_RED, true);
}


class LogLine: public Prop {
public:
	LogLine() :
			Prop("system/lastLog", (Flags )
					{ T_OBJECT, M_READ, T_10SEC, QOS_0, NO_RETAIN }) {
	}
	void toBytes(Bytes& message) {
			Json json(message);
			json=jsonLog;
		}
	};

LogLine logLine;

