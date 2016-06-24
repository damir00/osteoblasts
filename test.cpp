#include <stdio.h>

#include <cmath>

#define LOG(...) printf("%s:%d ",__FILE__,__LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout);


class BlinkCurve {
public:
	int count;
	float interval;
	float position;

	BlinkCurve() {
		count=3;
		interval=500;
		count*interval+1;
	}
	void reset() {
		position=0;
	}
	void update(float delta) {
		position+=delta;
	}
	bool get() {
		if(done()) {
			return false;
		}
		return ((int)position%(int)interval)<(interval/2);
	}
	bool done() {
		return (position>=(float)count*interval);
	}
};

int main() {

	BlinkCurve blink;

	float delta=100;
	float ts=0;

	blink.reset();
	while(true) {
		ts+=delta;
		blink.update(delta);
		printf("ts %f, blink %d\n",ts,blink.get());
		if(blink.done()) {
			printf("blink done\n");
			break;
		}
	}


	return 0;
}

