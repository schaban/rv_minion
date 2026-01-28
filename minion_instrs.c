/* minion: rv32g ISA simulator */
/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2023 Sergey Chaban <sergey.chaban@gmail.com> */

static int s_altMnemonics = 1;

static void dispatch_F(MINION* pMi, uint32_t instr, uint32_t mode);
static void dispatch_M(MINION* pMi, uint32_t instr, uint32_t mode);

static int get_rd(uint32_t instr) {
	return (instr >> 7) & 0x1F;
}

static int get_rs1(uint32_t instr) {
	return (instr >> 15) & 0x1F;
}

static int get_rs2(uint32_t instr) {
	return (instr >> 20) & 0x1F;
}

static int get_rs3(uint32_t instr) {
	return (instr >> 27) & 0x1F;
}

static int get_funct3(uint32_t instr) {
	return (instr >> 12) & 7;
}

static int get_funct7(uint32_t instr) {
	return (instr >> 25) & 0x7F;
}

static int32_t get_I_imm(uint32_t instr) {
	return ((int32_t)instr) >> 20;
}

static uint32_t get_U_imm(uint32_t instr) {
	return instr >> 12;
}

static int32_t get_S_imm(uint32_t instr) {
	int32_t imm0 = (instr >> 7) & 0x1F;
	int32_t imm1 = ((int32_t)instr >> 20) & ~0x1F;
	int32_t imm = imm0 | imm1;
	return imm;
}

static int32_t get_UJ_imm(uint32_t instr) {
	int32_t imm = (instr >> (31 - 20)) & (1 << 20);
	imm |= (instr >> (21 - 1)) & (0x3FF << 1);
	imm |= (instr >> (20 - 11)) & (1 << 11);
	imm |= instr & (0xFF << 12);
	imm <<= (31-20);
	imm >>= (31-20);
	return imm;
}

static int32_t get_SB_imm(uint32_t instr) {
	int32_t imm = (instr >> (31 - 12)) & (1 << 12);
	imm |= (instr >> (25 - 5)) & (0x3F << 5);
	imm |= (instr >> (8 - 1)) & (0xF << 1);
	imm |= (instr & (1 << 7)) << (11 - 7);
	imm <<= (31-12);
	imm >>= (31-12);
	return imm;
}

typedef int32_t (*ARITH_OP)(int32_t s1, int32_t s2);

static int32_t arith_add(int32_t s1, int32_t s2) { return s1 + s2; }
static int32_t arith_sub(int32_t s1, int32_t s2) { return s1 - s2; }
static int32_t arith_sll(int32_t s1, int32_t s2) { return s1 << s2; }
static int32_t arith_slt(int32_t s1, int32_t s2) { return s1 < s2 ? 1 : 0; }
static int32_t arith_sltu(int32_t s1, int32_t s2) { return (uint32_t)s1 < (uint32_t)s2 ? 1 : 0; }
static int32_t arith_xor(int32_t s1, int32_t s2) { return s1 ^ s2; }
static int32_t arith_srl(int32_t s1, int32_t s2) { return (uint32_t)s1 >> s2; }
static int32_t arith_sra(int32_t s1, int32_t s2) { return s1 >> s2; }
static int32_t arith_or(int32_t s1, int32_t s2) { return s1 | s2; }
static int32_t arith_and(int32_t s1, int32_t s2) { return s1 & s2; }

static void arith_rs1_rs2(MINION* pMi, int rd, int rs1, int rs2, ARITH_OP op) {
	if (!op) return;
	if (rd > 0 && rd < 32) {
		int32_t s1 = pMi->regs[rs1];
		int32_t s2 = pMi->regs[rs2];
		pMi->regs[rd] = op(s1, s2);
	}
}

