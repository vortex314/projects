/* 
 * File:   Led.h
 * Author: lieven
 *
 * Created on September 1, 2013, 7:36 PM
 */

#ifndef LED_H
#define	LED_H
#include <stdint.h>
#include "Erc.h"


class Led  {
public:
	enum State  { BLINK,OFF,ON,ONCE };
    enum LedColor {LED_RED,LED_GREEN,LED_BLUE};
    Led(LedColor color);
    ~Led();
    void init();
    void on();
    void off();
    int _led;
};

#endif	/* LED_H */

