#include "minion.c"

static int s_binInfo = 0;
static int s_echoInstrs = 0;
static int s_execDbg = 0;
static int s_gbgFregs = 0;

static int s_perfNative = 0;
static int s_perfCount = 0;

static int s_binMem = 0;
const char* s_pBinPath = NULL;
static void* s_pBinData = NULL;
static size_t s_binDataSize = 0;

static const char* s_pTestName = NULL;

#include "utils.c"

static void test_func_dump(MINION* pMi) {
	const char* pTestFunc = "sin_s";
	minion_set_pc_to_func(pMi, pTestFunc);
	if (minion_valid_pc(pMi)) {
		uint32_t i;
		uint32_t n = minion_get_func_instr_count(pMi, pTestFunc);
		minion_msg(pMi, "----------------------------------\n");
		minion_msg(pMi, "func %s @ %X, %d instrs\n", pTestFunc, pMi->pc, n);
		for (i = 0; i < n; i++) {
			uint32_t instr = minion_fetch_pc_instr(pMi);
			minion_instr(pMi, instr, MINION_IMODE_ECHO);
		}
	} else {
		minion_err(pMi, "test func not found!\n");
	}
}

static void test_exec_from_pc(MINION* pMi) {
	int echoInstrs = s_echoInstrs ? MINION_IMODE_ECHO : 0;
	int dbg = s_execDbg;
	uint32_t insCount = 0;
	while (1) {
		uint32_t instr = minion_fetch_pc_instr(pMi);
		if (dbg) {
			minion_msg(pMi, "\n ---------------- %d\n", insCount);
		}
		minion_instr(pMi, instr, MINION_IMODE_EXEC | echoInstrs);
		++insCount;
		if (dbg) {
			minion_dump_regs(pMi);
			if (s_gbgFregs) {
				minion_msg(pMi, " --- fregs ---\n");
				minion_dump_fregs_s(pMi);
			}
		}
		if (pMi->pcStatus & MINION_PCSTATUS_NATIVE) {
			break;
		}
		if (insCount > 100000) {
			minion_err(pMi, "reached execution limit!\n");
			break;
		}
	}
}

static void test_inner_mem(MINION* pMi) {
	uint32_t vptrBuf;
	size_t bufSize;
	void* pBufNative;
	int i, n;

	int ifnGetDataBuf = minion_find_func(pMi, "get_data_buf");
	int ifnGetDataBufSize = minion_find_func(pMi, "get_data_buf_size");
	int ifnPeek32 = minion_find_func(pMi, "peek32");
	int ifnPoke32 = minion_find_func(pMi, "poke32");

	minion_msg(pMi, "\nobtaining inner buffer ...\n");

	minion_set_pc_to_func_idx(pMi, ifnGetDataBuf);
	test_exec_from_pc(pMi);
	vptrBuf = minion_get_a0(pMi);
	pBufNative = minion_resolve_vptr(pMi, vptrBuf);
	minion_msg(pMi, "buf @ 0x%X, native -> 0x%p\n", vptrBuf, pBufNative);

	minion_set_a0(pMi, 0);
	minion_set_pc_to_func_idx(pMi, ifnGetDataBufSize);
	test_exec_from_pc(pMi);
	bufSize = minion_get_a0(pMi);
	minion_msg(pMi, "buf size: %d bytes\n", bufSize);
	n = bufSize / 4;

	minion_msg(pMi, "poking ...\n");
	for (i = 0; i < n; i++) {
		uint32_t pokePtr = vptrBuf + (i*4);
		minion_msg(pMi, " --> poke(0x%X, %d)\n", pokePtr, i);
		minion_set_a0(pMi, pokePtr);
		minion_set_a1(pMi, i);
		minion_set_pc_to_func_idx(pMi, ifnPoke32);
		test_exec_from_pc(pMi);
	}
	minion_msg(pMi, "peeking ...\n");
	for (i = 0; i < n; i++) {
		int peekVal;
		uint32_t peekPtr = vptrBuf + (i*4);
		minion_set_a0(pMi, peekPtr);
		minion_set_pc_to_func_idx(pMi, ifnPeek32);
		test_exec_from_pc(pMi);
		peekVal = minion_get_a0(pMi);
		minion_msg(pMi, "<-- peek(0x%X): %d\n", peekPtr, peekVal);
		if (peekVal != i) {
			minion_msg(pMi, "!!! peek-poke mismatch @ %d\n", i);
		}
	}
}

