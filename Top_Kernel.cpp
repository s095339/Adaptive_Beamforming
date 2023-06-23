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
#include "hw/qrf.hpp"
#include "hw/Weights_CAL_Mul.h"

//#include "utils/x_matrix_utils.hpp"
#include <iostream>
#include <stdio.h>
#include "hls_stream.h"
#include "hls_x_complex.h"
#include "utils/std_complex_utils.h"
#include <complex>
#include "dut_type.hpp"


void A_stream2stream(hls::stream<complex<double>> &A_outstream, hls::stream<complex<double>> &A_instream){
	complex<double> read_value_A_instream;
	for(int i=0;i<M*S;i++){
#pragma HLS PIPELINE
		read_value_A_instream = A_instream.read();
		A_outstream.write(read_value_A_instream);
	}
}

void read_fifo_U(complex<double> local_L[M][M], complex<double> local_U[M][M], hls::stream<complex<double>> &U_instream){

	read_U_loop1:
		for(int i=0;i<M;i++){
			read_U_loop2:
				for(int j=0;j<M;j++){
					#pragma HLS PIPELINE II=1
					complex<double> stream_read_data;
					stream_read_data=U_instream.read();
					local_U[i][j]=stream_read_data;
					local_L[j][i]=conj(stream_read_data);
				}
		}
}

void read_fifo_Vs(complex<double> local_Vs[M], hls::stream<complex<double>> &Vs_instream){

	read_Vs_loop:
		for(int i=0;i<M;i++){
			local_Vs[i]=Vs_instream.read();
		}
}

void CALC_Z(complex<double> z[M], complex<double> local_L[M][M], complex<double> local_Vs[M]){

	complex<double> tmp[M]={complex<double>(0,0)};
	complex<double> ctemp(0,0);

	tmp[0]=local_Vs[0]/local_L[0][0];
	for(int N=1;N<M;N++){
		ctemp=local_Vs[N];
		for(int i=0;i<M;i++){
#pragma HLS PIPELINE
			ctemp-=local_L[N][i]*tmp[i];
			/*if(i==N-1){
				tmp[N] = (local_Vs[N]-ctemp)/local_L[N][N];
				ctemp=complex<double>(0,0);
				break;
			}*/
		}
		tmp[N] = ctemp/local_L[N][N];
		//ctemp=complex<double>(0,0);
	}


	/*
	complex<double> tmp[M]={complex<double>(0,0)};
	complex<double> ctemp(0,0);

	tmp[0]=local_Vs[0]/local_L[0][0];
	for(int N=1;N<M;N++){
		ctemp=local_Vs[N];
		for(int i=0;i<M;i++){
#pragma HLS PIPELINE
			ctemp-=local_L[N][i]*tmp[i];
			if(i==N-1){
				tmp[N] = ctemp/local_L[N][N];
				break;
			}
		}
	}
	*/

index_calz_read:
	for(int i=0;i<M;i++){
		z[i]=tmp[i];
	}
}

void CALC_U(complex<double> u[M], complex<double> local_U[M][M], complex<double> z[M]){

	complex<double> tmp[M]={complex<double>(0,0)};
	complex<double> ctemp(0,0);

	tmp[M-1]=z[M-1]/local_U[M-1][M-1];
	//ctemp=z[M-2];
	for(int N=M-2;N>=0;N--){
		ctemp=z[N];
		for(int i=M-1;i>=0;i--){
#pragma HLS PIPELINE
			ctemp-=local_U[N][i]*tmp[i];
			/*if(i==N+1){
				tmp[N] = (z[N]-ctemp)/local_U[N][N];
				ctemp=complex<double>(0,0);
				break;
			}*/
		}
		tmp[N] = ctemp/local_U[N][N];
		//ctemp=complex<double>(0,0);
	}
/*
	complex<double> tmp[M]={complex<double>(0,0)};
	complex<double> ctemp(0,0);

	tmp[M-1]=z[M-1]/local_U[M-1][M-1];
	//ctemp=z[M-2];
	for(int N=M-2;N>=0;N--){
		ctemp=z[N];
		for(int i=M-1;i>=0;i--){
#pragma HLS PIPELINE
			ctemp-=local_U[N][i]*tmp[i];
			if(i==N+1){
				tmp[N] = ctemp/local_U[N][N];
				break;
			}
		}
	}
	*/

index_calu_read:
	for(int i=0;i<M;i++){
		u[i]=tmp[i];
	}
}


void CALC_Z_SQUARE(complex<double> &z_sqr, complex<double> z[M]){

	complex<double> z_conj[M];
	complex<double> tmp=complex<double>(0,0);
	for(int i=0;i<M;i++){
		z_conj[i]=conj(z[i]);
		tmp+=z_conj[i]*z[i];
	}
	z_sqr=tmp;
}


