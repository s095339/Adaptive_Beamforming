#include "libspir_types.h"
#include "hls_stream.h"
#include "xcl_top_defines.h"
#include "ap_axi_sdata.h"
#include "xcl_top_datamovers.h"
#define EXPORT_PIPE_SYMBOLS 1
#include "cpu_pipes.h"
#undef EXPORT_PIPE_SYMBOLS
#include "xcl_half.h"
#include <cstddef>
#include <vector>
#include <complex>
#include <pthread.h>
using namespace std;

extern "C" {

void Top_Kernel(size_t matrixA, size_t matrixR);

static pthread_mutex_t __xlnx_cl_Top_Kernel_mutex = PTHREAD_MUTEX_INITIALIZER;
void __stub____xlnx_cl_Top_Kernel(char **argv) {
  void **args = (void **)argv;
  size_t matrixA = *((size_t*)args[0+1]);
  size_t matrixR = *((size_t*)args[1+1]);
 pthread_mutex_lock(&__xlnx_cl_Top_Kernel_mutex);
  Top_Kernel(matrixA, matrixR);
  pthread_mutex_unlock(&__xlnx_cl_Top_Kernel_mutex);
}
}
