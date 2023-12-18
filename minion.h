/* minion: rv32g ISA simulator */
/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2023 Sergey Chaban <sergey.chaban@gmail.com> */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MINION_TSTR_SIZE 4096

#define MINION_VPTR_TAG 0xDA000000
#define MINION_VPTR_TAG_MASK 0xFF000000
#define MINION_VPTR_BITS 20

#define MINION_PC_NATIVE 0xD00D0000

#define MINION_IMODE_EXEC (1 << 0)
#define MINION_IMODE_ECHO (1 << 1)

#define MINION_PCSTATUS_JAL    (1)
#define MINION_PCSTATUS_JALR   (1 << 1)
#define MINION_PCSTATUS_RET    (1 << 2)
#define MINION_PCSTATUS_BR     (1 << 3)
#define MINION_PCSTATUS_NATIVE (1 << 31)

typedef struct _MINION_FUNC_INFO {
	const char* pName;
	uint32_t addr;
	uint32_t size;
} MINION_FUNC_INFO;

typedef struct _MINION_MEM_MAP {
	void* p;
	uint32_t size;
	uint32_t vptr;
} MINION_MEM_MAP;

typedef struct _MINION_BIN {
	int version;
	uint32_t codeOrg;
	uint32_t dataOrg;
	uint32_t sdataOrg;
	uint32_t gpIni;
	uint32_t binSize;
	int nfuncs;
	void* pBinMem;
	char* pNameMem;
	MINION_FUNC_INFO* pFuncs;
	char tmpStr[MINION_TSTR_SIZE];
} MINION_BIN;

typedef struct _MINION {
	void* pBinMem;
	MINION_FUNC_INFO* pFuncs;
	uint32_t codeOrg;
	uint32_t binSize;
	int nfuncs;

	int32_t regs[32];
	double fregs[32];
	uint32_t pc;
	uint32_t pcStatus;
	uint32_t fcsr;
	uint32_t instrsExecuted;
	void* pStkMem;
	void* pUser;
	void (*ecall_fn)(struct _MINION*);
	void (*ebreak_fn)(struct _MINION*);
	void (*aext_fn)(struct _MINION*, uint32_t op, int rd, int rs1, int rs2, uint32_t instr, uint32_t mode);
	void (*bext_logic_fn)(struct _MINION*, int rd, int rs1, int rs2, uint32_t instr, uint32_t mode);
	MINION_MEM_MAP memMap[16];
} MINION;

void minion_err(MINION* pMi, const char* pFmt, ...);
void minion_msg(MINION* pMi, const char* pFmt, ...);
void minion_sys_err(const char* pFmt, ...);
void minion_sys_msg(const char* pFmt, ...);
void minion_reset_sp(MINION* pMi);
int minion_valid_pc(MINION* pMi);
int minion_is_mapped_vptr(uint32_t vptr);
void* minion_resolve_vptr(MINION* pMi, uint32_t vptr);
uint32_t minion_mem_map(MINION* pMi, void* p, uint32_t size);
void minion_mem_unmap(MINION* pMi, uint32_t vptr);
int minion_find_func(MINION* pMi, const char* pFnName);
int minion_valid_func_idx(MINION* pMi, int ifn);
void minion_set_pc_to_func_idx(MINION* pMi, int ifn);
void minion_set_pc_to_func(MINION* pMi, const char* pFnName);
uint32_t minion_get_func_instr_count(MINION* pMi, const char* pFnName);
int minion_bin_find_func(MINION_BIN* pBin, const char* pFnName);
void minion_bin_from_mem(MINION_BIN* pBin, void* pMem, size_t memSize);
void minion_bin_load(MINION_BIN* pBin, const char* pPath);
void minion_bin_free(MINION_BIN* pBin);
void minion_bin_info(MINION_BIN* pBin);
void minion_init(MINION* pMi, MINION_BIN* pBin);
void minion_release(MINION* pMi);
void minion_set_silent(int flg);
void minion_enable_alt_regnames(int flg);
void minion_enable_alt_mnemonics(int flg);

void minion_instr(MINION* pMi, uint32_t instr, uint32_t mode);
uint32_t minion_fetch_pc_instr(MINION* pMi);

void minion_set_ra(MINION* pMi, uint32_t ra);
uint32_t minion_get_ra(MINION* pMi);

void minion_set_sp(MINION* pMi, uint32_t sp);
uint32_t minion_get_sp(MINION* pMi);

void minion_set_gp(MINION* pMi, uint32_t gp);
uint32_t minion_get_gp(MINION* pMi);

int32_t minion_get_a0(MINION* pMi);
void minion_set_a0(MINION* pMi, int32_t val);
int32_t minion_get_a1(MINION* pMi);
void minion_set_a1(MINION* pMi, int32_t val);
int32_t minion_get_a2(MINION* pMi);
void minion_set_a2(MINION* pMi, int32_t val);
int32_t minion_get_a3(MINION* pMi);
void minion_set_a3(MINION* pMi, int32_t val);
int32_t minion_get_a4(MINION* pMi);
void minion_set_a4(MINION* pMi, int32_t val);
int32_t minion_get_a5(MINION* pMi);
void minion_set_a5(MINION* pMi, int32_t val);
int32_t minion_get_a6(MINION* pMi);
void minion_set_a6(MINION* pMi, int32_t val);
int32_t minion_get_a7(MINION* pMi);
void minion_set_a7(MINION* pMi, int32_t val);

int32_t minion_get_t0(MINION* pMi);
void minion_set_t0(MINION* pMi, int32_t val);
int32_t minion_get_t1(MINION* pMi);
void minion_set_t1(MINION* pMi, int32_t val);
int32_t minion_get_t2(MINION* pMi);
void minion_set_t2(MINION* pMi, int32_t val);
int32_t minion_get_t3(MINION* pMi);
void minion_set_t3(MINION* pMi, int32_t val);
int32_t minion_get_t4(MINION* pMi);
void minion_set_t4(MINION* pMi, int32_t val);
int32_t minion_get_t5(MINION* pMi);
void minion_set_t5(MINION* pMi, int32_t val);
int32_t minion_get_t6(MINION* pMi);
void minion_set_t6(MINION* pMi, int32_t val);

float minion_get_freg_s(MINION* pMi, int no);
void minion_set_freg_s(MINION* pMi, int no, float val);

float minion_get_fa0_s(MINION* pMi);
void minion_set_fa0_s(MINION* pMi, float val);
float minion_get_fa1_s(MINION* pMi);
void minion_set_fa1_s(MINION* pMi, float val);

double minion_get_freg_d(MINION* pMi, int no);
void minion_set_freg_d(MINION* pMi, int no, double val);

double minion_get_fa0_d(MINION* pMi);
void minion_set_fa0_d(MINION* pMi, double val);
double minion_get_fa1_d(MINION* pMi);
void minion_set_fa1_d(MINION* pMi, double val);

void minion_dump_regs(MINION* pMi);
void minion_dump_fregs_s(MINION* pMi);
void minion_dump_fregs_d(MINION* pMi);

#ifdef __cplusplus
}
#endif

