/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include "xf_solver_L2.hpp"
#include "xf_solver_L1.hpp"
//#include "utils/x_matrix_utils.hpp"
#include <iostream>
#include <stdio.h>
#include "hls_stream.h"
#include "hls_x_complex.h"
#include "utils/std_complex_utils.h"
#include <complex>
#include "dut_type.hpp"
struct my_qrf_traits : xf::solver::qrfTraits {
    static const int ARCH = 1;//SEL_ARCH;
};

void Master2Stream(MATRIX_IN_T* matrixA,
	  hls::stream<MATRIX_IN_T>& matrixAStrm,
	  const unsigned int rowA,
    const unsigned int colA) 
{

  for(unsigned int i = 0; i < rowA*colA; i++) {
    #pragma HLS LOOP_TRIPCOUNT min = rowA*colA max = rowA*colA
    matrixAStrm.write(matrixA[i]);
  }
}

void QRF(   hls::stream<MATRIX_IN_T>& matrixAStrm,
	        hls::stream<MATRIX_OUT_T>& matrixQStrm,
            hls::stream<MATRIX_OUT_T>& matrixRStrm
	    ) 
      
{
    xf::solver::qrf<0, 100, 10, MATRIX_IN_T, MATRIX_OUT_T, my_qrf_traits>(matrixAStrm, matrixQStrm,matrixRStrm);
}

void qrf_transpose(     
  hls::stream<MATRIX_OUT_T>& matrixQStrm,
	hls::stream<MATRIX_OUT_T>& matrixRStrm,
	//hls::x_complex<double>* matrixQ,
    MATRIX_OUT_T matrixR_trans_conj[][10],
    const unsigned int rowQ,
    const unsigned int colQ,
    const unsigned int rowR,
    const unsigned int colR
    ) 
{
 
  MATRIX_OUT_T tempQ, tempR,temp;

  unsigned int r,c;
  r=0;
  c=0;
  for(unsigned int i = 0; i < rowR*colR; i++) {
    #pragma HLS PIPELINE
    #pragma HLS LOOP_TRIPCOUNT min = rowR*colR max = rowR*colR

    c = i%(colR);
    r = i/(colR);
    //std::cout<<"r="<<r<<";c="<<c<<std::endl;
    tempR = matrixRStrm.read();
    
    //tempR.imag(100);
    //tempR = tempR.conj();

    //轉製共軛--------------------------
    if(i < 100){//if i < 100.
      temp = std::conj(tempR);
      matrixR_trans_conj[c][r] = (r>c)? 0:temp;
      //std::cout<<temp<<std::endl;
    }
    //-----------------------------------------
  }
  
  for(unsigned int i=0; i<rowQ*colQ; i++){
    //clean the Q stream
    #pragma HLS LOOP_TRIPCOUNT min = rowQ*colQ max = rowQ*colQ
    #pragma HLS PIPELINE
    tempQ = matrixQStrm.read();
  }
}


void pass_dataflow(
    MATRIX_IN_T* matrixA,
    //hls::x_complex<double>* matrixQ,
    MATRIX_OUT_T* matrixR,
    const unsigned int rowA,
    const unsigned int colA,
    const unsigned int rowQ,
    const unsigned int colQ,
    const unsigned int rowR,
    const unsigned int colR
    ) 
{
  //#pragma HLS DATAFLOW

  //std::cout<< "0" <<std::endl;
  static hls::stream<MATRIX_IN_T> matrixAStrm;
  static hls::stream<MATRIX_OUT_T> matrixQStrm;
  static hls::stream<MATRIX_OUT_T> matrixRStrm;
  MATRIX_OUT_T matrixR_trans_conj[10][10];  //10X10 matrix


  Master2Stream(matrixA, matrixAStrm, rowA, colA);
  

  QRF(matrixAStrm,  matrixQStrm, matrixRStrm);
  

  qrf_transpose(matrixQStrm,matrixRStrm,matrixR_trans_conj, 
                 rowQ, colQ, rowR, colR);

  unsigned int k=0;
  for(unsigned int r=0; r<10; r++){
    for(unsigned int c=0; c<10; c++){
      matrixR[k] = matrixR_trans_conj[r][c];
      //std::cout<<matrixR[k]<<std::endl;
      k++;
    }
  }
  //std::cout<< "4" <<std::endl;
}



