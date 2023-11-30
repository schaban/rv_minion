typedef int (*sort_cmpfn)(const void*, const void*);

static void sort_swap_bytes(uint8_t* pA, uint8_t* pB, size_t n) {
	uint8_t t;
	do {
		t = *pA;
		*pA++ = *pB;
		*pB++ = t;
		--n;
	} while (n != 0);
}

typedef struct _SORT_WK {
	sort_cmpfn cmpfn;
	size_t elemSize;
} SORT_WK;

static uint8_t* sort_pivot(SORT_WK* pWk, uint8_t* p, size_t n) {
	if (n > 10) {
		size_t j = pWk->elemSize * (n / 6);
		uint8_t* pI = p + j;
		uint8_t* pJ = pI + j * 2;
		uint8_t* pK = pI + j * 4;
		if (pWk->cmpfn(pI, pJ) < 0) {
			if (pWk->cmpfn(pI, pK) < 0) {
				if (pWk->cmpfn(pJ, pK) < 0) {
					return pJ;
				}
				return pK;
			}
			return pI;
		}
		if (pWk->cmpfn(pJ, pK) < 0) {
			if (pWk->cmpfn(pI, pK) < 0) {
				return pI;
			}
			return pK;
		}
		return pJ;
	}
	return p + (pWk->elemSize * (n / 2));
}


static void sort_sub(SORT_WK* pWk, uint8_t* p, size_t n) {
	size_t s = pWk->elemSize;
	while (n > 1) {
		size_t j;
		uint8_t* pJ;
		uint8_t* pN;
		uint8_t* pI = sort_pivot(pWk, p, n);
		sort_swap_bytes(p, pI, s);
		pI = p;
		pN = p + (n * s);
		pJ = pN;
		while (1) {
			do {
				pI += s;
			} while (pI < pN && pWk->cmpfn(pI, p) < 0);
			do {
				pJ -= s;
			} while (pJ > p && pWk->cmpfn(pJ, p) > 0);
			if (pJ < pI) {
				break;
			}
			sort_swap_bytes(pI, pJ, s);
		}
		sort_swap_bytes(p, pJ, s);
		j = (size_t)(pJ - p) / s;
		n -= j + 1;
		if (j >= n) {
			sort_sub(pWk, p, j);
			p += (j + 1) * s;
		} else {
			sort_sub(pWk, p + (j + 1) * s, n);
			n = j;
		}
	}
}

static int sort_cmpfn_f32(const void* pA, const void* pB) {
	float* pVal1 = (float*)pA;
	float* pVal2 = (float*)pB;
	float v1 = *pVal1;
	float v2 = *pVal2;
	if (v1 > v2) return 1;
	if (v1 < v2) return -1;
	return 0;
}

void sort_f32(float* pFloats, size_t num) {
	SORT_WK wk;
	if (!pFloats) return;
	if (num < 2) return;
	wk.cmpfn = sort_cmpfn_f32;
	wk.elemSize = sizeof(float);
	sort_sub(&wk, (uint8_t*)pFloats, num);
}

static int sort_cmpfn_i32(const void* pA, const void* pB) {
	int32_t* pVal1 = (int32_t*)pA;
	int32_t* pVal2 = (int32_t*)pB;
	int32_t v1 = *pVal1;
	int32_t v2 = *pVal2;
	if (v1 > v2) return 1;
	if (v1 < v2) return -1;
	return 0;
}

void sort_i32(int32_t* pInts, size_t num) {
	SORT_WK wk;
	if (!pInts) return;
	if (num < 2) return;
	wk.cmpfn = sort_cmpfn_i32;
	wk.elemSize = sizeof(int32_t);
	sort_sub(&wk, (uint8_t*)pInts, num);
}

static int sort_cmpfn_i64(const void* pA, const void* pB) {
	int64_t* pVal1 = (int64_t*)pA;
	int64_t* pVal2 = (int64_t*)pB;
	int64_t v1 = *pVal1;
	int64_t v2 = *pVal2;
	if (v1 > v2) return 1;
	if (v1 < v2) return -1;
	return 0;
}

void sort_i64(int64_t* pInts, size_t num) {
	SORT_WK wk;
	if (!pInts) return;
	if (num < 2) return;
	wk.cmpfn = sort_cmpfn_i64;
	wk.elemSize = sizeof(int64_t);
	sort_sub(&wk, (uint8_t*)pInts, num);
}
