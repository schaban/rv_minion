/* minion: rv32g ISA simulator */
/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2023 Sergey Chaban <sergey.chaban@gmail.com> */

#include "minion.h"

static int s_silentFlg = 0;


static const char* skip_space(const char* pStr) {
	while (1) {
		char c = *pStr;
		if (!c) return pStr;
		if (c != ' ') return pStr;
		pStr++;
	}
}

static const char* skip_chars(const char* pStr) {
	while (1) {
		char c = *pStr;
		if (!c) return pStr;
		if (c == ' ' || c == '\n') return pStr;
		pStr++;
	}
}

static const char* ck_str_cmd(const char* pStr, const char* pCmd) {
	const char* pArg = NULL;
	if (pStr && pCmd) {
		size_t lstr = strlen(pStr);
		size_t lcmd = strlen(pCmd);
		if (lstr >= lcmd) {
			if (memcmp(pStr, pCmd, lcmd) == 0) {
				pArg = skip_space(pStr + lcmd);
			}
		}
	}
	return pArg;
}

static uint32_t u32_hex(const char* pStr, size_t len) {
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

void minion_err(MINION* pMi, const char* pFmt, ...) {
	va_list argLst;
	va_start(argLst, pFmt);
	if (!s_silentFlg) vfprintf(stderr, pFmt, argLst);
	va_end(argLst);
}

void minion_msg(MINION* pMi, const char* pFmt, ...) {
	va_list argLst;
	va_start(argLst, pFmt);
	if (!s_silentFlg) vfprintf(stdout, pFmt, argLst);
	va_end(argLst);
}

void minion_sys_err(const char* pFmt, ...) {
	va_list argLst;
	va_start(argLst, pFmt);
	if (!s_silentFlg) vfprintf(stderr, pFmt, argLst);
	va_end(argLst);
}

void minion_sys_msg(const char* pFmt, ...) {
	va_list argLst;
	va_start(argLst, pFmt);
	if (!s_silentFlg) vfprintf(stdout, pFmt, argLst);
	va_end(argLst);
}

int minion_valid_pc(MINION* pMi) {
	int res = 0;
	if (pMi) {
		res = pMi->pc >= pMi->codeOrg;
	}
	return res;
}

int minion_is_mapped_vptr(uint32_t vptr) {
	return ((vptr & MINION_VPTR_TAG_MASK) == MINION_VPTR_TAG);
}

void* minion_resolve_vptr(MINION* pMi, uint32_t vptr) {
	void* p = NULL;
	if (vptr < pMi->codeOrg && vptr > 4) {
		p = pMi->pStkMem;
		if (p)  {
			p = (uint8_t*)p + vptr;
		}
	} else if (minion_is_mapped_vptr(vptr)) {
		int vidx = (vptr >> MINION_VPTR_BITS) & 0xF;
		if (pMi->memMap[vidx].size > 0) {
			uint32_t offs = vptr - pMi->memMap[vidx].vptr;
			if (offs < pMi->memMap[vidx].size) {
				p = (uint8_t*)pMi->memMap[vidx].p + offs;
			}
		}
	} else if (vptr >= pMi->codeOrg) {
		uint32_t offs = vptr - pMi->codeOrg;
		if (offs < pMi->binSize) {
			p = (uint8_t*)pMi->pBinMem + offs;
		}
	}
	return p;
}

#include "minion_regs.c"
#include "minion_instrs.c"


uint32_t minion_mem_map(MINION* pMi, void* p, uint32_t size) {
	uint32_t vptr = 0;
	if (size > 0x10000) {
		minion_err(pMi, "can't create memory map, size = 0x%X\n", size);
	} else {
		int i;
		int idx = -1;
		for (i = 0; i < 16; i++) {
			if (pMi->memMap[i].vptr == 0) {
				idx = i;
			}
			break;
		}
		if (idx < 0) {
			minion_err(pMi, "can't map memory, no free slots\n");
		} else {
			vptr = MINION_VPTR_TAG | (idx << MINION_VPTR_BITS);
			pMi->memMap[idx].p = p;
			pMi->memMap[idx].size = size;
			pMi->memMap[idx].vptr = vptr;
		}
	}
	return vptr;
}

void minion_mem_unmap(MINION* pMi, uint32_t vptr) {
	if (minion_is_mapped_vptr(vptr)) {
		int idx = (vptr >> MINION_VPTR_BITS) & 0xF;
		pMi->memMap[idx].p = NULL;
		pMi->memMap[idx].size = 0;
		pMi->memMap[idx].vptr = 0;
	} else {
		minion_err(pMi, "can't umap memory, invalid vptr\n");
	}
}

int minion_find_func(MINION* pMi, const char* pFnName) {
	int i;
	if (pMi && pFnName && pMi->pFuncs) {
		for (i = 0; i < pMi->nfuncs; i++) {
			if (strcmp(pFnName, pMi->pFuncs[i].pName) == 0) {
				return i;
			}
		}
	}
	return -1;
}

int minion_valid_func_idx(MINION* pMi, int ifn) {
	return pMi && ifn >= 0 && ifn < pMi->nfuncs;
}

void minion_set_pc_to_func_idx(MINION* pMi, int ifn) {
	if (minion_valid_func_idx(pMi, ifn)) {
		pMi->pc = pMi->pFuncs[ifn].addr;
	}
}

void minion_set_pc_to_func(MINION* pMi, const char* pFnName) {
	int ifn = minion_find_func(pMi, pFnName);
	minion_set_pc_to_func_idx(pMi, ifn);
}

uint32_t minion_get_func_instr_count(MINION* pMi, const char* pFnName) {
	uint32_t n = 0;
	int ifn = minion_find_func(pMi, pFnName);
	if (ifn >= 0) {
		n = pMi->pFuncs[ifn].size >> 2;
	}
	return n;
}


void minion_bin_read(MINION_BIN* pBin, FILE* pFile) {
	int i;
	size_t nameMemSize = 0;
	long funcsLstOffs = 0;
	char* pStr = pBin ? fgets(pBin->tmpStr, MINION_TSTR_SIZE, pFile) : NULL;
	const char* pVer = ck_str_cmd(pStr, "MINION");
	if (!pVer) {
		minion_sys_err("Not a MINION!\n");
		return;
	}
	pBin->version = atoi(pVer);

	while (1) {
		const char* pArg;
		pStr = fgets(pBin->tmpStr, MINION_TSTR_SIZE, pFile);
		pArg = ck_str_cmd(pStr, "$code");
		if ((pArg = ck_str_cmd(pStr, "$code"))) {
			pBin->codeOrg = u32_hex(pArg, skip_chars(pArg) - pArg);
		} else if ((pArg = ck_str_cmd(pStr, "$data"))) {
			pBin->dataOrg = u32_hex(pArg, skip_chars(pArg) - pArg);
		} else if ((pArg = ck_str_cmd(pStr, "$sdata"))) {
			pBin->sdataOrg = u32_hex(pArg, skip_chars(pArg) - pArg);
		} else if ((pArg = ck_str_cmd(pStr, "$gp"))) {
			pBin->gpIni = u32_hex(pArg, skip_chars(pArg) - pArg);
		} else if ((pArg = ck_str_cmd(pStr, "$bin"))) {
			size_t nread;
			pBin->binSize = (uint32_t)atoi(pArg);
			pBin->pBinMem = malloc(pBin->binSize);
			nread = fread(pBin->pBinMem, 1, pBin->binSize, pFile);
			if (nread != pBin->binSize) {
				minion_sys_err("Incomplete binary part!\n");
			}
		} else if ((pArg = ck_str_cmd(pStr, "$funcs"))) {
			pBin->nfuncs = atoi(pArg);
			pBin->pFuncs = (MINION_FUNC_INFO*)malloc(pBin->nfuncs * sizeof(MINION_FUNC_INFO));
			funcsLstOffs = ftell(pFile);
			for (i = 0; i < pBin->nfuncs; i++) {
				uint32_t funcAddr;
				uint32_t funcSize;
				const char* pSizeArg;
				const char* pNameArg;
				uint32_t nameSize;
				pStr = fgets(pBin->tmpStr, MINION_TSTR_SIZE, pFile);
				pSizeArg = skip_space(skip_chars(pStr));
				pNameArg = skip_space(skip_chars(pSizeArg));
				funcAddr = u32_hex(pStr, skip_chars(pStr) - pStr);
				funcSize = atoi(pSizeArg);
				nameSize = (uint32_t)(skip_chars(pNameArg) - pNameArg);
				nameMemSize += nameSize + 1;
				pBin->pFuncs[i].addr = funcAddr;
				pBin->pFuncs[i].size = funcSize;
			}
		} else {
			break;
		}
	}

	if (pBin->nfuncs && nameMemSize && funcsLstOffs > 0) {
		char* pNameMem;
		fseek(pFile, funcsLstOffs, SEEK_SET);
		pBin->pNameMem = (char*)malloc(nameMemSize);
		memset(pBin->pNameMem, 0, nameMemSize);
		pNameMem = pBin->pNameMem;
		for (i = 0; i < pBin->nfuncs; i++) {
			const char* pSizeArg;
			const char* pNameArg;
			uint32_t nameSize;
			pStr = fgets(pBin->tmpStr, MINION_TSTR_SIZE, pFile);
			pSizeArg = skip_space(skip_chars(pStr));
			pNameArg = skip_space(skip_chars(pSizeArg));
			nameSize = (uint32_t)(skip_chars(pNameArg) - pNameArg);
			memcpy(pNameMem, pNameArg, nameSize);
			pBin->pFuncs[i].pName = pNameMem;
			pNameMem += nameSize + 1;
		}
	}
}

void minion_bin_info(MINION_BIN* pBin) {
	int i;
	if (!pBin) return;
	minion_sys_msg("ver: %d\n", pBin->version);
	minion_sys_msg("code: %X\n", pBin->codeOrg);
	minion_sys_msg("data: %X\n", pBin->dataOrg);
	minion_sys_msg("sdata: %X\n", pBin->sdataOrg);
	minion_sys_msg("gp: %X\n", pBin->gpIni);
	minion_sys_msg("nfuncs: %d\n", pBin->nfuncs);
	minion_sys_msg("binSize: %d (0x%X)\n", pBin->binSize, pBin->binSize);
	minion_sys_msg("pBinMem: %p\n", pBin->pBinMem);
	if (pBin->pFuncs) {
		for (i = 0; i < pBin->nfuncs; i++) {
			uint32_t fnSize = pBin->pFuncs[i].size;
			minion_sys_msg("func[%d]: %s @ %X, size=%d (0x%X, nins=%d)\n",
			       i, pBin->pFuncs[i].pName, pBin->pFuncs[i].addr, fnSize, fnSize, fnSize/4);
		}
	}
}

void minion_bin_load(MINION_BIN* pBin, const char* pPath) {
	FILE* pFile = fopen(pPath, "r+b");
	minion_bin_read(pBin, pFile);
	fclose(pFile);
}

void minion_bin_free(MINION_BIN* pBin) {
	if (!pBin) return;
	if (pBin->pFuncs) {
		free(pBin->pFuncs);
	}
	if (pBin->pNameMem) {
		free(pBin->pNameMem);
	}
	if (pBin->pBinMem) {
		free(pBin->pBinMem);
	}
	memset(pBin, 0, sizeof(MINION_BIN));
}

void minion_init(MINION* pMi, MINION_BIN* pBin) {
	if (!pMi) return;
	if (!pBin) return;

	pMi->pBinMem = pBin->pBinMem;
	pMi->codeOrg = pBin->codeOrg;
	pMi->binSize = pBin->binSize;
	pMi->nfuncs = pBin->nfuncs;
	pMi->pFuncs = pBin->pFuncs;

	if (pMi->codeOrg) {
		size_t stkSize = pMi->codeOrg;
		pMi->pStkMem = malloc(stkSize);
		memset(pMi->pStkMem, 0, stkSize);
	}

	minion_set_ra(pMi, MINION_PC_NATIVE);
	minion_set_sp(pMi, pMi->codeOrg);
	minion_set_gp(pMi, pBin->gpIni);
}

void minion_release(MINION* pMi) {
	if (!pMi) return;
	if (pMi->pStkMem) {
		free(pMi->pStkMem);
	}
	memset(pMi, 0, sizeof(MINION));
}

void minion_set_silent(int flg) {
	s_silentFlg = flg;
}

