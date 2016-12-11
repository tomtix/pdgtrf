#ifndef TDP_TRSV_H
#define TDP_TRSV_H

#include <stdint.h>
#include "incblas.h"
#include "proc.h"

void tdp_pdtrsv(
    const enum CBLAS_ORDER order, const enum CBLAS_UPLO Uplo,
    const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_DIAG Diag,
    const int64_t N, int64_t b, const double *A,
    const int64_t lda, double *X, const int64_t incX,
    tdp_trf_dist *dist, tdp_proc *proc);


#endif // TDP_TRSV_H
