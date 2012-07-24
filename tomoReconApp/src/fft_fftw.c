/*** File fft_fftw.c 2/26/2005
     This file calls routines from the FFTW library.  It 
     emulates the 1-D and n-D routines from Numerical Recipes, so that 
     gridrec requires essentially no modification

     Written by Mark Rivers
 **/
#include <stdlib.h>
#include <string.h>
#include <fftw3.h>

/* Note that we are using the routines designed for
 * COMPLEX data type.  C does not normally support COMPLEX, but Gridrec uses
 * a C structure to emulate it.
 */

void four1(float data[], unsigned long nn, int isign)
{
   static int n_prev;
   static fftwf_complex *in, *out;
   static fftwf_plan forward_plan, backward_plan;
   int n = nn;

   if (n != n_prev) {
      /* Create plans */
      if (n_prev != 0) fftwf_free(in);
      in = fftwf_malloc(sizeof(fftwf_complex)*n);
      out = in;
      printf("fft_test1f: creating plans, n=%d, n_prev=%d\n", n, n_prev);
      n_prev = n;
      forward_plan = fftwf_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_MEASURE);
      backward_plan = fftwf_plan_dft_1d(n, in, out, FFTW_BACKWARD, FFTW_MEASURE);
   }
   /* The Numerical Recipes routines are passed a pointer to one element
    * before the start of the array - add one */
   memcpy(in, data+1, n*sizeof(fftwf_complex));
   if (isign == -1) fftwf_execute(forward_plan);
   else             fftwf_execute(backward_plan);
   memcpy(data+1, in, n*sizeof(fftwf_complex));
}
	
void fourn(float data[], unsigned long nn[], int ndim, int isign)
{
   static int nx_prev, ny_prev;
   static fftwf_complex *in, *out;
   static fftwf_plan forward_plan, backward_plan;
   int nx = nn[2];
   int ny = nn[1];

   /* NOTE: This function only works for ndim=2 */
   if (ndim != 2) {
      printf("fourn only works with ndim=2\n");
      return;
   }

   if ((nx != nx_prev) || (ny != ny_prev)) {
      /* Create plans */
      if (nx_prev != 0) fftwf_free(in);
      in = fftwf_malloc(sizeof(fftwf_complex)*nx*ny);
      out = in;
      printf("fft_test2f: creating plans, nx=%d, ny=%d, nx_prev=%d, ny_prev=%d\n",
             nx, ny, nx_prev, ny_prev);
      nx_prev = nx;
      ny_prev = ny;
      forward_plan = fftwf_plan_dft_2d(ny, nx, in, out, FFTW_FORWARD, FFTW_MEASURE);
      backward_plan = fftwf_plan_dft_2d(ny, nx, in, out, FFTW_BACKWARD, FFTW_MEASURE);
   }
   /* The Numerical Recipes routines are passed a pointer to one element
    * before the start of the array - add one */
   memcpy(in, data+1, nx*ny*sizeof(fftwf_complex));
   if (isign == -1) fftwf_execute(forward_plan);
   else             fftwf_execute(backward_plan);
   memcpy(data+1, in, nx*ny*sizeof(fftwf_complex));
}