static void arith_R(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int fn3 = get_funct3(instr);
	int fn7 = get_funct7(instr);
	const char* pOpName = "<invalid_arith_R>";
	ARITH_OP opFunc = NULL;

	if (fn7 == 1) {
		dispatch_M(pMi, instr, mode);
		return;
	}

	switch (fn3) {
		case 0:
			if (fn7 == 0) {
				pOpName = "add";
				opFunc = arith_add;
			} else if (fn7 == 0x20) {
				pOpName = "sub";
				opFunc = arith_sub;
			}
			break;
		case 1:
			pOpName = "sll";
			opFunc = arith_sll;
			break;
		case 2:
			pOpName = "slt";
			opFunc = arith_slt;
			break;
		case 3:
			pOpName = "sltu";
			opFunc = arith_sltu;
			break;
		case 4:
			pOpName = "xor";
			opFunc = arith_xor;
			break;
		case 5:
			if (fn7 == 0) {
				pOpName = "srl";
				opFunc = arith_srl;
			} else if (fn7 == 0x20) {
				pOpName = "sra";
				opFunc = arith_sra;
			}
			break;
		case 6:
			pOpName = "or";
			opFunc = arith_or;
			break;
		case 7:
			pOpName = "and";
			opFunc = arith_and;
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s  %s, %s, %s\n",
		           pMi->pc, instr, pOpName,
		           minion_get_reg_name(rd),
		           minion_get_reg_name(rs1),
		           minion_get_reg_name(rs2)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		arith_rs1_rs2(pMi, rd, rs1, rs2, opFunc);
	}
}

static void arith_rs1_imm(MINION* pMi, int rd, int rs1, int32_t imm, ARITH_OP op) {
	if (!op) return;
	if (rd > 0 && rd < 32) {
		int32_t rs1val = pMi->regs[rs1];
		pMi->regs[rd] = op(rs1val, imm);
	}
}

static void arith_I(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int imm = get_I_imm(instr);
	int fn3 = get_funct3(instr);
	int fn7 = get_funct7(instr);
	const char* pOpName = "<invalid_arith_I>";
	ARITH_OP opFunc = NULL;
	int isMV = 0;
	int isLI = 0;
	int isNOP = 0;

	switch (fn3) {
		case 0:
			pOpName = "addi";
			if (s_altMnemonics) {
				isNOP = (rd == 0) & (rs1 == 0);
				isLI = (rs1 == 0);
				isMV = (imm == 0) & (!isLI);
			}
			opFunc = arith_add;
			break;
		case 1:
			pOpName = "slli";
			opFunc = arith_sll;
			break;
		case 2:
			pOpName = "slti";
			opFunc = arith_slt;
			break;
		case 3:
			pOpName = "sltiu";
			opFunc = arith_sltu;
			break;
		case 4:
			pOpName = "xori";
			opFunc = arith_xor;
			break;
		case 5:
			if (fn7 == 0) {
				pOpName = "srli";
				opFunc = arith_srl;
			} else if (fn7 == 0x20) {
				pOpName = "srai";
				opFunc = arith_sra;
			}
			break;
		case 6:
			pOpName = "ori";
			opFunc = arith_or;
			break;
		case 7:
			pOpName = "andi";
			opFunc = arith_and;
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		if (isNOP) {
			minion_msg(pMi, "%08X: %08X  nop\n", pMi->pc, instr);
		} else if (isMV) {
			minion_msg(pMi, "%08X: %08X  mv   %s, %s\n",
				pMi->pc, instr,
				minion_get_reg_name(rd),
				minion_get_reg_name(rs1)
			);
		} else if (isLI) {
			minion_msg(pMi, "%08X: %08X  li   %s, %d\n",
				pMi->pc, instr,
				minion_get_reg_name(rd),
				imm
			);
		} else {
			minion_msg(pMi, "%08X: %08X  %s %s, %s, %d\n",
		           pMi->pc, instr, pOpName,
		           minion_get_reg_name(rd),
		           minion_get_reg_name(rs1),
		           imm
			);
		}
	}

	if (mode & MINION_IMODE_EXEC) {
		arith_rs1_imm(pMi, rd, rs1, imm, opFunc);
	}
}

static void dispatch_A(MINION* pMi, uint32_t instr, uint32_t mode) {
	if (pMi->aext_fn) {
		uint32_t op = instr >> 27;
		int rd = get_rd(instr);
		int rs1 = get_rs1(instr);
		int rs2 = get_rs2(instr);
		pMi->aext_fn(pMi, op, rd, rs1, rs2, instr, mode);
	}
}

static void sys_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int32_t imm = get_I_imm(instr);
	const char* pOpName = "<sys>";
	if (imm == 0) {
		pOpName = "ecall";
	} else if (imm == 1) {
		pOpName = "ebreak";
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s\n",
		       pMi->pc, instr,
		       pOpName
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		if (imm == 0) {
			if (pMi->ecall_fn) {
				pMi->ecall_fn(pMi);
			}
		} else if (imm == 1) {
			if (pMi->ebreak_fn) {
				pMi->ebreak_fn(pMi);
			}
		}
	}
}

