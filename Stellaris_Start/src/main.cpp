/*
 * main.cpp
 *
 *  Created on: 26-aug.-2014
 *      Author: lieven2
 */

#include "Board.h"
#include "Sys.h"
#include "Str.h"
#include "malloc.h"


extern "C" int
main(void)
{
	Board::init();
	Str str(100);
	int count=0;
	while(1){
		uint64_t clock = Sys::upTime();
		str << "A";
		uint8_t *start;
		while(true) {
			start = (uint8_t*)malloc(1000);
			if(start!=0) {
				count++;
			} else while(1);
		}

	}
}