void inhom(hls::stream<complex<double>> &weights, hls::stream<complex<double>> &U, hls::stream<complex<double>> &Vs, hls::stream<complex<double>> &A_instream, hls::stream<complex<double>> &A_outstream){

//#pragma HLS INTERFACE mode=ap_fifo depth=10 port=weights
//#pragma HLS INTERFACE mode=ap_fifo depth=100 port=U
//#pragma HLS INTERFACE mode=ap_fifo depth=10 port=Vs

//#pragma HLS INTERFACE mode=ap_ctrl_none port=return
//#pragma HLS STREAM depth=100 variable=U
//#pragma HLS STREAM depth=10 variable=weights
//#pragma HLS STREAM depth=10 variable=Vs
/*
#pragma HLS INTERFACE axis port=weights
#pragma HLS INTERFACE axis port=U
#pragma HLS INTERFACE axis port=Vs
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=z_2 bundle=control
#pragma HLS INTERFACE m_axi port=z_2 offset=slave bundle=gmem
*/

complex<double> local_L[M][M];//resource limitation
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=local_L
complex<double> local_U[M][M];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=local_U
complex<double> local_Vs[M];
complex<double> u[M]={complex<double>(0,0)};
complex<double> z[M]={complex<double>(0,0)};
complex<double> z_sqr=complex<double>(0,0);
complex<double> wt[M]={complex<double>(0,0)};;

//#pragma HLS DATAFLOW

/*
read_U_loop1:
	for(int i=0;i<M;i++){
		read_U_loop2:
			for(int j=0;j<M;j++){
				#pragma HLS PIPELINE II=1
				complex<double> stream_read_data;
				stream_read_data=U.read();
				local_U[i][j]=stream_read_data;
				local_L[j][i]=conj(stream_read_data);
			}
	}

read_Vs_loop:
	for(int i=0;i<M;i++){
		local_Vs[i]=Vs.read();
	}
*/

A_stream2stream(A_outstream,A_instream);
read_fifo_U(local_L,local_U,U);
read_fifo_Vs(local_Vs,Vs);

Calc_para:

	CALC_Z(z,local_L,local_Vs);
	CALC_Z_SQUARE(z_sqr,z);
	CALC_U(u,local_U,z);




Weights_write:
	for(int i=0;i<M;i++){
		wt[i]=u[i]/z_sqr;
		weights.write(wt[i]);
	}


}




///////
void Weights_Mul(complex<double> output_result[S], hls::stream<complex<double>> &A_instream, hls::stream<complex<double>> &weights_instream){

//#pragma HLS STREAM depth=1000 variable=A_instream
//#pragma HLS STREAM depth=10 variable=weights_instream
	//ADD STATIC
static	complex<double> A_matrix_w[S][M]={complex<double>(0,0)};
#pragma HLS ARRAY_RESHAPE variable=A_matrix_w type=block factor=2 dim=2
#pragma HLS BIND_STORAGE variable=A_matrix_w type=ram_1wnr

static	complex<double> weights[M]={complex<double>(0,0)};
#pragma HLS ARRAY_RESHAPE variable=weights type=complete
static	complex<double> results[S]={complex<double>(0,0)};
//#pragma HLS ARRAY_PARTITION variable=results type=complete


//read stream A and weights
read_stream_input:
	for(int i=0;i<S;i++){
		for(int j=0;j<M;j++){
#pragma HLS PIPELINE
			A_matrix_w[i][j]=A_instream.read();
			if(i==S-1){
				weights[j]=weights_instream.read();
			}
		}
	}
/*
	for(int i=0;i<M;i++){
#pragma HLS PIPELINE
		for(int j=0;j<S;j++){
			results[j]+=A_matrix_w[j][i]*weights[i];
		}
	}
*/

	for(int i=0;i<S;i++){
#pragma HLS PIPELINE
		for(int j=0;j<M;j++){
			results[i]+=A_matrix_w[i][j]*weights[j];
		}
	}

//burst write to host
burst_write_output:
	for(int i=0;i<S;i++){
#pragma HLS PIPELINE II=1
		output_result[i]=results[i];
	}


}
////////
struct my_qrf_traits : xf::solver::qrfTraits {
    static const int ARCH = 1;//SEL_ARCH;
};

void Master2Stream(MATRIX_IN_T* matrixA,
	  hls::stream<MATRIX_IN_T>& matrixAStrm,
    MATRIX_IN_T* Vs,
    hls::stream<MATRIX_IN_T>& VsStrm_out1
    ) 
{

  for(unsigned int i = 0; i < 1000; i++) {
    #pragma HLS LOOP_TRIPCOUNT min = 1000 max = 1000
    #pragma HLS PIPELINE
    matrixAStrm.write(matrixA[i]);
  }

  for(unsigned int i = 0;i<10;i++){
    #pragma HLS PIPELINE
    VsStrm_out1.write(Vs[i]);
  }
}