static void test_mapped_mem(MINION* pMi) {
	int buf[10];
	int ifnPeek32 = minion_find_func(pMi, "peek32");
	int ifnPoke32 = minion_find_func(pMi, "poke32");
	uint32_t testNum = 0x12345678;
	int testIdx = 1;
	uint32_t vptr = minion_mem_map(pMi, buf, sizeof(buf));
	memset(&buf, 0, sizeof(buf));
	minion_msg(pMi, "map: %p -> %X\n", buf, vptr);

	minion_set_a0(pMi, vptr + testIdx*4);
	minion_set_a1(pMi, testNum);
	minion_set_pc_to_func_idx(pMi, ifnPoke32);
	test_exec_from_pc(pMi);

	if (buf[testIdx] != testNum) {
		minion_err(pMi, "!!! mapped poke failed\n");
	}

	minion_mem_unmap(pMi, vptr);
}


static int feq_s(float x, float y) {
	float diff = x - y;
	if (diff < 0.0f) diff = -diff;
	return diff < 1e-4f;
}

static float fmin_s(float x, float y) {
	return x < y ? x : y;
}

static float fmax_s(float x, float y) {
	return x > y ? x : y;
}

static void test_f_2op_s(MINION* pMi) {
	int i;
	static struct _2OP_TEST {
		const char* pOp;
		const char* pFunc;
	} tbl[] = {
		{ "+", "f_2op_add_s" },
		{ "-", "f_2op_sub_s" },
		{ "*", "f_2op_mul_s" },
		{ "/", "f_2op_div_s" },
		{ "min", "f_2op_min_s" },
		{ "max", "f_2op_max_s" },
	};
	int ntests = sizeof(tbl) / sizeof(tbl[0]);

	float res;
	float ref;
	float val1 = 3.2f;
	float val2 = -4.5f;

	for (i = 0; i < ntests; ++i) {
		int ifn = minion_find_func(pMi, tbl[i].pFunc);
		minion_set_fa0_s(pMi, val1);
		minion_set_fa1_s(pMi, val2);
		minion_set_pc_to_func_idx(pMi, ifn);
		test_exec_from_pc(pMi);
		res = minion_get_fa0_s(pMi);
		if (strcmp(tbl[i].pOp, "+") == 0) {
			ref = val1 + val2;
		} else if (strcmp(tbl[i].pOp, "-") == 0) {
			ref = val1 - val2;
		} else if (strcmp(tbl[i].pOp, "*") == 0) {
			ref = val1 * val2;
		} else if (strcmp(tbl[i].pOp, "/") == 0) {
			ref = val1 / val2;
		} else if (strcmp(tbl[i].pOp, "min") == 0) {
			ref = fmin_s(val1, val2);
		} else if (strcmp(tbl[i].pOp, "max") == 0) {
			ref = fmax_s(val1, val2);
		}
		minion_msg(pMi, "%s %f %f\n", tbl[i].pOp, val1, val2);
		minion_msg(pMi, "ref: %f\n", ref);
		minion_msg(pMi, "res: %f\n", res);
		if (!feq_s(res, ref)) {
			minion_msg(pMi, "!!! 2op_s mismatch\n");
		}
	}
}

static void test_f_1op_s(MINION* pMi) {
	float res;
	float ref;
	float val;
	int ifnNeg = minion_find_func(pMi, "f_neg_s");
	int ifnAbs = minion_find_func(pMi, "f_abs_s");

	val = -1.23f;
	ref = -val;
	minion_set_fa0_s(pMi, val);
	minion_set_pc_to_func_idx(pMi, ifnNeg);
	test_exec_from_pc(pMi);
	res = minion_get_fa0_s(pMi);
	minion_msg(pMi, "neg -> ref: %f, res: %f\n", ref, res);

	ref = fabsf(val);
	minion_set_fa0_s(pMi, val);
	minion_set_pc_to_func_idx(pMi, ifnAbs);
	test_exec_from_pc(pMi);
	res = minion_get_fa0_s(pMi);
	minion_msg(pMi, "abs -> ref: %f, res: %f\n", ref, res);
}

