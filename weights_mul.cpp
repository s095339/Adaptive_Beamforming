#include "hw/Weights_CAL_Mul.h"


void Weights_Mul(complex<double> output_result[S], hls::stream<complex<double>> &A_instream, hls::stream<complex<double>> &weights_instream){

//#pragma HLS STREAM depth=1000 variable=A_instream
//#pragma HLS STREAM depth=10 variable=weights_instream
	//ADD STATIC https://docs.xilinx.com/r/2021.2-English/ug1399-vitis-hls/Array-Initialization
static	complex<double> A_matrix_w[S][M]={complex<double>(0,0)};
#pragma HLS ARRAY_RESHAPE variable=A_matrix_w type=block factor=2 dim=2
#pragma HLS BIND_STORAGE variable=A_matrix_w type=ram_1wnr

static	complex<double> INweights[M]={complex<double>(0,0)};
#pragma HLS ARRAY_RESHAPE variable=INweights type=complete
static	complex<double> results[S]={complex<double>(0,0)};
//#pragma HLS ARRAY_PARTITION variable=results type=complete


//read stream A and weights
read_stream_input:
	for(int i=0;i<S;i++){
		for(int j=0;j<M;j++){
#pragma HLS PIPELINE
			A_matrix_w[i][j]=A_instream.read();
			if(i==S-1){
				INweights[j]=weights_instream.read();
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
			results[i]+=A_matrix_w[i][j]*INweights[j];
		}
	}

//burst write to host
burst_write_output:
	for(int i=0;i<S;i++){
#pragma HLS PIPELINE II=1
		output_result[i]=results[i];
	}


}
