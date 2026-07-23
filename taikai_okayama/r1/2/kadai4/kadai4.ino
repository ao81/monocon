// 17:12

#define USE_TIMER3_ISR
#include "mono_con.h"

const int dmspeed = 80;
const int lowEdge = 0 + 200;

int tsw, sw, ypos, ph;
int presw, preypos;
bool sw1ps = false, sw2ps = false;

int phase = 0;

word in, spm;

typedef union {
	struct {
		char sm : 1;
		char dm : 1;
		char smiscw : 1;
		char dmiscw : 1;
	} fl;
	char all;
} Status;
Status st;

void DMcw() {
	analogWrite(FIN_PIN, dmspeed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, dmspeed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ph = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ypos = analogRead(A2);

		if (sw == HIGH && presw == LOW) sw1ps = true;
		if (ypos < lowEdge && preypos >= lowEdge) sw2ps = true;

		presw = sw;
		preypos = ypos;
	}

	spm++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ypos = preypos = analogRead(A2);

	st.all = 0;
}

void loop() {
	if (sw1ps) {
		sw1ps = false;
		tsw = digitalRead(_USER_CON_5PIN);
		st.fl.smiscw = (tsw != HIGH);
		st.fl.sm = true;
	}

	if (sw2ps) {
		sw2ps = false;
		tsw = digitalRead(_USER_CON_5PIN);
		st.fl.dmiscw = (tsw != HIGH);
		st.fl.dm = true;
	}

	if (st.fl.sm) {
		if (spm > 40) {
			spm = 0;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			if (st.fl.smiscw) {
				if (++phase > 3) phase = 0;
			} else {
				if (--phase < 0) phase = 3;
			}
		}
	}

	if (st.fl.dm) {
		if (st.fl.dmiscw) DMcw();
		else DMccw();
	} else DMstop();

	if (ph == HIGH) st.all = 0;
}