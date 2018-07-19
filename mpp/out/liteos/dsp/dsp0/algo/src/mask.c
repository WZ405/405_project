// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2011, Javier Sánchez Pérez <jsanchez@dis.ulpgc.es>
// All rights reserved.


#ifndef MASK_C
#define MASK_C

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "xmalloc.c"


#define BOUNDARY_CONDITION_DIRICHLET 0
#define BOUNDARY_CONDITION_REFLECTING 1
#define BOUNDARY_CONDITION_PERIODIC 2

#define DEFAULT_GAUSSIAN_WINDOW_SIZE 5
#define DEFAULT_BOUNDARY_CONDITION BOUNDARY_CONDITION_REFLECTING


/**
 *
 * Details on how to compute the divergence and the grad(u) can be found in:
 * [2] A. Chambolle, "An Algorithm for Total Variation Minimization and
 * Applications", Journal of Mathematical Imaging and Vision, 20: 89-97, 2004
 *
 **/


/**
 *
 * Function to compute the divergence with backward differences
 * (see [2] for details)
 *
 **/
void divergence(
		const float *v1, // x component of the vector field
		const float *v2, // y component of the vector field
		float *div,      // output divergence
		const int nx,    // image width
		const int ny     // image height
	       )
{
	// compute the divergence on the central body of the image
  float(*__restrict resv1) = v1;
  float(*__restrict resv2) = v2;
  float(*__restrict resdiv) = div;
#pragma aligned (resv1, 64)     // this will improve compiler's auto vectorization.
#pragma aligned (resv2, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
#pragma aligned (resdiv, 64)     // this will improve compiler's auto vectorization.
	for (int i = 1; i < ny-1; i++)
	{
		for(int j = 1; j < nx-1; j++)
		{
			const int p  = i * nx + j;
			const int p1 = p - 1;
			const int p2 = p - nx;

			const float v1x = resv1[p] - resv1[p1];
			const float v2y = resv2[p] - resv2[p2];

			resdiv[p] = v1x + v2y;
		}
	}

	// compute the divergence on the first and last rows
	for (int j = 1; j < nx-1; j++)
	{
		const int p = (ny-1) * nx + j;

		resdiv[j] = resv1[j] - resv1[j-1] + resv2[j];
		resdiv[p] = resv1[p] - resv1[p-1] - resv2[p-nx];
	}

	// compute the divergence on the first and last columns
	for (int i = 1; i < ny-1; i++)
	{
		const int p1 = i * nx;
		const int p2 = (i+1) * nx - 1;

		resdiv[p1] =  resv1[p1]   + resv2[p1] - resv2[p1 - nx];
		resdiv[p2] = -resv1[p2-1] + resv2[p2] - resv2[p2 - nx];

	}

	resdiv[0]         =  resv1[0] + resv2[0];
	resdiv[nx-1]      = -resv1[nx - 2] + resv2[nx - 1];
	resdiv[(ny-1)*nx] =  resv1[(ny-1)*nx] - resv2[(ny-2)*nx];
	resdiv[ny*nx-1]   = -resv1[ny*nx - 2] - resv2[(ny-1)*nx - 1];
}


/**
 *
 * Function to compute the gradient with forward differences
 * (see [2] for details)
 *
 **/
void forward_gradient(
		const float *f, //input image
		float *fx,      //computed x derivative
		float *fy,      //computed y derivative
		const int nx,   //image width
		const int ny    //image height
		)
{
	// compute the gradient on the central body of the image
//#pragma omp parallel for schedule(dynamic)
  float(*__restrict resf) = f;
  float(*__restrict resfx) = fx;
  float(*__restrict resfy) = fy;
#pragma aligned (resf, 64)     // this will improve compiler's auto vectorization.
#pragma aligned (resfx, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
#pragma aligned (resfy, 64)     // this will improve compiler's auto vectorization.
	for (int i = 0; i < ny-1; i++)
	{
		for(int j = 0; j < nx-1; j++)
		{
			const int p  = i * nx + j;
			const int p1 = p + 1;
			const int p2 = p + nx;

			resfx[p] = resf[p1] - resf[p];
			resfy[p] = resf[p2] - resf[p];
		}
	}

	// compute the gradient on the last row
	for (int j = 0; j < nx-1; j++)
	{
		const int p = (ny-1) * nx + j;

		resfx[p] = resf[p+1] - f[p];
		resfy[p] = 0;
	}

	// compute the gradient on the last column
	for (int i = 1; i < ny; i++)
	{
		const int p = i * nx-1;

		resfx[p] = 0;
		resfy[p] = f[p+nx] - f[p];
	}

	resfx[ny * nx - 1] = 0;
	resfy[ny * nx - 1] = 0;
}