static void load_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int32_t imm = get_I_imm(instr);
	int fn3 = get_funct3(instr);
	const char* pOpName = "<load>";
	int32_t size = 0;

	switch (fn3) {
		case 0:
			pOpName = "lb";
			size = -1;
			break;
		case 1:
			pOpName = "lh";
			size = -2;
			break;
		case 2:
			pOpName = "lw";
			size = 4;
			break;
		case 4:
			pOpName = "lbu";
			size = 1;
			break;
		case 5:
			pOpName = "lhu";
			size = 2;
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s   %s,%d(%s)\n",
		           pMi->pc, instr, pOpName,
		           minion_get_reg_name(rd),
		           imm,
		           minion_get_reg_name(rs1)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		if (rd != 0 && size != 0) {
			void* pNativeSrc = minion_resolve_vptr(pMi, pMi->regs[rs1] + imm);
			if (pNativeSrc) {
				switch (size) {
					case -1:
						pMi->regs[rd] = *(int8_t*)pNativeSrc;
						break;
					case -2:
						pMi->regs[rd] = *(int16_t*)pNativeSrc;
						break;
					case 1:
						pMi->regs[rd] = *(uint8_t*)pNativeSrc;
						break;
					case 2:
						pMi->regs[rd] = *(uint16_t*)pNativeSrc;
						break;
					case 4:
						pMi->regs[rd] = *(int32_t*)pNativeSrc;
						break;
				}
			}
		}
	}
}

static void store_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int32_t imm = get_S_imm(instr);
	int fn3 = get_funct3(instr);
	const char* pOpName = "<store>";
	int32_t size = 0;

	switch (fn3) {
		case 0:
			pOpName = "sb";
			size = 1;
			break;
		case 1:
			pOpName = "sh";
			size = 2;
			break;
		case 2:
			pOpName = "sw";
			size = 4;
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s   %s,%d(%s)\n",
		           pMi->pc, instr, pOpName,
		           minion_get_reg_name(rs2),
		           imm,
		           minion_get_reg_name(rs1)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		if (size > 0) {
			void* pNativeDst = minion_resolve_vptr(pMi, pMi->regs[rs1] + imm);
			if (pNativeDst) {
				void* pRegSrc = &pMi->regs[rs2];
				memcpy(pNativeDst, pRegSrc, size);
			}
		}
	}
}

static void branch_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int fn3 = get_funct3(instr);
	int32_t imm = get_SB_imm(instr);

	if (mode & MINION_IMODE_ECHO) {
		const char* pCmpName = "<?>";
		switch (fn3) {
			case 0:
				pCmpName = "eq";
				break;
			case 1:
				pCmpName = "ne";
				break;
			case 4:
				pCmpName = "lt";
				break;
			case 5:
				pCmpName = "ge";
				break;
			case 6:
				pCmpName = "ltu";
				break;
			case 7:
				pCmpName = "geu";
				break;
		}
		minion_msg(pMi, "%08X: %08X  b%s  %s, %s, %X\n", pMi->pc, instr,
		           pCmpName,
		           minion_get_reg_name(rs1),
		           minion_get_reg_name(rs2),
		           pMi->pc + imm
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		int32_t s1 = pMi->regs[rs1];
		int32_t s2 = pMi->regs[rs2];
		int cmpRes = 0;
		switch (fn3) {
			case 0:
				cmpRes = (s1 == s2);
				break;
			case 1:
				cmpRes = (s1 != s2);
				break;
			case 4:
				cmpRes = (s1 < s2);
				break;
			case 5:
				cmpRes = (s1 >= s2);
				break;
			case 6:
				cmpRes = ((uint32_t)s1 < (uint32_t)s2);
				break;
			case 7:
				cmpRes = ((uint32_t)s1 >= (uint32_t)s2);
				break;
		}
		if (cmpRes) {
			int32_t newPC = pMi->pc + imm;
			pMi->pc = newPC;
			pMi->pcStatus |= MINION_PCSTATUS_BR;
		}
	}
}

static void jalr_op(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int32_t imm = get_I_imm(instr);
	int isRet = (instr == 0x8067);

	if (mode & MINION_IMODE_ECHO) {
		if (isRet && s_altMnemonics) {
			minion_msg(pMi, "%08X: %08X  ret\n", pMi->pc, instr);
		} else {
			minion_msg(pMi, "%08X: %08X  jalr  %s, %s, %d\n", pMi->pc, instr,
		           minion_get_reg_name(rd),
		           minion_get_reg_name(rs1),
		           imm
			);
		}
	}

	if (mode & MINION_IMODE_EXEC) {
		int32_t newPC = pMi->regs[rs1] + imm;
		if (rd != 0) {
			pMi->regs[rd] = pMi->pc + 4;
		}
		pMi->pc = newPC;
		pMi->pcStatus |= MINION_PCSTATUS_JALR;
		if (isRet) {
			pMi->pcStatus |= MINION_PCSTATUS_RET;
		}
	}
}

