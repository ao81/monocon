#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

const int na[] = { NOTE_C4, NOTE_E4, NOTE_G4 };
const int da[] = { 200, 200, 400 };

const int nb[] = { NOTE_G4, NOTE_E4, NOTE_C4 };
const int db[] = { 200, 200, 400 };

void loop() {
	if (in.d(d2).fall) mel.play(na, da, 3);
	if (in.d(d1).rise) mel.play(nb, db, 3);
	mel.update();
}