void QRF( hls::stream<MATRIX_IN_T>& matrixAStrm,
          hls::stream<MATRIX_OUT_T>& matrixRStrm,
          hls::stream<MATRIX_IN_T>& VsStrm_out1,
          hls::stream<MATRIX_IN_T>& VsStrm_out2,
          hls::stream<MATRIX_OUT_T>& QRF_A_outstream
	    ) 
      
{
    //xf::solver::qrf<0, 100, 10, MATRIX_IN_T, MATRIX_OUT_T, my_qrf_traits>(matrixAStrm,matrixQStrm,matrixRStrm,VsStrm_out1,VsStrm_out2,QRF_A_outstream);
    xf::solver::qrf_alt<0, 100, 10, my_qrf_traits, MATRIX_IN_T, MATRIX_OUT_T>(matrixAStrm,matrixRStrm,VsStrm_out1,VsStrm_out2,QRF_A_outstream);
}



///////

void mykernel(hls::stream<MATRIX_IN_T> &matrixAStrm, hls::stream<MATRIX_IN_T> &matrixRStrm, hls::stream<MATRIX_IN_T> &VsStrm_in, std::complex<double> matrixR[100]){

	hls::stream<MATRIX_IN_T> inhom_A_outstream;
	hls::stream<MATRIX_OUT_T> weight_stream;
	#pragma HLS stream depth=10000 variable=inhom_A_outstream
  	#pragma HLS stream depth=1000 variable=weight_stream
  
	#pragma HLS DATAFLOW
	inhom(weight_stream,matrixRStrm,VsStrm_in,matrixAStrm,inhom_A_outstream);
	Weights_Mul(matrixR,inhom_A_outstream,weight_stream);

}


///////


void pass_dataflow(
    MATRIX_IN_T* matrixA,
    MATRIX_IN_T* Vs,
    MATRIX_OUT_T* matrixR
    ) 
{

	hls::stream<MATRIX_IN_T> matrixAStrm;
	hls::stream<MATRIX_OUT_T> matrixRStrm;
	hls::stream<MATRIX_OUT_T> qrf_A_outstream;
	hls::stream<MATRIX_OUT_T> VsStrm_out1;
	hls::stream<MATRIX_OUT_T> VsStrm_out2;
	hls::stream<MATRIX_IN_T> inhom_A_outstream;
	hls::stream<MATRIX_IN_T> weight_stream;

 

  #pragma HLS stream depth=10000 variable=matrixAStrm
  #pragma HLS stream depth=10000 variable=matrixRStrm

  #pragma HLS stream depth=10000 variable=qrf_A_outstream
  #pragma HLS stream depth=1000 variable=VsStrm_out1
  #pragma HLS stream depth=1000 variable=VsStrm_out2

  //#pragma HLS stream depth=10000 variable=inhom_A_outstream
  //#pragma HLS stream depth=1000 variable=weight_stream
  
  #pragma HLS DATAFLOW
  
  Master2Stream(matrixA, matrixAStrm,Vs,VsStrm_out1);
  
  QRF(matrixAStrm,matrixRStrm,VsStrm_out1,VsStrm_out2,qrf_A_outstream);

  mykernel(qrf_A_outstream,matrixRStrm,VsStrm_out2,matrixR);
 // inhom(weight_stream,matrixRStrm,VsStrm_out2,qrf_A_outstream,inhom_A_outstream);
  
  //Weights_Mul(matrixR,inhom_A_outstream,weight_stream);

}



extern "C" void Top_Kernel(
    MATRIX_IN_T matrixA[1000],
    MATRIX_IN_T Vs[10],
    MATRIX_OUT_T matrixR[100]
    ) 
{

//#pragma HLS INTERFACE axis port = matrixAStrm
//#pragma HLS INTERFACE axis port = Vs
//#pragma HLS INTERFACE axis port = matrixRStrm 

#pragma HLS INTERFACE ap_ctrl_chain port=return

#pragma HLS INTERFACE m_axi port = matrixA bundle = gmem0 offset = slave depth = 1000
#pragma HLS INTERFACE m_axi port = Vs bundle = gmem1 offset = slave depth = 10
#pragma HLS INTERFACE m_axi port = matrixR bundle = gmem2 offset = slave depth = 100

#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE s_axilite port = matrixA bundle = control
#pragma HLS INTERFACE s_axilite port = Vs bundle = control
#pragma HLS INTERFACE s_axilite port = matrixR bundle = control



pass_dataflow(
    matrixA,
    Vs,
    matrixR
    );
}

