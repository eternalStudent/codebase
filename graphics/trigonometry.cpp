float32 _sin(float32 x) {
	float32 x2 = x * x;
	union {uint32 u; float32 f;} data1, data2, data3, data4;
	data1.u = 0x3F800000;
	data2.u = 0xBE2AAAA0;
	data3.u = 0x3C0882C0;
	data4.u = 0xB94C6000;

	return x * data1.f + x2 * data2.f + x2 * data3.f + x2 * data4.f;
}

float32 _cos(float32 x) {
	float32 x2 = x * x;
	union {uint32 u; float32 f;} data1, data2, data3, data4;
	data1.u = 0x3F800000;
	data2.u = 0xBEFFFFDA;
	data3.u = 0x3D2A9F60;
	data4.u = 0xBAB22C00;

	return data1.f + x2 * data2.f + x2 * data3.f + x2 * data4.f;
}

float32 sinf(float32 x) {
	bool negate = false;

	// x in -infty, infty

	if (x < 0) {
		x = -x;
		negate = true;
	}

	// x in 0, infty

	float32 pi = PI();
	x -= 2 * pi * floor(x / (2 * pi));

	// x in 0, 2*pi

	if (x < pi / 2) {
	} 
	else if (x < pi) {
		x = pi - x;
	} 
	else if (x < 3 * pi / 2) {
		x = x - pi;
		negate = !negate;
	} 
	else {
		x = pi * 2 - x;
		negate = !negate;
	}

	// x in 0, pi/2

	float32 y = x < pi / 4 ? _sin(x) : _cos(pi / 2 - x);
	return negate ? -y : y;
}

float32 cosf(float32 x) {
	bool negate = false;

	// x in -infty, infty

	if (x < 0) {
		x = -x;
	}

	// x in 0, infty
	float32 pi = PI();
	x -= 2 * pi * floor(x / (2 * pi));

	// x in 0, 2*pi

	if (x < pi / 2) {
	} else if (x < pi) {
		x = pi - x;
		negate = !negate;
	} else if (x < 3 * pi / 2) {
		x = x - pi;
		negate = !negate;
	} else {
		x = pi * 2 - x;
	}

	// x in 0, pi/2

	float32 y = x < pi / 4 ? _cos(x) : _sin(pi / 2 - x);
	return negate ? -y : y;
}
