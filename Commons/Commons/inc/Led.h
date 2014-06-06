/* 
 * File:   Led.h
 * Author: lieven
 *
 * Created on September 1, 2013, 7:36 PM
 */

#ifndef LED_H
#define	LED_H
#include "Timer.h"
#include "Stream.h"

class Led : public Stream {
public:
	enum State  { BLINK,OFF,ON,ONCE };
    enum LedColor {LED_RED,LED_GREEN,LED_BLUE};
    Led(LedColor color);
    ~Led();
    void init();
    void blink(int frequency);
    void light(bool on);
    void toggle();
    void once(int interval);
    Erc event(Event* event);
private:
    void on();
    void off();

    State  _state;
    Timer _timer;
    bool _isOn;
    uint16_t _frequency;
    int _led;
};

#endif	/* LED_H */

