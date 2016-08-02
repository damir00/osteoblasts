#ifndef _BGA_EASING_H_
#define _BGA_EASING_H_

class Easing {
public:
	// no easing, no acceleration
	static float linear(float t) {
		return t;
	}
	// accelerating from zero velocity
	static float inQuad(float t) {
		return t*t;
	}
	// decelerating to zero velocity
	static float outQuad(float t) {
		return t*(2-t);
	}
	// acceleration until halfway, then deceleration
	static float inOutQuad(float t) {
		return t<.5 ? 2*t*t : -1+(4-2*t)*t;
	}
	// accelerating from zero velocity
	static float inCubic(float t) {
		return t*t*t;
	}
	// decelerating to zero velocity
	static float outCubic(float t) {
		//return (--t)*t*t+1;
		//float t1=t-1.0f;
		//return t*t1*t1+1;
		t--;
		return t*t*t+1;
	}
	// acceleration until halfway, then deceleration
	static float inOutCubic(float t) {
		return t<.5 ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1;
	}
	// accelerating from zero velocity
	static float inQuart(float t) {
		return t*t*t*t;
	}
	// decelerating to zero velocity
	static float outQuart(float t) {
		float t1=t-1.0f;
		return 1-t*t1*t1*t1;
	}
	// acceleration until halfway, then deceleration
	static float inOutQuart(float t) {
		float t1=t-1.0f;
		return t<.5 ? 8*t*t*t*t : 1-8*t*t1*t1*t1;
	}
	// accelerating from zero velocity
	static float inQuint(float t) {
		return t*t*t*t*t;
	}
	// decelerating to zero velocity
	static float outQuint(float t) {
		float t1=t-1.0f;
		return 1+t*t1*t1*t1*t1;
	}
	// acceleration until halfway, then deceleration
	static float inOutQuint(float t) {
		float t1=t-1.0f;
		return t<.5 ? 16*t*t*t*t*t : 1+16*t*t1*t1*t1*t1;
	}
};

#endif
