// wasm-kernels/alg_wasm.c
#include <stdint.h>
#define N 32
static int16_t A[N*N], B[N*N];
static volatile int32_t C[N*N], sink;

__attribute__((export_name("mm32x32_i16")))
void mm32x32_i16(void) {
  for (int i=0;i<N*N;i++){ A[i]=(i*3+1)&0x7FFF; B[i]=(i*5+7)&0x7FFF; }
  for (int i=0;i<N;i++) for (int j=0;j<N;j++){
    int32_t acc=0;
    for (int k=0;k<N;k++) acc += (int32_t)A[i*N+k]*(int32_t)B[k*N+j];
    C[i*N+j]=acc;
  }
  int32_t sum=0; for (int i=0;i<N*N;i++) sum+=C[i];
  sink=sum;
}