extern "C" void Top_Kernel(
    MATRIX_IN_T matrixA[1000],
    //hls::x_complex<double> matrixQ[100*100],
    MATRIX_OUT_T matrixR[100]
    ) 
{
// extern "C" void kernel_geqrf_0(double dataA[MA*NA], double tau[NA]) {
//#pragma HLS INTERFACE axis port = matrixAStrm
//#pragma HLS INTERFACE axis port = matrixQStrm 
//#pragma HLS INTERFACE axis port = matrixRStrm 
#pragma HLS INTERFACE m_axi port = matrixA bundle = gmem0 offset = slave depth = 1000
//#pragma HLS INTERFACE m_axi port = matrixQ bundle = gmem1 offset = slave num_read_outstanding = 16 max_read_burst_length = \
//    32
#pragma HLS TNTERFACE m_axi port = matrixR bundle = gmem1 offset = slave depth = 100


//#pragma HLS INTERFACE s_axilite port = matrixA bundle = control
//#pragma HLS INTERFACE s_axilite port = matrixQ bundle = control
//#pragma HLS INTERFACE s_axilite port = matrixR bundle = control

//#pragma HLS INTERFACE s_axilite port = return bundle = control

//xf::solver::qrf<0, 100, 10, hls::x_complex<double>, hls::x_complex<double>, my_qrf_traits>(matrixAStrm, matrixQStrm,matrixRStrm);
const unsigned int rowA = 100;
const unsigned int colA = 10;
const unsigned int rowQ = 100;
const unsigned int colQ = 100;
const unsigned int rowR = 100;
const unsigned int colR = 10;
/*
int k=0;
for (int r = 0; r < rowA; r++) {
  for (int c = 0; c < colA; c++) {
      
      std::cout << matrixA[k] << std::endl;
      k++;
  }
}
*/
pass_dataflow(
    matrixA,
    //matrixQ,
    matrixR,
    rowA, 
    colA,
    rowQ, 
    colQ, 
    rowR, 
    colR
    );
}
/*
extern "C" void kernel_geqrf_0(hls::x_complex<double>* dataA, hls::x_complex<double>* tau) {
// extern "C" void kernel_geqrf_0(double dataA[MA*NA], double tau[NA]) {
#pragma HLS INTERFACE m_axi port = dataA bundle = gmem0 offset = slave num_read_outstanding = \
    16 max_read_burst_length = 32
#pragma HLS INTERFACE m_axi port = tau bundle = gmem1 offset = slave num_read_outstanding = 16 max_read_burst_length = \
    32

#pragma HLS INTERFACE s_axilite port = dataA bundle = control
#pragma HLS INTERFACE s_axilite port = tau bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    //#pragma HLS data_pack variable=dataA
    //#pragma HLS data_pack variable=tau

    // matrix initialization
    hls::x_complex<double> dataA_2D[MA][NA];

    // Matrix transform from 1D to 2D
    int k = 0;
    for (int i = 0; i < MA; ++i) {
        for (int j = 0; j < NA; ++j) {
#pragma HLS pipeline
            dataA_2D[i][j] = dataA[k];
            k++;
        }
    }

    // Calling for QR
    xf::solver::geqrf<hls::x_complex<double>, MA, NA, NCU>(MA, NA, dataA_2D, NA, tau);

    // Matrix transform from 2D to 1D
    k = 0;
    for (int i = 0; i < MA; ++i) {
        for (int j = 0; j < NA; ++j) {
#pragma HLS pipeline
            dataA[k] = dataA_2D[i][j];
            k++;
        }
    }
}
*/


