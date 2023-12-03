enum {
	ECALL_OUTSTR = 0,
	ECALL_OUTINT,
	ECALL_OUTHEX,
	ECALL_OUTPTR,
	ECALL_OUTF32,
	ECALL_ENVINFO,
	ECALL_STRLEN,
	ECALL_MATH,

	ECALL_MAX
};

typedef struct _ENV_INFO {
	uint32_t codeOrg;
} ENV_INFO;

enum {
	EMATH_NOP = 0,
	EMATH_SIN,
	EMATH_COS,
	EMATH_POW,

	EMATH_MAX
};

typedef struct _EMATH_ARGS {
	int32_t func;
	float x;
	float y;
	float res;
} EMATH_ARGS;
