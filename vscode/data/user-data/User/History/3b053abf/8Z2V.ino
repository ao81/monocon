#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int idx = 0;
int tsw, sw, ph;


word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

	}
}