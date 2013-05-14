#include <string.h>
#include <fftw3.h>

#include <epicsExport.h>

extern "C" {

epicsShareFunc void epicsShareAPI fftw_1d (int argc, char *argv[])
{

        float *data = (float *)argv[0];
        int n = *(int *)argv[1];
        int isign  = *(int *)argv[2];

        static int n_prev;
        static fftwf_complex *in, *out;
        static fftwf_plan forward_plan, backward_plan;

        if (n != n_prev) {
           /* Create plans */
           if (n_prev != 0) fftwf_free(in);
           in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*n);
           out = in;
           printf("fft_test1f: creating plans, n=%d, n_prev=%d\n", n, n_prev);
           n_prev = n;
           forward_plan = fftwf_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_MEASURE);
           backward_plan = fftwf_plan_dft_1d(n, in, out, FFTW_BACKWARD, FFTW_MEASURE);
        }
        memcpy(in, data, n*sizeof(fftwf_complex));
        if (isign == -1) fftwf_execute(forward_plan);
        else             fftwf_execute(backward_plan);
        memcpy(data, in, n*sizeof(fftwf_complex));
}

epicsShareFunc void epicsShareAPI fftw_2d (int argc, char *argv[])
{

        float *data = (float *)argv[0];
        int nx  = *(int *)argv[1];
        int ny  = *(int *)argv[2];
        int isign  = *(int *)argv[3];

        static int nx_prev, ny_prev;
        static fftwf_complex *in, *out;
        static fftwf_plan forward_plan, backward_plan;

        if ((nx != nx_prev) || (ny != ny_prev)) {
           /* Create plans */
           if (nx_prev != 0) fftwf_free(in);
           in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*nx*ny);
           out = in;
           printf("fft_test2f: creating plans, nx=%d, ny=%d, nx_prev=%d, ny_prev=%d\n", 
                  nx, ny, nx_prev, ny_prev);
           nx_prev = nx;
           ny_prev = ny;
           forward_plan = fftwf_plan_dft_2d(ny, nx, in, out, FFTW_FORWARD, FFTW_MEASURE);
           backward_plan = fftwf_plan_dft_2d(ny, nx, in, out, FFTW_BACKWARD, FFTW_MEASURE);
        }
        memcpy(in, data, nx*ny*sizeof(fftwf_complex));
        if (isign == -1) fftwf_execute(forward_plan);
        else             fftwf_execute(backward_plan);
        memcpy(data, in, nx*ny*sizeof(fftwf_complex));
}

} // extern "C"
