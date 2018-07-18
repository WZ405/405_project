// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2012, Javier Sánchez Pérez <jsanchez@dis.ulpgc.es>
// All rights reserved.


#ifndef ZOOM_C
#define ZOOM_C

#include "xmalloc.c"
#include "mask.c"
#include "bicubic_interpolation.c"

#define ZOOM_SIGMA_ZERO 0.6

/**
  *
  * Compute the size of a zoomed image from the zoom factor
  *
**/
void zoom_size(
	int nx,      // width of the orignal image
	int ny,      // height of the orignal image
	int *nxx,    // width of the zoomed image
	int *nyy,    // height of the zoomed image
	float factor // zoom factor between 0 and 1
)
{
	//compute the new size corresponding to factor
	//we add 0.5 for rounding off to the closest number
	*nxx = (int)((float) nx * factor + 0.5);
	*nyy = (int)((float) ny * factor + 0.5);
}

/**
  *
  * Downsample an image
  *
**/
void zoom_out(
	const float *I,    // input image
	float *Iout,       // output image
	const int nx,      // image width
	const int ny,      // image height
	const float factor // zoom factor between 0 and 1
)
{
	float(*__restrict resI) = I;
  	float(*__restrict resIout) = Iout;


	// temporary working image
	float *Is = xmalloc(nx * ny * sizeof*Is);
    
	float(*__restrict resIs) = Is;

#pragma aligned (resI, 64)     // this will improve compiler's auto vectorization.
#pragma aligned (resIout, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
#pragma aligned (resIs, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
	for(int i = 0; i < nx * ny; i++)
		resIs[i] = resI[i];

	// compute the size of the zoomed image
	int nxx, nyy;
	zoom_size(nx, ny, &nxx, &nyy, factor);

	// compute the Gaussian sigma for smoothing
	const float sigma = ZOOM_SIGMA_ZERO * sqrt(1.0/(factor*factor) - 1.0);

	// pre-smooth the image
	gaussian(resIs, nx, ny, sigma);

	// re-sample the image using bicubic interpolation
	#pragma omp parallel for
	for (int i1 = 0; i1 < nyy; i1++)
	for (int j1 = 0; j1 < nxx; j1++)
	{
		const float i2  = (float) i1 / factor;
		const float j2  = (float) j1 / factor;

		float g = bicubic_interpolation_at(resIs, j2, i2, nx, ny, false);
		resIout[i1 * nxx + j1] = g;
	}

	free(Is);
}


/**
  *
  * Function to upsample the image
  *
**/
void zoom_in(
	const float *I, // input image
	float *Iout,    // output image
	int nx,         // width of the original image
	int ny,         // height of the original image
	int nxx,        // width of the zoomed image
	int nyy         // height of the zoomed image
)
{
	// compute the zoom factor
	const float factorx = ((float)nxx / nx);
	const float factory = ((float)nyy / ny);

	// re-sample the image using bicubic interpolation
	#pragma omp parallel for
	for (int i1 = 0; i1 < nyy; i1++)
	for (int j1 = 0; j1 < nxx; j1++)
	{
		float i2 =  (float) i1 / factory;
		float j2 =  (float) j1 / factorx;

		float g = bicubic_interpolation_at(I, j2, i2, nx, ny, false);
		Iout[i1 * nxx + j1] = g;
	}
}



#endif//ZOOM_C
