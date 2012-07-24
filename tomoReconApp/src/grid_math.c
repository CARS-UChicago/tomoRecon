/* File grid_math.c
   This code was originally included in grid_io.c, separated
   into 2 files so code which only needs math part does not need
   to be linked with netCDF library.
   Mark Rivers, May 2000
*/

 /*******************************************
 * Memory allocation and other utilities
 *******************************************/

#include "grid.h"

/*** Global variables ***/
int verbose=0;

/**** Local variables ***/

void rel_sgram(float **S)
{
	free_matrix(S);

}  /*** End rel_sgram() ***/


float *malloc_vector_f (long n) 
{
    float *v = NULL;
    
    v = (float *) malloc((size_t) (n * sizeof(float)));
    if (!v) {
        fprintf (stderr, "malloc error in malloc_vector_f for length %ld.\n", n);
        v = NULL;
        return v;
    }
    return v;
}

complex *malloc_vector_c (long n) 
{
    complex *v = NULL;
    
    v = (complex *) malloc((size_t) (n * sizeof(complex)));
    if (!v) {
        fprintf (stderr, "malloc error in malloc_vector_c for length %ld.\n", n);
        v = NULL;
        return v;
    }
    return v;
}

float **malloc_matrix_f (long nr, long nc)
{
    float **m = NULL;
    long i;

    /* Allocate pointers to rows */

    m = (float **) malloc((size_t) (nr * sizeof(float *)));
    if (!m) {
        fprintf (stderr, "malloc error in malloc_matrix_f for %ld row pointers.\n", nr);
        m = NULL;
        return m;
    }
    /* Allocate rows and set the pointers to them */

    m[0] = (float *) malloc((size_t) (nr * nc * sizeof(float)));
    if (!m[0]) {
        fprintf (stderr, "malloc error in malloc_matrix_f for %ld row with %ld columns.\n", nr, nc);
        m[0] = NULL;
        free (m);
        m = NULL;
        return m;
    }

    for (i = 1; i < nr; i++) m[i] = m[i-1] + nc;

    return m;
}

complex **malloc_matrix_c (long nr, long nc)
{
    complex **m = NULL;
    long i;

    /* Allocate pointers to rows */
    m = (complex **) malloc((size_t) (nr * sizeof(complex *)));
    if (!m) {
        fprintf (stderr, "malloc error in malloc_matrix_c for %ld row pointers.\n", nr);
        m = NULL;
        return m;
    }
    /* Allocate rows and set the pointers to them */
    m[0] = (complex *) malloc((size_t) (nr * nc * sizeof(complex)));
    if (!m[0]) {
        fprintf (stderr,
		 "malloc error in malloc_matrix_c for %ld row with %ld columns.\n",
			nr, nc);
        m[0] = NULL;
        free (m);
        m = NULL;
        return m;
    }
    for (i = 1; i < nr; i++) m[i] = m[i-1] + nc;

    return m;
}