static void test_fcvt(MINION* pMi) {
	int32_t ires;
	uint32_t ures;
	float fres;
	float fval;
	double dval;
	int ifnCvtWS = minion_find_func(pMi, "fcvt_w_s");
	int ifnCvtWUS = minion_find_func(pMi, "fcvt_wu_s");
	int ifnCvtSD = minion_find_func(pMi, "fcvt_s_d");

	fval = -1.2f;
	minion_set_fa0_s(pMi, fval);

	minion_set_pc_to_func_idx(pMi, ifnCvtWS);
	test_exec_from_pc(pMi);
	ires = minion_get_a0(pMi);

	minion_set_pc_to_func_idx(pMi, ifnCvtWUS);
	test_exec_from_pc(pMi);
	ures = minion_get_a0(pMi);

	minion_msg(pMi, "ires: %d\n", ires);
	minion_msg(pMi, "ures: %x\n", ures);

	dval = 3.14159265;
	minion_set_fa0_d(pMi, dval);
	minion_set_pc_to_func_idx(pMi, ifnCvtSD);
	test_exec_from_pc(pMi);
	fres = minion_get_fa0_s(pMi);
	minion_msg(pMi, "%.8f -> %.8f\n", dval, fres);
}

#include "sincos.c"

static void test_sin_s(MINION* pMi) {
	int i;
	int ifnSinS = minion_find_func(pMi, "sin_s");
	float val0 = -8.0f * PI_F;
	float val1 = 8.0f * PI_F;
	int n = 100;
	float add = (val1 - val0) / (float)n;
	float x = val0;
	minion_msg(pMi, "testing sin_s...\n");
	for (i = 0; i <= n; ++i) {
		float res;
		float ref = sin_s(x);
		minion_set_fa0_s(pMi, x);
		minion_set_pc_to_func_idx(pMi, ifnSinS);
		test_exec_from_pc(pMi);
		res = minion_get_fa0_s(pMi);
		minion_msg(pMi, "%f -> ref: %f, res: %f\n", x, ref, res);
		if (!feq_s(res, ref)) {
			minion_msg(pMi, "!!! sin_s mismatch\n");
			break;
		}
		x += add;
	}
}

static void test_cos_s(MINION* pMi) {
	int i;
	int ifnCosS = minion_find_func(pMi, "cos_s");
	float val0 = -8.0f * PI_F;
	float val1 = 8.0f * PI_F;
	int n = 100;
	float add = (val1 - val0) / (float)n;
	float x = val0;
	minion_msg(pMi, "testing cos_s...\n");
	for (i = 0; i <= n; ++i) {
		float res;
		float ref = cos_s(x);
		minion_set_fa0_s(pMi, x);
		minion_set_pc_to_func_idx(pMi, ifnCosS);
		test_exec_from_pc(pMi);
		res = minion_get_fa0_s(pMi);
		minion_msg(pMi, "%f -> ref: %f, res: %f\n", x, ref, res);
		if (!feq_s(res, ref)) {
			minion_msg(pMi, "!!! cos_s mismatch\n");
			break;
		}
		x += add;
	}
}


uint32_t fib(uint32_t x) {
	if (x >= 2) {
		x = fib(x - 1) + fib(x - 2);
	}
	return x;
}

static void test_fib(MINION* pMi) {
	int i;
	int ifn = minion_find_func(pMi, "fib");
	minion_msg(pMi, "----------------------------------\n");
	minion_msg(pMi, "fib @ func[%d]\n", ifn);

	pMi->instrsExecuted = 0;

	for (i = 0; i < 13; i++) {
		int res;
		int ref = fib(i);
		minion_set_pc_to_func_idx(pMi, ifn);
		minion_set_a0(pMi, i);
		test_exec_from_pc(pMi);
		res = minion_get_a0(pMi);
		minion_msg(pMi, "%2d: ref = %d, res = %d\n", i, ref, res);
		if (ref != res) {
			minion_msg(pMi, "!!! fib seq mismatch\n");
			break;
		}
	}
	minion_msg(pMi, "instrs executed: %d\n", pMi->instrsExecuted);
}


__attribute__((noinline)) static void perf_sincos_s(MINION* pMi) {
	int i;
	int ifnSinS = minion_find_func(pMi, "sin_s");
	int ifnCosS = minion_find_func(pMi, "cos_s");
	float val0 = -8.0f * PI_F;
	float val1 = 8.0f * PI_F;
	int n = s_perfCount > 0 ? s_perfCount : 1000000;
	float add = (val1 - val0) / (float)n;
	float x = val0;
	float sum = 0.0f;
	pMi->instrsExecuted = 0;
	for (i = 0; i <= n; ++i) {
		float s;
		float c;
		if (s_perfNative) {
			s = sin_s(x);
			c = cos_s(x);
		} else {
			minion_set_fa0_s(pMi, x);
			minion_set_pc_to_func_idx(pMi, ifnSinS);
			test_exec_from_pc(pMi);
			s = minion_get_fa0_s(pMi);
			minion_set_fa0_s(pMi, x);
			minion_set_pc_to_func_idx(pMi, ifnCosS);
			test_exec_from_pc(pMi);
			c = minion_get_fa0_s(pMi);
		}
		sum += s*s + c*c;
		x += add;
	}
	minion_msg(pMi, "%s sum = %f\n", s_perfNative ? "native" : "minion", sum);
	minion_msg(pMi, "instrs executed: %d\n", pMi->instrsExecuted);
}


