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
#include "rand.c"
#include "sort.c"

#include "ecalls.h"

static uintptr_t envcall(uint32_t id, uintptr_t ptr) {
	register uintptr_t argRes __asm("a0") = ptr;
	register uint32_t callId __asm("a7") = id;
	__asm volatile(
		"ecall"
		: "+r" (argRes)
		: "r" (callId)
		: "memory"
	);
	return argRes;
}

static void envcall_void(uint32_t id, uintptr_t ptr) {
	register uintptr_t arg __asm("a0") = ptr;
	register uint32_t callId __asm("a7") = id;
	__asm volatile(
		"ecall"
		:
		: "r" (callId), "r" (arg)
		: "memory"
	);
}

void* get_code_org() {
	ENV_INFO info = {};
	envcall_void(ECALL_ENVINFO, (uintptr_t)&info);
	return (void*)info.codeOrg;
}

size_t e_strlen(const char* pStr) {
	return (size_t)envcall(ECALL_STRLEN, (uintptr_t)pStr);
}

void o_str(const char* pStr) { envcall_void(ECALL_OUTSTR, (uintptr_t)pStr); }
void o_int(int i) { envcall_void(ECALL_OUTINT, (uintptr_t)&i); }
void o_hex(int i) { envcall_void(ECALL_OUTHEX, (uintptr_t)&i); }
void o_ptr(void* p) { envcall_void(ECALL_OUTPTR, (uintptr_t)p); }
void o_f32(float x) { envcall_void(ECALL_OUTF32, (uintptr_t)&x); }
void o_eol() { o_str("\n"); }

float e_sinf(float x) {
	EMATH_ARGS args;
	args.func = EMATH_SIN;
	args.x = x;
	envcall_void(ECALL_MATH, (uintptr_t)&args);
	return args.res;
}

float e_cosf(float x) {
	EMATH_ARGS args;
	args.func = EMATH_COS;
	args.x = x;
	envcall_void(ECALL_MATH, (uintptr_t)&args);
	return args.res;
}

float e_powf(float x, float y) {
	EMATH_ARGS args;
	args.func = EMATH_POW;
	args.x = x;
	args.y = y;
	envcall_void(ECALL_MATH, (uintptr_t)&args);
	return args.res;
}

void test_ecalls() {
	const char* pTestStr = "RISC-V";
	float testX = 1.23f;
	float testY = 5.53f;

	o_str("Hello from minion...");
	o_eol();

	o_str("Test string is \"");
	o_str(pTestStr);
	o_str("\", its length is ");
	o_int(e_strlen(pTestStr));
	o_str(".");
	o_eol();

	o_str("hex: ");
	o_hex(0xFABEC0DE);
	o_eol();

	o_str("code org: ");
	o_ptr(get_code_org());
	o_eol();

	o_str("test f32: ");
	o_f32(testX);
	o_eol();

	o_str("cos(sin(");
	o_f32(testX);
	o_str(")) = ");
	o_f32(e_cosf(e_sinf(testX)));
	o_eol();

	o_str("pow(");
	o_f32(testX);
	o_str(", ");
	o_f32(testY);
	o_str(") = ");
	o_f32(e_powf(testX, testY));
	o_eol();
}


void _start() {
	__asm volatile ("sub  a0, a0, a0");
	__asm volatile ("ebreak");
}
