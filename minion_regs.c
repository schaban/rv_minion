/* minion: rv32g ISA simulator */
/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2023 Sergey Chaban <sergey.chaban@gmail.com> */

static int s_altRegNames = 1;

const char* minion_get_reg_name(int reg) {
	const char* pName;
	static const char* nameTbl[] = {
		"x0", "zero",
		"x1", "ra",
		"x2", "sp",
		"x3", "gp",
		"x4", "tp",
		"x5", "t0",
		"x6", "t1",
		"x7", "t2",
		"x8", "s0",
		"x9", "s1",
		"x10", "a0",
		"x11", "a1",
		"x12", "a2",
		"x13", "a3",
		"x14", "a4",
		"x15", "a5",
		"x16", "a6",
		"x17", "a7",
		"x18", "s2",
		"x19", "s3",
		"x20", "s4",
		"x21", "s5",
		"x22", "s6",
		"x23", "s7",
		"x24", "s8",
		"x25", "s9",
		"x26", "s10",
		"x27", "s11",
		"x28", "t3",
		"x29", "t4",
		"x30", "t5",
		"x31", "t6"
	};
	if (reg >= 0 && reg <= 31) {
		pName = nameTbl[reg*2 + (s_altRegNames ? 1 : 0)];
	} else {
		pName = "<invalid-reg>";
	}
	return pName;
}

const char* minion_get_f_reg_name(int reg) {
	const char* pName;
	static const char* nameTbl[] = {
		"f0", "ft0",
		"f1", "ft1",
		"f2", "ft2",
		"f3", "ft3",
		"f4", "ft4",
		"f5", "ft5",
		"f6", "ft6",
		"f7", "ft7",
		"f8", "fs0",
		"f9", "fs1",
		"f10", "fa0",
		"f11", "fa1",
		"f12", "fa2",
		"f13", "fa3",
		"f14", "fa4",
		"f15", "fa5",
		"f16", "fa6",
		"f17", "fa7",
		"f18", "fs2",
		"f19", "fs3",
		"f20", "fs4",
		"f21", "fs5",
		"f22", "fs6",
		"f23", "fs7",
		"f24", "fs8",
		"f25", "fs9",
		"f26", "fs10",
		"f27", "fs11",
		"f28", "ft8",
		"f29", "ft9",
		"f30", "ft10",
		"f31", "ft11"
	};
	if (reg >= 0 && reg <= 31) {
		pName = nameTbl[reg*2 + (s_altRegNames ? 1 : 0)];
	} else {
		pName = "<invalid-freg>";
	}
	return pName;
}

void minion_set_ra(MINION* pMi, uint32_t ra) { pMi->regs[1] = ra; }
uint32_t minion_get_ra(MINION* pMi) { return pMi->regs[1]; }

void minion_set_sp(MINION* pMi, uint32_t sp) { pMi->regs[2] = sp; }
uint32_t minion_get_sp(MINION* pMi) { return pMi->regs[2]; }

void minion_set_gp(MINION* pMi, uint32_t gp) { pMi->regs[3] = gp; }
uint32_t minion_get_gp(MINION* pMi) { return pMi->regs[3]; }


int32_t minion_get_a0(MINION* pMi) { return pMi->regs[10]; }
void minion_set_a0(MINION* pMi, int32_t val) { pMi->regs[10] = val; }
int32_t minion_get_a1(MINION* pMi) { return pMi->regs[11]; }
void minion_set_a1(MINION* pMi, int32_t val) { pMi->regs[11] = val; }

int32_t minion_get_t0(MINION* pMi) { return pMi->regs[5]; }
void minion_set_t0(MINION* pMi, int32_t val) { pMi->regs[5] = val; }
int32_t minion_get_t1(MINION* pMi) { return pMi->regs[6]; }
void minion_set_t1(MINION* pMi, int32_t val) { pMi->regs[6] = val; }
int32_t minion_get_t2(MINION* pMi) { return pMi->regs[7]; }
void minion_set_t2(MINION* pMi, int32_t val) { pMi->regs[7] = val; }
int32_t minion_get_t3(MINION* pMi) { return pMi->regs[28]; }
void minion_set_t3(MINION* pMi, int32_t val) { pMi->regs[28] = val; }
int32_t minion_get_t4(MINION* pMi) { return pMi->regs[29]; }
void minion_set_t4(MINION* pMi, int32_t val) { pMi->regs[29] = val; }
int32_t minion_get_t5(MINION* pMi) { return pMi->regs[30]; }
void minion_set_t5(MINION* pMi, int32_t val) { pMi->regs[30] = val; }
int32_t minion_get_t6(MINION* pMi) { return pMi->regs[31]; }
void minion_set_t6(MINION* pMi, int32_t val) { pMi->regs[31] = val; }


float minion_get_freg_s(MINION* pMi, int no) {
	float* p = (float*)&pMi->fregs[no & 0x1F];
	return *p;
}

void minion_set_freg_s(MINION* pMi, int no, float val) {
	float* p = (float*)&pMi->fregs[no & 0x1F];
	*p = val;
}

float minion_get_fa0_s(MINION* pMi) { return minion_get_freg_s(pMi, 10); }
void minion_set_fa0_s(MINION* pMi, float val) { minion_set_freg_s(pMi, 10, val); }
float minion_get_fa1_s(MINION* pMi) { return minion_get_freg_s(pMi, 11); }
void minion_set_fa1_s(MINION* pMi, float val) { minion_set_freg_s(pMi, 11, val); }


double minion_get_freg_d(MINION* pMi, int no) {
	return pMi->fregs[no & 0x1F];
}

void minion_set_freg_d(MINION* pMi, int no, double val) {
	pMi->fregs[no & 0x1F] = val;
}

double minion_get_fa0_d(MINION* pMi) { return minion_get_freg_d(pMi, 10); }
void minion_set_fa0_d(MINION* pMi, double val) { minion_set_freg_d(pMi, 10, val); }
double minion_get_fa1_d(MINION* pMi) { return minion_get_freg_d(pMi, 11); }
void minion_set_fa1_d(MINION* pMi, double val) { minion_set_freg_d(pMi, 11, val); }


void minion_dump_regs(MINION* pMi) {
	int i;
	if (!pMi) return;
	for (i = 0; i < 32; ++i) {
		minion_msg(pMi, "%s: 0x%08X (%d)\n", minion_get_reg_name(i), pMi->regs[i], pMi->regs[i]);
	}
}

void minion_dump_fregs_s(MINION* pMi) {
	int i;
	if (!pMi) return;
	for (i = 0; i < 32; ++i) {
		minion_msg(pMi, "%s: %f\n", minion_get_f_reg_name(i), minion_get_freg_s(pMi, i));
	}
}

void minion_dump_fregs_d(MINION* pMi) {
	int i;
	if (!pMi) return;
	for (i = 0; i < 32; ++i) {
		minion_msg(pMi, "%s: %f\n", minion_get_f_reg_name(i), minion_get_freg_d(pMi, i));
	}
}

