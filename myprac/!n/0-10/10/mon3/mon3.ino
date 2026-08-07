#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Js js(a1, a2);
int n = -1, offset = 2;
bool mirror = false;

const char* ptn4[][3] = {
	{"",".",""},
	{"",""," .."},
	{"","   .",""},
	{"    ..","",""},
};

const char *ptn8[][3] = {
	{"",".",""},
	{"","",".."},
	{"",""," .."},
	{"","","  .."},
	{"","   .",""},
	{"   ..","",""},
	{"    ..","",""},
	{".    .","",""},
};

void setup() { delay(100); }

void loop() {
	if (!ts) n = js.dir(4, offset, mirror);
	else n = js.dir(8, offset, mirror);
	if (n == -1) dp.off();
	else dp.art(ts ? ptn8[n] : ptn4[n]);
	if (sw1.htol()) offset = (offset + 1) % 4;
	if (sw2.htol()) mirror = !mirror;
	if (sw3.htol()) { mirror = false; offset = 2; }
	if (sig(n).change()) bz(1000,  50);
	led(ts ? B : G);
}
