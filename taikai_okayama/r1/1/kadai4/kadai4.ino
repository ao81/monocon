#define USE_TIMER3_ISR
#include "mono_con.h"

const int lowEdge = 0 + 200;

int tsw, sw, ph, ypos;
int presw, preypos;
bool swps, yposps;

word in, spm;

bool isspm, isspmcw, isdm, isdmcw;

int phase = 0;

void DMcw(int speed = 80) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed = 80) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ypos = analogRead(A2);
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (ypos < lowEdge && preypos >= lowEdge) yposps = true;
		if (sw == LOW && presw == HIGH) swps = true;

		preypos = ypos;
		presw = sw;
	}

	spm++;
}

void setup() {
	config_init();
	serial_init();

	ypos = analogRead(A2);
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);

	isspm = isspmcw = isdm = isdmcw = false;
}

void loop() {
	if (swps) {
		swps = false;
		isspmcw = tsw;
		isspm = true;
	}

	if (yposps) {
		yposps = false;
		isdmcw = tsw;
		isdm = true;
	}

	if (ph == HIGH) {
		isspm = isdm = false;
	}

	if (isspm) {
		if (spm > 30) {
			spm = 0;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			if (!isspmcw) { if (++phase > 3) phase = 0; }
			else { if (--phase < 0) phase = 3; }
		}
	}

	if (isdm) {
		if (!isdmcw) DMcw();
		else DMccw();
	} else DMstop();
}