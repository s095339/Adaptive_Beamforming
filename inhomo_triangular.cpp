
#include "hw/Weights_CAL_Mul.h"


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



