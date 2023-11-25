#include <stddef.h>
#include <stdint.h>

int32_t fcvt_w_s(float x) {
	__asm volatile("fcvt.w.s a0, fa0");
}

uint32_t fcvt_wu_s(float x) {
        __asm volatile("fcvt.wu.s a0, fa0");
}

float fcvt_s_d(double x) {
	__asm volatile("fcvt.s.d fa0, fa0");
}

float fcvt_s_w(int x) {
	__asm volatile("fcvt.s.w fa0, a0");
}

float fcvt_s_wu(unsigned x) {
	__asm volatile("fcvt.s.wu fa0, a0");
}



float f_2op_add_s(float x, float y) {
	return x + y;
}

float f_2op_sub_s(float x, float y) {
	return x - y;
}

float f_2op_mul_s(float x, float y) {
	return x * y;
}

float f_2op_div_s(float x, float y) {
	return x / y;
}

float f_2op_min_s(float x, float y) {
#if RV32_NOASM
        return x < y ? x : y;
#else
        __asm volatile("fmin.s fa0, fa0, fa1");
#endif
}


float f_2op_max_s(float x, float y) {
#if RV32_NOASM
	return x > y ? x : y;
#else
	__asm volatile("fmax.s fa0, fa0, fa1");
#endif
}


float f_sqrt_s(float x) {
	__asm volatile("fsqrt.s fa0, fa0");
}

float f_abs_s(float x) {
	__asm volatile("fabs.s fa0, fa0");
}

float f_neg_s(float x) {
	__asm volatile("fneg.s fa0, fa0");
}


int32_t g_sdataBuf[5] __attribute__ ((aligned(16))) = { -1 };

void* get_data_buf() {
	return g_sdataBuf;
}

uint32_t get_data_buf_size() {
	return (uint32_t)sizeof(g_sdataBuf);
}

void poke8(uint8_t* p, uint8_t val) {
	*p = val;
}

int peek_u8(uint8_t* p) {
	return *p;
}

int peek_i8(int8_t* p) {
	return *p;
}

void poke16(uint16_t* p, uint16_t val) {
	*p = val;
}

int peek_u16(uint16_t* p) {
        return *p;
}

int peek_i16(int16_t* p) {
        return *p;
}

void poke32(uint32_t* p, uint32_t val) {
	*p = val;
}

int peek32(int32_t* p) {
	return *p;
}

uint32_t fib(uint32_t x) {
	if (x >= 2) {
		x = fib(x - 1) + fib(x - 2);
	}
	return x;
}

uint32_t u32_hex(const char* pStr, size_t len) {
	uint32_t val = 0;
	if (pStr) {
		if (len > 0 && len <= 8) {
			const char* p0 = pStr;
			const char* p = pStr + len - 1;
			int s = 0;
			while (p >= p0) {
				static char tbl[] = { 0, '0', 'a' - 10, 0, 'A' - 10, 0, 0, 0 };
				char c = *p;
				uint32_t d = 0;
				int i0 = (c >= '0' && c <= '9');
				int i1 = (c >= 'a' && c <= 'f');
				int i2 = (c >= 'A' && c <= 'F');
				uint32_t offs = tbl[i0 | (i1<<1) | (i2<<2)];
				d = c - offs;
				d &= offs != 0 ? (uint32_t)-1 : 0;
				val |= d << s;
				s += 4;
				--p;
			}
		}
	}
	return val;
}


#include "sincos.c"
#include "matrix.c"

void _start() {
	__asm volatile ("sub  a0, a0, a0");
	__asm volatile ("ebreak");
}
