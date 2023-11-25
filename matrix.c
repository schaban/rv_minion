int mtx_invert_s(float* pMtx, const int N, int* pWk /* [N*3] */) {
	int i, j, k;
	int* pPiv = pWk;
	int* pCol = pWk + N;
	int* pRow = pWk + N*2;
	int ir = 0;
	int ic = 0;
	int rc;
	float piv;
	float ipiv;
	for (i = 0; i < N; ++i) {
		pPiv[i] = 0;
	}
	for (i = 0; i < N; ++i) {
		float amax = 0.0f;
		for (j = 0; j < N; ++j) {
			if (pPiv[j] != 1) {
				int rj = j * N;
				for (k = 0; k < N; ++k) {
					if (0 == pPiv[k]) {
						float a = pMtx[rj + k];
						if (a < 0.0f) a = -a;
						if (a >= amax) {
							amax = a;
							ir = j;
							ic = k;
						}
					}
				}
			}
		}
		++pPiv[ic];
		if (ir != ic) {
			int rr = ir * N;
			int rc = ic * N;
			for (j = 0; j < N; ++j) {
				float t = pMtx[rr + j];
				pMtx[rr + j] = pMtx[rc + j];
				pMtx[rc + j] = t;
			}
		}
		pRow[i] = ir;
		pCol[i] = ic;
		rc = ic * N;
		piv = pMtx[rc + ic];
		if (piv == 0) return 0; /* singular */
		ipiv = 1.0f / piv;
		pMtx[rc + ic] = 1;
		for (j = 0; j < N; ++j) {
			pMtx[rc + j] *= ipiv;
		}
		for (j = 0; j < N; ++j) {
			if (j != ic) {
				float* pDst;
				float* pSrc;
				int rj = j * N;
				float d = pMtx[rj + ic];
				pMtx[rj + ic] = 0;
				pDst = &pMtx[rj];
				pSrc = &pMtx[rc];
				for (k = 0; k < N; ++k) {
					*pDst++ -= *pSrc++ * d;
				}
			}
		}
	}
	for (i = N; --i >= 0;) {
		ir = pRow[i];
		ic = pCol[i];
		if (ir != ic) {
			for (j = 0; j < N; ++j) {
				int rj = j * N;
				float t = pMtx[rj + ir];
				pMtx[rj + ir] = pMtx[rj + ic];
				pMtx[rj + ic] = t;
			}
		}
	}
	return 1;
}

void mtx_mul_s(float* pDst, const float* pSrc1, const float* pSrc2, const int M, const int N, const int P) {
	int i, j, k;
	for (i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		float s = pSrc1[ra];
		for (k = 0; k < P; ++k) {
			pDst[rr + k] = pSrc2[k] * s;
		}
	}
	for (i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		for (j = 1; j < N; ++j) {
			int rb = j * P;
			float s = pSrc1[ra + j];
			for (k = 0; k < P; ++k) {
				pDst[rr + k] += pSrc2[rb + k] * s;
			}
		}
	}
}
