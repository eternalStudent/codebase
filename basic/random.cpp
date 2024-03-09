struct {
	uint32 seed;
} random;
	
void RandomInit(uint32 seed) {
	random.seed = seed;
}

uint32 RandomNext() {
	uint32 result = random.seed;
	result ^= result << 13;
	result ^= result >> 17;
	result ^= result << 5;
	random.seed = result;
	return result;
}
	
// [0..1)
float64 NextFloat64() {
	return RandomNext()*(1.0/0xffffffff);
}
 
// [min..max)
uint32 RandomUniform_u(uint32 minValue, uint32 maxValue) {
 	ASSERT(minValue < maxValue);
		  
	int64 range = (int64)maxValue - minValue;
	return (uint32)(NextFloat64()*range) + minValue;
}

// [min..max)
int32 RandomUniform_i(int32 minValue, int32 maxValue) {
	ASSERT(minValue < maxValue);

	if (0 < minValue) return (int32)RandomUniform_u((uint32)minValue, (uint32)maxValue);
	return (int32)RandomUniform_u(0, (uint32)(maxValue - minValue)) + minValue;
}

// [min..max]
uint32 RandomBinomial(uint32 minValue, uint32 maxValue) {
	ASSERT(minValue < maxValue);

	uint32 sum = 0;
	int32 range = maxValue - minValue;
	for (int32 i = 0; i < range/32; i++) {
		uint32 num = RandomNext();
		sum += (uint32)BitCount(num);
	}
	uint32 extra = range%32;
	if (extra) {
		uint32 num = RandomNext() >> (32-extra);
		sum += (uint32)BitCount(num);
	}

	return sum + minValue;
}