static void cli_opts(int argc, char* argv[]) {
	int i;
	int offs;
	for (i = 1; i < argc; i++) {
		char* pOpt = argv[i];
		size_t len = strlen(pOpt);
		if (len > 2) {
			if (strcmp(pOpt,  "--silent") == 0) {
				minion_set_silent(1);
			} else if (strcmp(pOpt,  "--bin-info") == 0) {
				s_binInfo = 1;
			} else if (strcmp(pOpt,  "--exec-dbg") == 0) {
				s_execDbg = 1;
			} else if (strcmp(pOpt,  "--no-exec-dbg") == 0) {
				s_execDbg = 0;
			}  else if (strcmp(pOpt,  "--dbg-fregs") == 0) {
				s_gbgFregs = 1;
			} else if (strcmp(pOpt,  "--echo-instrs") == 0) {
				s_echoInstrs = 1;
			} else if (strcmp(pOpt,  "--no-echo-instrs") == 0) {
				s_echoInstrs = 0;
			} else if (strcmp(pOpt,  "--bin-mem") == 0) {
				s_binMem = 1;
			} else if (strcmp(pOpt,  "--bin-file") == 0) {
				s_binMem = 0;
			} else if ((offs = opt_prefix(pOpt, "--bin-path=")) > 0) {
				s_pBinPath = pOpt + offs;
			} else if ((offs = opt_prefix(pOpt, "--test=")) > 0) {
				s_pTestName = pOpt + offs;
			} else if (strcmp(pOpt,  "--perf-native") == 0) {
				s_perfNative = 1;
			} else if ((offs = opt_prefix(pOpt, "--perf-count=")) > 0) {
				s_perfCount = atoi(pOpt + offs);
			}
		}
	}
}

int main(int argc, char* argv[]) {
	MINION_BIN miBin;
	MINION mi;
	s_pBinPath = "out/test.minion";
	s_pTestName = "fib";
	cli_opts(argc, argv);
	memset(&miBin, 0, sizeof(miBin));
	memset(&mi, 0, sizeof(mi));
	if (s_binMem) {
		s_pBinData = bin_load(s_pBinPath, &s_binDataSize);
		minion_sys_msg("Loading binary via memory, path: \"%s\", p: %p, size = 0x%X \n", s_pBinPath, s_pBinData, s_binDataSize);
		minion_bin_from_mem(&miBin, s_pBinData, s_binDataSize);
	} else {
		minion_sys_msg("Loading binary from file, path: \"%s\"\n", s_pBinPath);
		minion_bin_load(&miBin, s_pBinPath);
	}
	if (s_binInfo) {
		minion_bin_info(&miBin);
	}
	minion_init(&mi, &miBin);

	if (mi.codeOrg > 0) {
		if (strcmp(s_pTestName,  "disasm") == 0) {
			test_func_dump(&mi);
		} else if (strcmp(s_pTestName,  "inner_mem") == 0) {
			test_inner_mem(&mi);
		} else if (strcmp(s_pTestName,  "mapped_mem") == 0) {
			test_mapped_mem(&mi);
		} else if (strcmp(s_pTestName,  "fib") == 0) {
			test_fib(&mi);
		} else if (strcmp(s_pTestName,  "2op_s") == 0) {
			test_f_2op_s(&mi);
		} else if (strcmp(s_pTestName,  "fcvt") == 0) {
			test_fcvt(&mi);
		} else if (strcmp(s_pTestName,  "sin_s") == 0) {
			test_sin_s(&mi);
		} else if (strcmp(s_pTestName,  "cos_s") == 0) {
			test_cos_s(&mi);
		} else if (strcmp(s_pTestName,  "1op_s") == 0) {
			test_f_1op_s(&mi);
		} else if (strcmp(s_pTestName,  "perf_sincos_s") == 0) {
			perf_sincos_s(&mi);
		}
	} else {
		minion_sys_err("Corrupted minion!\n");
	}

	minion_bin_free(&miBin);
	minion_release(&mi);

	return 0;
}

