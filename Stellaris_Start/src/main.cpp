/*
 * main.cpp
 *
 *  Created on: 26-aug.-2014
 *      Author: lieven2
 */

#include "Board.h"
#include "Sys.h"
#include "Str.h"


extern "C" int
main(void)
{
	Board::init();
	Str str(100);
	while(1){
		uint64_t clock = Sys::upTime();
		str << "A";
	}
}

