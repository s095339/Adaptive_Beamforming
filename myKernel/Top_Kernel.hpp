#ifndef __KERNEL_QRF__
#define __KERNEL_QRF__

#include "hls_stream.h"

#include "dut_type.hpp"

const int A_ROWS = QRF_A_ROWS;
const int A_COLS = QRF_A_COLS;
const bool TRANSPOSED_Q = QRF_TRANSPOSED_Q;

extern "C" void Top_Kernel(MATRIX_IN_T* matrixA,
                            MATRIX_OUT_T* matrixR);
#endif


