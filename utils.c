#include <time.h>

static size_t file_size(FILE* pFile) {
	size_t size = 0;
	if (pFile) {
		long fpos = ftell(pFile);
		if (fpos >= 0) {
			if (fseek(pFile, 0, SEEK_END) == 0) {
				long flen = ftell(pFile);
				if (flen >= 0) {
					size = (size_t)flen;
				}
			}
			fseek(pFile, fpos, SEEK_SET);
		}
	}
	return size;
}

static size_t file_read(FILE* pFile, void* pDst, size_t size) {
	size_t nread = 0;
	if (pFile && pDst && size > 0) {
		nread = fread(pDst, 1, size, pFile);
	}
	return nread;
}

static void* bin_load(const char* pPath, size_t* pSize) {
	void* pData = NULL;
	size_t size = 0;
	if (pPath) {
		FILE* pFile = fopen(pPath, "rb");
		if (pFile) {
			size = file_size(pFile);
			if (size > 0) {
				pData = malloc(size);
				if (pData) {
					fseek(pFile, 0, SEEK_SET);
					size = file_read(pFile, pData, size);
				}
			}
			fclose(pFile);
		}
	}
	if (pSize) {
		*pSize = size;
	}
	return pData;
}

static void* load_cstr(const char* pPath, size_t* pSize) {
	void* pData = NULL;
	size_t size = 0;
	if (pPath) {
		FILE* pFile = fopen(pPath, "rb");
		if (pFile) {
			size = file_size(pFile);
			if (size > 0) {
				pData = malloc(size + 1);
				if (pData) {
					fseek(pFile, 0, SEEK_SET);
					size = file_read(pFile, pData, size);
					((uint8_t*)pData)[size] = 0;
				}
			}
			fclose(pFile);
		}
	}
	if (pSize) {
		*pSize = size;
	}
	return pData;
}

static int opt_prefix(const char* pStr, const char* pPre) {
	int offs = -1;
	if (pStr && pPre) {
		size_t lens = strlen(pStr);
		size_t lenp = strlen(pPre);
		if (lens > lenp) {
			if (memcmp(pStr, pPre, lenp) == 0) {
				offs = (int)lenp;
			}
		}
	}
	return offs;
}

static double time_micros() {
	double ms = 0.0f;
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	ms = (double)t.tv_nsec*1.0e-3 + (double)t.tv_sec*1.0e6;
	return ms;
}

static double time_millis() {
	return time_micros() * 1e-3;
}
