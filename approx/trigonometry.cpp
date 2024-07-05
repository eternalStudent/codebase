inline float32 PI() {
	union {uint32 u; float32 f;} temp;
	temp.u = 0x40490fdb;
	return temp.f;
}

// stolen shamelessly from: https://marc-b-reynolds.github.io/math/2020/03/11/SinCosPi.html
//-------------------------------------------------------------------------------------------

inline float32 mulsign(float32 v, uint32 s) { 
	union {uint32 u; float32 f;} cvt;
	cvt.f = v;
	cvt.u ^= s;
	return cvt.f;
}

// dst[0] = cos(pi a), dst[1] = sin(pi a)
void sincospi(float32 a, float32* sin, float32* cos) {
	float32 dst[2];
	// constants for sin(pi a) and cos(pi a) for a on [-1/4,1/4]
	static float32 S[] = { 0x1.921fb6p1f, -0x1.4abbecp2f,  0x1.466b2p1f, -0x1.2f5992p-1f };
	static float32 C[] = {-0x1.3bd3ccp2f,  0x1.03c1b8p2f, -0x1.55b7cep0f, 0x1.d684aap-3f };

	float32 c, s, a2, a3, r;
	uint32 q, sx, sy;

	// range reduce
	r  = round(a + a);
	a  = fmadd(r, -0.5f, a);
	
	// setting up for reconstruct
	q  = (uint32)(int32)r;
	sy = (q<<31); sx = (q>>1)<<31; sy ^= sx; q &= 1; 
	
	// compute the approximations
	a2 = a*a;
	c  = fmadd(C[3], a2, C[2]); s = fmadd(S[3], a2, S[2]); a3 = a2*a;
	c  = fmadd(c,    a2, C[1]); s = fmadd(s,    a2, S[1]); 
	c  = fmadd(c,    a2, C[0]); s = a3 * s;
	c  = fmadd(c,    a2,  1.f); s = fmadd(a,    S[0], s);
	c  = mulsign(c, sx);         s = mulsign(s, sy);

	dst[q  ] = c;
	dst[q^1] = s;

	if (cos) *cos = dst[0];
	if (sin) *sin = dst[1];
}

inline void sincos(float32 t, float32* sin, float32* cos) {
	const float32 pi = PI();
	float32 a = t / pi;
	sincospi(a, sin, cos);
}

inline float32 cos(float32 t) {
	float32 cos;
	sincos(t, NULL, &cos);
	return cos;
}

inline float32 sin(float32 t) {
	float32 sin;
	sincos(t, &sin, NULL);
	return sin;
}