#define USE_TIMER3_ISR
#include "mono_con.h"

word in;

ISR(TIMER_COMPA_vect) {
	if (in++ > 5) {
		in = 0;


	}
}