/**
 *
 * Function to compute the gradient with centered differences
 *
 **/
void centered_gradient(
		const float *input,  //input image
		float *dx,           //computed x derivative
		float *dy,           //computed y derivative
		const int nx,        //image width
		const int ny         //image height
		)
{
	// compute the gradient on the center body of the image
//#pragma omp parallel for schedule(dynamic)

    float(*__restrict resinput) = input;
    float(*__restrict resdx) = dx;
    float(*__restrict resdy) = dy;
#pragma aligned (input, 64)     // this will improve compiler's auto vectorization.
#pragma aligned (dx, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
#pragma aligned (dy, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
	for (int i = 1; i < ny-1; i++)
	{
		for(int j = 1; j < nx-1; j++)
		{
			const int k = i * nx + j;
			resdx[k] = 0.5*(resinput[k+1] - resinput[k-1]);
			resdy[k] = 0.5*(resinput[k+nx] - resinput[k-nx]);
		}
	}

	// compute the gradient on the first and last rows
	for (int j = 1; j < nx-1; j++)
	{
		resdx[j] = 0.5*(resinput[j+1] - resinput[j-1]);
		resdy[j] = 0.5*(resinput[j+nx] - resinput[j]);

		const int k = (ny - 1) * nx + j;

		resdx[k] = 0.5*(resinput[k+1] - resinput[k-1]);
		resdy[k] = 0.5*(resinput[k] - resinput[k-nx]);
	}

	// compute the gradient on the first and last columns
	for(int i = 1; i < ny-1; i++)
	{
		const int p = i * nx;
		resdx[p] = 0.5*(resinput[p+1] - resinput[p]);
		resdy[p] = 0.5*(resinput[p+nx] - resinput[p-nx]);

		const int k = (i+1) * nx - 1;

		resdx[k] = 0.5*(resinput[k] - resinput[k-1]);
		resdy[k] = 0.5*(resinput[k+nx] - resinput[k-nx]);
	}

	// compute the gradient at the four corners
	resdx[0] = 0.5*(resinput[1] - resinput[0]);
	resdy[0] = 0.5*(resinput[nx] - resinput[0]);

	resdx[nx-1] = 0.5*(resinput[nx-1] - resinput[nx-2]);
	resdy[nx-1] = 0.5*(resinput[2*nx-1] - resinput[nx-1]);

	resdx[(ny-1)*nx] = 0.5*(resinput[(ny-1)*nx + 1] - resinput[(ny-1)*nx]);
	resdy[(ny-1)*nx] = 0.5*(resinput[(ny-1)*nx] - resinput[(ny-2)*nx]);

	resdx[ny*nx-1] = 0.5*(resinput[ny*nx-1] - resinput[ny*nx-1-1]);
	resdy[ny*nx-1] = 0.5*(resinput[ny*nx-1] - resinput[(ny-1)*nx-1]);
}


/**
 *
 * In-place Gaussian smoothing of an image
 *
 */
