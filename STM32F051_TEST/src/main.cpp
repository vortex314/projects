#include "Sys.h"

#include <stm32f0xx.h>

 int main(void) {
	Sys::init();
	int i = 0;int j;

	while (true) { // EVENTPUMP
		i++;
		j++;
		j+=i;
	}
	return 0;
}
#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line) {
	while(1);
}
#endif
