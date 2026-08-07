#include "monocon_chuugoku.h"

Di sw(d1);

void loop() {
	led(sw ? 0 : R);
}
