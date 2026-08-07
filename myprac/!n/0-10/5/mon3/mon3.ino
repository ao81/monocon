#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static bool move = false;
	static int goal = 0, pulse = 0;
	if (!move) {
		goal = in.encClamp(a3, a4, 0, 60);
		dispn(goal);
		if (in.htol(d3)) { move = true; pulse = 0; }
	} else {
		if (in.ltoh(PH_PIN)) pulse++;
		if (pulse < goal && pulse > goal - 10) {
			dm.drive(50);
		} else if (pulse < goal) {
			dm.drive(100);
		} else {
			dm.br();
			move = false;
			in.encSet(a3, a4, 0);
		}
		dispn(goal - pulse);
	}
}
