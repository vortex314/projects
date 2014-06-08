/*
 * CircBuf.h
 *
 *  Created on: 19-aug.-2012
 *      Author: lieven
 */

#ifndef CIRCBUF_H_
#define CIRCBUF_H_
//#include "base.h"
#include <stdint.h>
#include <errno.h>

#define POW 4
#define CIRCBUF_SIZE (1<<POW)
#define CIRCBUF_MASK (CIRCBUF_SIZE-1)

class CircBuf {
private:
	uint8_t* start;
	uint16_t readPos;
	uint16_t writePos;
	uint16_t limit;
public:
	CircBuf();
	CircBuf(int size);
	~CircBuf();
	int write(uint8_t b);
	int read();
        bool hasSpace();
        bool hasData();
        void clear();
	uint32_t size();
};

#endif /* CIRCBUF_H_ */
