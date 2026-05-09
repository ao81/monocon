#define USE_TIMER3_ISR
#include "mono_con.h"

word in, spm;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;


	}

	spm++;
}