static void jal_op(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int32_t imm = get_UJ_imm(instr);
	if (mode & MINION_IMODE_ECHO) {
		if (s_altMnemonics && rd == 0) {
			minion_msg(pMi, "%08X: %08X  j    %X\n", pMi->pc, instr, pMi->pc + imm);
		} else {
			minion_msg(pMi, "%08X: %08X  jal  %s, %X\n", pMi->pc, instr,
		               minion_get_reg_name(rd), pMi->pc + imm);
		}
	}

	if (mode & MINION_IMODE_EXEC) {
		int32_t newPC = pMi->pc + imm;
		if (rd != 0) {
			pMi->regs[rd] = pMi->pc + 4;
		}
		pMi->pc = newPC;
		pMi->pcStatus |= MINION_PCSTATUS_JAL;
	}
}

static void fence_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
}

static void invalid_op(MINION* pMi, uint32_t instr, uint32_t mode) {
	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  <invalid>\n", pMi->pc, instr);
	}
}

static void lui_op(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	uint32_t imm = get_U_imm(instr);
	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  lui  %s, 0x%X\n", pMi->pc, instr,
		           minion_get_reg_name(rd), imm
		);
	}
	if (mode & MINION_IMODE_EXEC) {
		if (rd != 0) {
			pMi->regs[rd] = imm << 12;
		}
	}
}

static void auipc_op(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	uint32_t imm = get_U_imm(instr);
	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  auipc %s, 0x%X\n", pMi->pc, instr,
		           minion_get_reg_name(rd), imm
		);
	}
	if (mode & MINION_IMODE_EXEC) {
		if (rd != 0) {
			pMi->regs[rd] = pMi->pc + (imm << 12);
		}
	}
}

static void f_load_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int imm = get_I_imm(instr);
	int fn3 = get_funct3(instr);
	const char* pOpName = "<f-load?>";
	size_t size = 0;

	switch (fn3) {
		case 2:
			pOpName = "flw";
			size = sizeof(float);
			break;
		case 3:
			pOpName = "fld";
			size = sizeof(double);
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s   %s,%d(%s)\n",
		           pMi->pc, instr, pOpName,
		           minion_get_f_reg_name(rd),
		           imm,
		           minion_get_reg_name(rs1)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		if (size > 0) {
			void* pNativeSrc = minion_resolve_vptr(pMi, pMi->regs[rs1] + imm);
			if (pNativeSrc) {
				memcpy(&pMi->fregs[rd], pNativeSrc, size);
			}
		}
	}
}

static void f_store_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int imm = get_S_imm(instr);
	int fn3 = get_funct3(instr);
	const char* pOpName = "<f-sore?>";
	size_t size = 0;

	switch (fn3) {
		case 2:
			pOpName = "fsw";
			size = sizeof(float);
			break;
		case 3:
			pOpName = "fsd";
			size = sizeof(double);
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s   %s,%d(%s)\n",
		           pMi->pc, instr, pOpName,
		           minion_get_f_reg_name(rs2),
		           imm,
		           minion_get_reg_name(rs1)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		if (size > 0) {
			void* pNativeDst = minion_resolve_vptr(pMi, pMi->regs[rs1] + imm);
			if (pNativeDst) {
				memcpy(pNativeDst, &pMi->fregs[rs2], size);
			}
		}
	}
}

