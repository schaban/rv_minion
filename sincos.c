
#define PI_D 3.1415926535897931
#define PI_F 3.14159274f

float sin_s(float x) {
	float val;
	float sgn;
	uint32_t ir;
	float fr;
	float s;
	float y;
	if (x < 0.0f) {
		sgn = -1.0f;
		val = -x;
	} else {
		sgn = 1.0f;
		val = x;
	}
	if (val > (float)((1<<24)-1)) {
		return 0.0f;
	}
	ir = (uint32_t)(val * (4.0f / PI_F));
	ir += ir & 1;
	fr = (float)ir;
	ir &= 7;
	if (ir > 3) {
		ir -= 4;
		sgn = -sgn;
	}
	if (val > (float)(1 << 13)) {
		val = val - (fr * (PI_F / 4.0f));
	} else {
		val -= fr * 0.78515625f;
		val -= fr * 0.000241875648f;
		val -= fr * 3.77489506e-8f;
	}
	s = val * val;
	switch (ir) {
		case 1:
		case 2:
			y = ((s*2.44331568e-5f - 0.00138873165f)*s + 0.0416666456f) * s*s;
			y -= s * 0.5f;
			y += 1.0f;
			break;
		default:
			y = ((0.00833216123f - s*0.000195152956f)*s - 0.166666552f)*s*val + val;
			break;
	}
	y *= sgn;
	return y;
}

float cos_s(float x) {
	float val;
	float sgn;
	uint32_t ir;
	float fr;
	float s;
	float y;
	if (x < 0.0f) {
		val = -x;
	} else {
		val = x;
	}
	if (val >(float)((1 << 24) - 1)) {
		return 0.0f;
	}
	sgn = 1.0f;
	ir = (uint32_t)(val * (4.0f / PI_F));
	ir += ir & 1;
	fr = (float)ir;
	ir &= 7;
	if (ir > 3) {
		ir -= 4;
		sgn = -sgn;
	}
	if (ir > 1) {
		sgn = -sgn;
	}
	if (val > (float)(1 << 13)) {
		val = val - (fr * (PI_F / 4.0f));
	} else {
		val -= fr * 0.78515625f;
		val -= fr * 0.000241875648f;
		val -= fr * 3.77489506e-8f;
	}
	s = val * val;
	switch (ir) {
		case 1:
		case 2:
			y = ((0.00833216123f - s*0.000195152956f)*s - 0.166666552f)*s*val + val;
			break;
		default:
			y = ((s*2.44331568e-5f - 0.00138873165f)*s + 0.0416666456f) * s*s;
			y -= s * 0.5f;
			y += 1.0f;
			break;
	}
	y *= sgn;
	return y;
}

