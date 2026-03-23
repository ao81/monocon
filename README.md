### 割り込みの書き方

```c++
#define USE_TIMER3_ISR
#include <mono_con.h>

word tc;

ISR (TIMER3_COMPA_vect) {
	tc++;
}

void loop() {
	if (tc > 1000) {
		tc = 0;
		// 1秒毎に実行
	}
}
```