static void f_calc_common_s(MINION* pMi, uint32_t instr, uint32_t mode) {
	uint32_t op = instr >> 27;
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int fn3 = get_funct3(instr);

	if (mode & MINION_IMODE_ECHO) {
		int fmt = -1;
		const char* pOpName = NULL;
		switch (op) {
			case 0:
				pOpName = "fadd.s";
				fmt = 0;
				break;
			case 1:
				pOpName = "fsub.s";
				fmt = 0;
				break;
			case 2:
				pOpName = "fmul.s";
				fmt = 0;
				break;
			case 3:
				pOpName = "fdiv.s";
				fmt = 0;
				break;
			case 4:
				if (0 == fn3) {
					pOpName = "fsgnj.s";
					if (s_altMnemonics && (rs1 == rs2)) {
						pOpName = "fmv.s";
						fmt = 1;
					} else {
						fmt = 0;
					}
				} else if (1 == fn3) {
					pOpName = "fsgnjn.s";
					if (s_altMnemonics && (rs1 == rs2)) {
						pOpName = "fneg.s";
						fmt = 1;
					} else {
						fmt = 0;
					}
				} else if (2 == fn3) {
					pOpName = "fsgnjx.s";
					if (s_altMnemonics && (rs1 == rs2)) {
						pOpName = "fabs.s";
						fmt = 1;
					} else {
						fmt = 0;
					}
				}
				break;
			case 5:
				if (0 == fn3) {
					pOpName = "fmin.s";
				} else if (1 == fn3) {
					pOpName = "fmax.s";
				}
				fmt = 0;
				break;
			case 8:
				pOpName = "fcvt.s.d";
				fmt = 1;
				break;
			case 0xB:
				pOpName = "fsqrt.s";
				break;
			case 0x14:
				if (0 == fn3) {
					pOpName = "fle.s";
				} else if (1 == fn3) {
					pOpName = "flt.s";
				} else if (2 == fn3) {
					pOpName = "feq.s";
				}
				fmt = 2;
				break;
			case 0x18:
				if (rs2 == 0) {
					pOpName = "fcvt.w.s";
				} else {
					pOpName = "fcvt.wu.s";
				}
				fmt = 3;
				break;
			case 0x1A:
				if (rs2 == 0) {
					pOpName = "fcvt.s.w";
				} else {
					pOpName = "fcvt.s.wu";
				}
				fmt = 3;
				break;
			case 0x1C:
				if (fn3 == 0) {
					pOpName = "fmv.x.w";
				} else {
					pOpName = "fclass.s";
				}
				fmt = 3;
				break;
			case 0x1E:
				pOpName = "fmv.w.x";
				fmt = 4;
				break;
		}

		if (pOpName) {
			if (fmt < 0) {
				minion_msg(pMi, "%08X: %08X  %s ---\n", pMi->pc, instr, pOpName);
			} else if (0 == fmt) {
				minion_msg(pMi, "%08X: %08X  %s  %s, %s, %s\n", pMi->pc, instr,
				           pOpName,
				           minion_get_f_reg_name(rd),
				           minion_get_f_reg_name(rs1),
				           minion_get_f_reg_name(rs2)
				);
			} else if (1 == fmt) {
				minion_msg(pMi, "%08X: %08X  %s  %s, %s\n", pMi->pc, instr,
				           pOpName,
				           minion_get_f_reg_name(rd),
				           minion_get_f_reg_name(rs1)
				);
			} else if (2 == fmt) {
				minion_msg(pMi, "%08X: %08X  %s  %s, %s, %s\n", pMi->pc, instr,
				           pOpName,
				           minion_get_reg_name(rd),
				           minion_get_f_reg_name(rs1),
				           minion_get_f_reg_name(rs2)
				);
			} else if (3 == fmt) {
				minion_msg(pMi, "%08X: %08X  %s  %s, %s\n", pMi->pc, instr,
				           pOpName,
				           minion_get_reg_name(rd),
				           minion_get_f_reg_name(rs1)
				);
			} else if (4 == fmt) {
				minion_msg(pMi, "%08X: %08X  %s  %s, %s\n", pMi->pc, instr,
				           pOpName,
				           minion_get_f_reg_name(rd),
				           minion_get_reg_name(rs1)
				);
			} else {
				minion_msg(pMi, "%08X: %08X  %s fmt%d\n", pMi->pc, instr, pOpName, fmt);
			}
		} else {
			minion_msg(pMi, "%08X: %08X !! <f-common-S>\n", pMi->pc, instr);
		}
	}

	if (mode & MINION_IMODE_EXEC) {
		float val1;
		float val2;
		float res;
		const uint32_t smsk = (1U << 31);
		switch (op) {
			case 0:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				res = val1 + val2;
				minion_set_freg_s(pMi, rd, res);
				break;
			case 1:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				res = val1 - val2;
				minion_set_freg_s(pMi, rd, res);
				break;
			case 2:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				res = val1 * val2;
				minion_set_freg_s(pMi, rd, res);
				break;
			case 3:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				res = val2 != 0.0f ? (val1 / val2) : 0.0f;
				minion_set_freg_s(pMi, rd, res);
				break;
			case 4:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				if (0 == fn3) {
					if (rs1 == rs2) {
						/* fmv.s */
						minion_set_freg_s(pMi, rd, val1);
					} else {
						/* fsgnj.s */
						*((uint32_t*)&val1) &= ~smsk;
						*((uint32_t*)&val1) |= *((uint32_t*)&val2) & smsk;
						minion_set_freg_s(pMi, rd, val1);
					}
				} else if (1 == fn3) {
					/* fsgnjn.s */
					*((uint32_t*)&val1) &= ~smsk;
					*((uint32_t*)&val1) |= (*((uint32_t*)&val2) & smsk) ^ smsk;
					minion_set_freg_s(pMi, rd, val1);
				} else if (2 == fn3) {
					/* fsgnjx.s" */
					uint32_t sgn = *((uint32_t*)&val1) & smsk;
					sgn ^= *((uint32_t*)&val2) & smsk;
					*((uint32_t*)&val1) &= ~smsk;
					*((uint32_t*)&val1) |= sgn;
					minion_set_freg_s(pMi, rd, val1);
				}
				break;
			case 5:
				if (0 == fn3) {
					val1 = minion_get_freg_s(pMi, rs1);
					val2 = minion_get_freg_s(pMi, rs2);
					res = val1 < val2 ? val1 : val2;
				} else if (1 == fn3) {
					val1 = minion_get_freg_s(pMi, rs1);
					val2 = minion_get_freg_s(pMi, rs2);
					res = val1 > val2 ? val1 : val2;
				}
				minion_set_freg_s(pMi, rd, res);
				break;
			case 8:
				res = (float)minion_get_freg_d(pMi, rs1);
				minion_set_freg_s(pMi, rd, res);
				break;
			case 0xB:
				val1 = minion_get_freg_s(pMi, rs1);
				res = sqrtf(val1);
				minion_set_freg_s(pMi, rd, res);
				break;
			case 0x14:
				val1 = minion_get_freg_s(pMi, rs1);
				val2 = minion_get_freg_s(pMi, rs2);
				if (0 == fn3) {
					/* fle.s */
					pMi->regs[rd] = (val1 <= val2);
				} else if (1 == fn3) {
					/* flt.s */
					pMi->regs[rd] = (val1 < val2);
				} else if (2 == fn3) {
					/* feq.s" */
					pMi->regs[rd] = (val1 == val2);
				}
				break;
			case 0x18:
				val1 = minion_get_freg_s(pMi, rs1);
				if (rs2 == 0) {
					/* fcvt.w.s */
					pMi->regs[rd] = (int32_t)val1;
				} else {
					/* fcvt.wu.s */
					pMi->regs[rd] = (uint32_t)val1;
				}
				break;
			case 0x1A:
				if (rs2 == 0) {
					/* fcvt.s.w */
					res = (float)((int32_t)pMi->regs[rs1]);
				} else {
					/* fcvt.s.wu */
					res = (float)((int32_t)pMi->regs[rs1]);
				}
				minion_set_freg_s(pMi, rd, res);
				break;
			case 0x1C:
				val1 = minion_get_freg_s(pMi, rs1);
				if (fn3 == 0) {
					/* fmv.x.w */
					memcpy(&pMi->regs[rd], &val1, 4);
				} else {
					/* fclass.s */
					minion_err(pMi, "!! %08X: %08X unimplemented fclass-S>\n", pMi->pc, instr);
				}
				break;
			case 0x1E:
				/* fmv.w.x */
				memcpy(&res, &pMi->regs[rs1], 4);
				minion_set_freg_s(pMi, rd, res);
				break;
			default:
				minion_err(pMi, "!! %08X: %08X unhandled <f-common-S>\n", pMi->pc, instr);
				break;
		}
	}
}

