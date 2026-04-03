// ロータリーエンコーダの代わりにsw1, sw2で解く

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[4][2] = {{ 0x01, 0x01 }, { 0x40, 0x40 }, { 0x08, 0x08 }, { 0x00, 0x00 }};

word in;

volatile bool sw1edge = false, sw2edge = false;
volatile int sw1, sw2;
int presw1, presw2;

ISR (TIMER3_COMPA_vect) {
    if (in > 5) {
        in = 0;
        sw1 = digitalRead(_USER_CON_5PIN);
        sw2 = digitalRead(_USER_CON_3PIN);

        if (sw1 == LOW && presw1 == HIGH) sw1edge = true;
        if (sw2 == LOW && presw2 == HIGH) sw2edge = true;

        presw1 = sw1;
        presw2 = sw2;
    }
    in++;
}

void setup() {
	config_init();
	serial_init();

	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);

	disp(0x00, 0x00);
}

void loop() {
    static int ptnidx = 3;

    if (sw1edge) {
        sw1edge = false;
        if (--ptnidx < 0) ptnidx = 2;
        disp(ptn[ptnidx][0], ptn[ptnidx][1]);
    } else if (sw2edge) {
        sw2edge = false;
        if (++ptnidx > 2) ptnidx = 0;
        disp(ptn[ptnidx][0], ptn[ptnidx][1]);
    }
}