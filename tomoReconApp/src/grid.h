/*** File grid.h  12:27 PM 11/7/97 **/

#define ANSI

/**** System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <time.h>
#include <sys/stat.h>

#include <fftw3.h>

/**** Macros and typedefs ****/
#ifndef max
#define max(A,B) ((A)>(B)?(A):(B))
#endif
#ifndef min
#define min(A,B) ((A)<(B)?(A):(B))
#endif
#define free_matrix(A) (free(*(A)),free(A))
#define abs(A) ((A)>0 ?(A):-(A))
#define pi  3.14159265359
#define TOLERANCE 0.1	/* For comparing centers of two sinograms */
#define LTBL_DEF 512	/* Default lookup table length */

typedef struct {
    float r;
    float i;
} complex;

typedef struct { 	/* Sinogram parameters */
   int n_ang;	    /* No. angles in sinogram */
   int n_det;	    /* No. elems (detectors) per angle */
   int geom;		  /* 0 if array of angles provided;
				           * 1,2 if uniform in half,full circle */ 
   float *angles;	/* Ptr to the array of angles, if used */
   float center;	/* Rotation axis location */
} sg_struct;

typedef struct {    /*Prolate spheroidal wave fcn (PSWF) data */
   float C;	        /* Parameter for particular 0th order pswf being used*/
   int nt;          /* Degree of Legendre polynomial expansion */
   float lmbda; 	  /* Eigenvalue */
   float coefs[15];	/* Coeffs for Legendre polynomial expansion */
} pswf_struct;

typedef struct {           /* Parameters for gridding algorithm */
   pswf_struct *pswf;	     /* Ptr to data for PSWF being used  */
   float sampl;	  	       /* "Oversampling" ratio */
   float MaxPixSiz; 	     /* Max pixel size for reconstruction */
   float R;		             /* Region of interest (ROI) relative size */
   float X0;		           /* (X0,Y0)=Offset of ROI from rotation axis, */
   float Y0;		           /* in units of center-to-edge distance.  */
   char fname[16];		     /*  Name of filter function   */		
   float (*filter)(float); /* Ptr to filter function.  */
   long ltbl;		           /* No. elements in convolvent lookup tables. */
   int verbose;            /* Debug printing flag */
} grid_struct;

#ifdef __cplusplus
class grid {  
public:
  grid(grid_struct *GP,sg_struct *SGP, long *imgsiz);
  ~grid();
  void recon(float center, float** G1,float** G2,float*** S1,float*** S2);
  void filphase_su(long pd,float fac, float(*pf)(float),complex *A);
  void pswf_su(pswf_struct *pswf,long ltbl, 
               long linv, float* wtbl,float* dwtbl,float* winv);
  
private:
  int flag;       
  int n_det;
  int n_ang;
  long pdim;
  long M;
  long M0;
  long M02;
  long ltbl;
  float sampl;
  float scale;
  float L;
  float X0;
  float Y0;
  float *SINE;
  float *COSE;
  float *wtbl; 
  float *dwtbl;
  float *work;
  float *winv;
  float previousCenter;
  float (*filter)(float);
  complex *cproj;
  complex *filphase;
  complex **H;
  fftwf_complex *HData;
  
  fftwf_plan forward_1d_plan;
  fftwf_plan backward_2d_plan;
  int verbose;   /* Debug printing flag */
};
#endif

/** Global variables **/

#ifdef __cplusplus
extern "C" {
#endif

/**** Function Prototypes ****/

/** Defined in pswf.c **/
void get_pswf(float C, pswf_struct **P);

/** Defined in filters.c  **/
float (*get_filter(char *name))(float);

#ifdef __cplusplus
}
#endif
