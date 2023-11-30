static uint32_t s_rand = 1;

void rand_seed(uint32_t seed) {
	s_rand = seed;
}

uint16_t rand_u16() {
	s_rand = s_rand*214013 + 2531011;
	return (uint16_t)((s_rand >> 16) & 0x7FFF);
}

float rand_f01() {
	union { float f; uint32_t u; } bits;
	uint32_t r0 = rand_u16() & 0xFF;
	uint32_t r1 = rand_u16() & 0xFF;
	uint32_t r2 = rand_u16() & 0xFF;
	bits.f = 1.0f;
	bits.u += (r0 | (r1 << 8) | ((r2 & 0x7F) << 16));
	bits.f -= 1.0f;
	return bits.f;
}