void gaussian(
	float *I,             // input/output image
	const int xdim,       // image width
	const int ydim,       // image height
	const float sigma    // Gaussian sigma
)
{
	const int boundary_condition = DEFAULT_BOUNDARY_CONDITION;
	const int window_size = DEFAULT_GAUSSIAN_WINDOW_SIZE;

	const float den  = 2*sigma*sigma;
	const int   size = (int) (window_size * sigma) + 1 ;
	const int   bdx  = xdim + size;
	const int   bdy  = ydim + size;

	if (boundary_condition && size > xdim) {
		fprintf(stderr, "GaussianSmooth: sigma too large\n");
		abort();
	}



    float(*__restrict resI) = I;

#pragma aligned (resI, 64)     // this will improve compiler's auto vectorization.

	// compute the coefficients of the 1D convolution kernel
	float B[size];
#pragma aligned (B, 64)	
	for(int i = 0; i < size; i++)
		B[i] = 1 / (sigma * sqrt(2.0 * 3.1415926)) * exp(-i * i / den);

	// normalize the 1D convolution kernel
	float norm = 0;
#pragma aligned (B, 64)	
	for(int i = 0; i < size; i++)
		norm += B[i];
	norm *= 2;
	norm -= B[0];
#pragma aligned (B, 64)	
	for(int i = 0; i < size; i++)
		B[i] /= norm;

	// convolution of each line of the input image
	float *R = xmalloc((size + xdim + size)*sizeof*R);


    float(*__restrict resR) = R;
#pragma aligned (resR, 64)     // this will improve compiler's auto vectorization.
    
	for (int k = 0; k < ydim; k++)
	{
		int i, j;
		#pragma aligned (resR, 64)
		#pragma aligned (resI, 64)
		for (i = size; i < bdx; i++)
			resR[i] = resI[k * xdim + i - size];

		switch (boundary_condition)
		{
		case BOUNDARY_CONDITION_DIRICHLET:
			#pragma aligned (resR, 64)
			#pragma aligned (resI, 64)
			for(i = 0, j = bdx; i < size; i++, j++)
				resR[i] = resR[j] = 0;
			break;

		case BOUNDARY_CONDITION_REFLECTING:
		    #pragma aligned (resR, 64)
		    #pragma aligned (resI, 64)
			for(i = 0, j = bdx; i < size; i++, j++) {
				resR[i] = resI[k * xdim + size-i];
				resR[j] = resI[k * xdim + xdim-i-1];
			}
			break;

		case BOUNDARY_CONDITION_PERIODIC:
		    #pragma aligned (resR, 64)
		    #pragma aligned (resI, 64)
			for(i = 0, j = bdx; i < size; i++, j++) {
				resR[i] = resI[k * xdim + xdim-size+i];
				resR[j] = resI[k * xdim + i];
			}
			break;
		}
        #pragma aligned (resR, 64)
		#pragma aligned (resI, 64)
		#pragma aligned (B, 64)	
		for (i = size; i < bdx; i++)
		{
			float sum = B[0] * resR[i];
			for (j = 1; j < size; j++ )
				sum += B[j] * ( resR[i-j] + resR[i+j] );
			resI[k * xdim + i - size] = sum;
		}
	}

	// convolution of each column of the input image
	float *T = xmalloc((size + ydim + size)*sizeof*T);

    printf("RESRREST\n");
	float(*__restrict resT) = T;
#pragma aligned (resT, 64)     // this will improve compiler's auto vectorization.
	for (int k = 0; k < xdim; k++)
	{
		int i, j;
		for (i = size; i < bdy; i++)
			resT[i] = resI[(i - size) * xdim + k];

		switch (boundary_condition)
		{
		case BOUNDARY_CONDITION_DIRICHLET:
			for (i = 0, j = bdy; i < size; i++, j++)
				resT[i] = resT[j] = 0;
			break;

		case BOUNDARY_CONDITION_REFLECTING:
			for (i = 0, j = bdy; i < size; i++, j++) {
				resT[i] = resI[(size-i) * xdim + k];
				resT[j] = resI[(ydim-i-1) * xdim + k];
			}
			break;

		case BOUNDARY_CONDITION_PERIODIC:
			for( i = 0, j = bdx; i < size; i++, j++) {
				resT[i] = resI[(ydim-size+i) * xdim + k];
				resT[j] = resI[i * xdim + k];
			}
			break;
		}

		for (i = size; i < bdy; i++)
		{
			float sum = B[0] * resT[i];
			for (j = 1; j < size; j++ )
				sum += B[j] * (resT[i-j] + resT[i+j]);
			resI[(i - size) * xdim + k] = sum;
		}
	}

	free(R);
	free(T);
}


#endif//MASK_C