static void f_calc_common_d(MINION* pMi, uint32_t instr, uint32_t mode) {
	minion_msg(pMi, "%08X: %08X  !f-common-D!\n", pMi->pc, instr);
}

static void f_calc_common(MINION* pMi, uint32_t instr, uint32_t mode) {
	int s = (instr >> 25) & 3;
	if (s == 0) {
		f_calc_common_s(pMi, instr, mode);
	} else if (s == 1) {
		f_calc_common_d(pMi, instr, mode);
	}
}

static void f_calc_fused_s(MINION* pMi, uint32_t instr, uint32_t mode) {
	uint32_t op = (instr >> 2) & 7;
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int rs3 = get_rs3(instr);

	if (mode & MINION_IMODE_ECHO) {
		const char* pOpName = "<f-fused-S>";
		switch (op) {
			case 0:
				pOpName = "fmadd.s";
				break;
			case 1:
				pOpName = "fmsub.s";
				break;
			case 2:
				pOpName = "fnmsub.s";
				break;
			case 3:
				pOpName = "fnmadd.s";
				break;
		}
		minion_msg(pMi, "%08X: %08X  %s  %s, %s, %s, %s\n", pMi->pc, instr,
		           pOpName,
		           minion_get_f_reg_name(rd),
		           minion_get_f_reg_name(rs1),
		           minion_get_f_reg_name(rs2),
		           minion_get_f_reg_name(rs3)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		float val1 = minion_get_freg_s(pMi, rs1);
		float val2 = minion_get_freg_s(pMi, rs2);
		float val3 = minion_get_freg_s(pMi, rs3);
		float res;
		switch (op) {
			case 0:
				/* fmadd.s */
				res = val1*val2 + val3;
				break;
			case 1:
				/* fmsub.s */
				res = val1*val2 - val3;
				break;
			case 2:
				/* fnmsub.s */
				res = -(val1*val2 - val3);
				break;
			case 3:
				/* fnmadd.s */
				res = -(val1*val2 + val3);
				break;
		}
		minion_set_freg_s(pMi, rd, res);
	}
}

static void f_calc_fused_d(MINION* pMi, uint32_t instr, uint32_t mode) {
	minion_msg(pMi, "%08X: %08X  !f-fused-D!\n", pMi->pc, instr);
}

static void f_calc_fused(MINION* pMi, uint32_t instr, uint32_t mode) {
	int s = (instr >> 25) & 3;
	if (s == 0) {
		f_calc_fused_s(pMi, instr, mode);
	} else if (s == 1) {
		f_calc_fused_d(pMi, instr, mode);
	}
}

static void f_sys_ops(MINION* pMi, uint32_t instr, uint32_t mode) {
}

static void dispatch_F(MINION* pMi, uint32_t instr, uint32_t mode) {
	uint32_t op = instr & 0x7F;
	uint32_t op1 = (op >> 2) & 7;
	uint32_t op2 = (op >> 5) & 3;

	switch (op2) {
		case 0:
			f_load_ops(pMi, instr, mode);
			break;
		case 1:
			f_store_ops(pMi, instr, mode);
			break;
		case 2:
			if (op1 == 4) {
				f_calc_common(pMi, instr, mode);
			} else {
				f_calc_fused(pMi, instr, mode);
			}
			break;
		case 3:
			f_sys_ops(pMi, instr, mode);
			break;
	}

}

static int32_t m_mul(int32_t s1, int32_t s2) {
	return (int32_t)((int64_t)s1 * (int64_t)s2);
}

static int32_t m_mulh(int32_t s1, int32_t s2) {
	return (int32_t)(((int64_t)s1 * (int64_t)s2) >> 32);
}

static int32_t m_mulhsu(int32_t s1, int32_t s2) {
	return (int32_t)(((int64_t)s1 * (uint64_t)s2) >> 32);
}

static int32_t m_mulhu(int32_t s1, int32_t s2) {
	return (int32_t)(((uint64_t)s1 * (uint64_t)s2) >> 32);
}

static int32_t m_div(int32_t s1, int32_t s2) {
	if (s2 == 0) return 0;
	return s1 / s2;
}

static int32_t m_divu(int32_t s1, int32_t s2) {
	if (s2 == 0) return 0;
	return (uint32_t)s1 / (uint32_t)s2;
}

static int32_t m_rem(int32_t s1, int32_t s2) {
	if (s2 == 0) return 0;
	return s1 % s2;
}

static int32_t m_remu(int32_t s1, int32_t s2) {
	if (s2 == 0) return 0;
	return (uint32_t)s1 % (uint32_t)s2;
}

static void dispatch_M(MINION* pMi, uint32_t instr, uint32_t mode) {
	int rd = get_rd(instr);
	int rs1 = get_rs1(instr);
	int rs2 = get_rs2(instr);
	int fn3 = get_funct3(instr);
	const char* pOpName = "<M.ext>";
	ARITH_OP opFunc = NULL;

	switch (fn3) {
		case 0:
			pOpName = "mul";
			opFunc = m_mul;
			break;
		case 1:
			pOpName = "mulh";
			opFunc = m_mulh;
			break;
		case 2:
			pOpName = "mulhsu";
			opFunc = m_mulhsu;
			break;
		case 3:
			pOpName = "mulhu";
			opFunc = m_mulhu;
			break;
		case 4:
			pOpName = "div";
			opFunc = m_div;
			break;
		case 5:
			pOpName = "divu";
			opFunc = m_divu;
			break;
		case 6:
			pOpName = "rem";
			opFunc = m_rem;
			break;
		case 7:
			pOpName = "remu";
			opFunc = m_remu;
			break;
	}

	if (mode & MINION_IMODE_ECHO) {
		minion_msg(pMi, "%08X: %08X  %s  %s, %s, %s\n",
		           pMi->pc, instr, pOpName,
		           minion_get_reg_name(rd),
		           minion_get_reg_name(rs1),
		           minion_get_reg_name(rs2)
		);
	}

	if (mode & MINION_IMODE_EXEC) {
		arith_rs1_rs2(pMi, rd, rs1, rs2, opFunc);
	}
}

void minion_instr(MINION* pMi, uint32_t instr, uint32_t mode) {
	uint32_t op = instr & 0x7F;
	uint32_t op1 = (op >> 2) & 7;
	uint32_t op2 = (op >> 5) & 3;
	int is32 = ((instr & 3) == 3) && (op1 != 7);
	if (!is32) {
		if (pMi) {
			minion_err(pMi, "not an rv32g instruction: %X\n", instr);
			pMi->faultFlags |= 1;
			pMi->pcStatus = MINION_PCSTATUS_NATIVE;
		}
		return;
	}
	if (!pMi) {
		minion_err(pMi, "can't proceed without a minion\n");
		return;
	}
	if (pMi->faultFlags != 0) {
		pMi->pcStatus = MINION_PCSTATUS_NATIVE;
		return;
	}
	pMi->pcStatus = 0;

	if (mode & MINION_IMODE_EXEC) {
		if (pMi->pc == MINION_PC_NATIVE) {
			minion_err(pMi, "invalid PC\n");
			return;
		}
	}

	if (op1 == 0)  {
		switch (op2) {
			case 0:
				load_ops(pMi, instr, mode);
				break;
			case 1:
				store_ops(pMi, instr, mode);
				break;
			case 2:
				dispatch_F(pMi, instr, mode);
				break;
			case 3:
				branch_ops(pMi, instr, mode);
				break;
		}
	} else if (op1 == 1)  {
		if (op2 == 3) {
			jalr_op(pMi, instr, mode);
		} else {
			dispatch_F(pMi, instr, mode);
		}
	} else if (op1 == 2)  {
		if (op2 == 2) {
			dispatch_F(pMi, instr, mode);
		} else {
			invalid_op(pMi, instr, mode);
		}
	} else if (op1 == 3)  {
		switch (op2) {
			case 0:
				fence_ops(pMi, instr, mode);
				break;
			case 1:
				dispatch_A(pMi, instr, mode);
				break;
			case 2:
				dispatch_F(pMi, instr, mode);
				break;
			case 3:
				jal_op(pMi, instr, mode);
				break;
		}
	} else if (op1 == 4) {
		switch (op2) {
			case 0:
				arith_I(pMi, instr, mode);
				break;
			case 1:
				arith_R(pMi, instr, mode);
				break;
			case 2:
				dispatch_F(pMi, instr, mode);
				break;
			case 3:
				sys_ops(pMi, instr, mode);
				break;
		}
	} else if (op1 == 5) {
		if (op2 == 0) {
			auipc_op(pMi, instr, mode);
		} else if (op2 == 1) {
			lui_op(pMi, instr, mode);
		} else {
			invalid_op(pMi, instr, mode);
		}
	}

	if ((mode & MINION_IMODE_ECHO)) {
		if (!(mode & MINION_IMODE_EXEC)) {
			pMi->pc += 4;
		}
	}
	if (mode & MINION_IMODE_EXEC) {
		++pMi->instrsExecuted;
		if (0 == pMi->pcStatus) {
			pMi->pc += 4;
		} else {
			if (pMi->pc == MINION_PC_NATIVE) {
				pMi->pcStatus |= MINION_PCSTATUS_NATIVE;
			}
		}
	}
}

uint32_t minion_fetch_pc_instr(MINION* pMi) {
	uint32_t instr = 0;
	if (minion_valid_pc(pMi)) {
		uint32_t binOffs = pMi->pc - pMi->codeOrg;
		if (binOffs < pMi->binSize) {
			memcpy(&instr, ((uint8_t*)pMi->pBinMem) + binOffs, sizeof(uint32_t));
		}
	}
	return instr;
}

