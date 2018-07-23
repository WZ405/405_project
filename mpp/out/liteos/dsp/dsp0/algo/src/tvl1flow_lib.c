
// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2011, Javier Sánchez Pérez <jsanchez@dis.ulpgc.es>
// All rights reserved.


#ifndef DUAL_TVL1_OPTIC_FLOW_H
#define DUAL_TVL1_OPTIC_FLOW_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "svp_dsp_performance.h"
#include "mask.c"
#include "bicubic_interpolation.c"
#include "zoom.c"
#include "svp_dsp_def.h"

#define MAX_ITERATIONS 300
#define PRESMOOTHING_SIGMA 0.8
#define GRAD_IS_ZERO 1E-10

/**
 * Implementation of the Zach, Pock and Bischof dual TV-L1 optic flow method
 *
 * see reference:
 *  [1] C. Zach, T. Pock and H. Bischof, "A Duality Based Approach for Realtime
 *      TV-L1 Optical Flow", In Proceedings of Pattern Recognition (DAGM),
 *      Heidelberg, Germany, pp. 214-223, 2007
 *
 *
 * Details on the total variation minimization scheme can be found in:
 *  [2] A. Chambolle, "An Algorithm for Total Variation Minimization and
 *      Applications", Journal of Mathematical Imaging and Vision, 20: 89-97, 2004
 **/


/**
 *
 * Function to compute the optical flow in one scale
 *
 **/
void Dual_TVL1_optic_flow(
		float *I0,           // source image
		float *I1,           // target image
		float *u1,           // x component of the optical flow
		float *u2,           // y component of the optical flow
		const int   nx,      // image width
		const int   ny,      // image height
		const float tau,     // time step
		const float lambda,  // weight parameter for the data term
		const float theta,   // weight parameter for (u - v)²
		const int   warps,   // number of warpings per scale
		const float epsilon, // tolerance for numerical convergence
		const bool  verbose  // enable/disable the verbose mode
		)
{
	unsigned long long cyclesStart = 0,cyclesStop = 0;
    unsigned long long totalCycles = 0;
  float(*__restrict resI0) = I0;
  float(*__restrict resI1) = I1;
  float(*__restrict resu1) = u1;
  float(*__restrict resu2) = u2;
	const int   size = nx * ny;
	const float l_t = lambda * theta;

	size_t sf = sizeof(float);
	float *I1x    = malloc(size*sf);
	float *I1y    = xmalloc(size*sf);
	float *I1w    = xmalloc(size*sf);
	float *I1wx   = xmalloc(size*sf);
	float *I1wy   = xmalloc(size*sf);
	float *rho_c  = xmalloc(size*sf);
	float *v1     = xmalloc(size*sf);
	float *v2     = xmalloc(size*sf);
	float *p11    = xmalloc(size*sf);
	float *p12    = xmalloc(size*sf);
	float *p21    = xmalloc(size*sf);
	float *p22    = xmalloc(size*sf);
	float *div    = xmalloc(size*sf);
	float *grad   = xmalloc(size*sf);
	float *div_p1 = xmalloc(size*sf);
	float *div_p2 = xmalloc(size*sf);
	float *u1x    = xmalloc(size*sf);
	float *u1y    = xmalloc(size*sf);
	float *u2x    = xmalloc(size*sf);
	float *u2y    = xmalloc(size*sf);

	centered_gradient(I1, I1x, I1y, nx, ny);

  	float(*__restrict resu1x) = u1x;
  	float(*__restrict resu1y) = u1y;
  	float(*__restrict resu2x) = u2x;
  	float(*__restrict resu2y) = u2y;
  	float(*__restrict resp11) = p11;
  	float(*__restrict resp12) = p12;
  	float(*__restrict resp21) = p21;
  	float(*__restrict resp22) = p22;
  	float(*__restrict resI1w) = I1w;
  	float(*__restrict resI1wx) = I1wx;
  	float(*__restrict resI1wy) = I1wy;
  	float(*__restrict resrho_c) = rho_c;
  	float(*__restrict resv1) = v1;
  	float(*__restrict resv2) = v2;
  	float(*__restrict resdiv_p1) = div_p1;
  	float(*__restrict resdiv_p2) = div_p2;
  	float(*__restrict resgrad) = grad;
	// initialization of p
	for (int i = 0; i < size; i++)
	{
		resp11[i] = resp12[i] = 0.0;
		resp21[i] = resp22[i] = 0.0;
	}
	for (int warpings = 0; warpings < warps; warpings++)
	{

		// compute the warping of the target image and its derivatives
		bicubic_interpolation_warp(I1,  u1, u2, I1w,  nx, ny, true);
		bicubic_interpolation_warp(I1x, u1, u2, I1wx, nx, ny, true);
		bicubic_interpolation_warp(I1y, u1, u2, I1wy, nx, ny, true);
		for (int i = 0; i < size; i++)
		{
			const float Ix2 = resI1wx[i] * resI1wx[i];
			const float Iy2 = resI1wy[i] * resI1wy[i];

			// store the |Grad(I1)|^2
			resgrad[i] = (Ix2 + Iy2);

			// compute the constant part of the rho function
			resrho_c[i] = (resI1w[i] - resI1wx[i] * resu1[i]
						- resI1wy[i] * resu2[i] - resI0[i]);
		}

		int n = 0;
		float error = INFINITY;

	

		xb_vecN_2xf32* __restrict pvfrho_c = (xb_vecN_2xf32*) rho_c;
		xb_vecN_2xf32* __restrict pvfu1    = (xb_vecN_2xf32*) u1;
		xb_vecN_2xf32* __restrict pvfu2    = (xb_vecN_2xf32*) u2;
		xb_vecN_2xf32* __restrict pvfI1wx  = (xb_vecN_2xf32*) I1wx;
		xb_vecN_2xf32* __restrict pvfI1wy  = (xb_vecN_2xf32*) I1wy;
		xb_vecN_2xf32* __restrict pvfgrad  = (xb_vecN_2xf32*) grad;

		vboolN_2 COMP1;
		xb_vecN_2xf32 vfrh0;

		while (error > epsilon * epsilon && n < MAX_ITERATIONS)
		{
			SVP_DSP_TIME_STAMP(cyclesStart);
			n++;
			// estimate the values of the variable (v1, v2)
			// (thresholding opterator TH)
			for (int i = 0; i < size/16; i++)
			{


//const float rho = resrho_c[i]+ (resI1wx[i] * resu1[i] + resI1wy[i] * resu2[i]);
				vfrho = pvfrho_c+ ((*pvfI1wx) * (*pvfu1) + (*pvfI1wx) * (*pvfu2));
				float d1, d2;
				COMP1 = IVP_OLTN_2XF32(vfrho,)

				// if (rho < - l_t * resgrad[i])
				// {
				// 	d1 = l_t * resI1wx[i];
				// 	d2 = l_t * resI1wy[i];
				// }
				else
				{
					if (rho > l_t * resgrad[i])
					{
						d1 = -l_t * resI1wx[i];
						d2 = -l_t * resI1wy[i];
					}
					else
					{
						if (resgrad[i] < GRAD_IS_ZERO)
							d1 = d2 = 0;
						else
						{
							float fi = -rho/resgrad[i];
							d1 = fi * resI1wx[i];
							d2 = fi * resI1wy[i];
						}
					}
				}

				resv1[i] = resu1[i] + d1;
				resv2[i] = resu2[i] + d2;
			}

			SVP_DSP_TIME_STAMP(cyclesStop);
			printf("While step 1  cycles: %llu \n",cyclesStop-cyclesStart);
			// compute the divergence of the dual variable (p1, p2)
			divergence(p11, p12, resdiv_p1, nx ,ny);
			divergence(p21, p22, resdiv_p2, nx ,ny);



			// estimate the values of the optical flow (u1, u2)
			error = 0.0;
			for (int i = 0; i < size; i++)
			{
				const float u1k = resu1[i];
				const float u2k = resu2[i];

				resu1[i] = resv1[i] + theta * resdiv_p1[i];
				resu2[i] = resv2[i] + theta * resdiv_p2[i];

				error += (resu1[i] - u1k) * (resu1[i] - u1k) +
					(resu2[i] - u2k) * (resu2[i] - u2k);
			}
			error /= size;





			// compute the gradient of the optical flow (Du1, Du2)
			forward_gradient(u1, u1x, u1y, nx ,ny);
			forward_gradient(u2, u2x, u2y, nx ,ny);



			// estimate the values of the dual variable (p1, p2)
			for (int i = 0; i < size; i++)
			{
				const float taut = tau / theta;
				const float g1   = hypot(resu1x[i], resu1y[i]);
				const float g2   = hypot(resu2x[i], resu2y[i]);
				const float ng1  = 1.0 + taut * g1;
				const float ng2  = 1.0 + taut * g2;

				resp11[i] = (resp11[i] + taut * resu1x[i]) / ng1;
				resp12[i] = (resp12[i] + taut * resu1y[i]) / ng1;
				resp21[i] = (resp21[i] + taut * resu2x[i]) / ng2;
				resp22[i] = (resp22[i] + taut * resu2y[i]) / ng2;
			}

		}


		if (verbose)
			printf("Warping: %d, "
					"Iterations: %d, "
					"Error: %f\n", warpings, n, error);
	}

	// delete allocated memory
	free(I1x);
	free(I1y);
	free(I1w);
	free(I1wx);
	free(I1wy);
	free(rho_c);
	free(v1);
	free(v2);
	free(p11);
	free(p12);
	free(p21);
	free(p22);
	free(div);
	free(grad);
	free(div_p1);
	free(div_p2);
	free(u1x);
	free(u1y);
	free(u2x);
	free(u2y);
}

/**
 *
 * Compute the max and min of an array
 *
 **/
static void getminmax(
	float *min,     // output min
	float *max,     // output max
	const float *x, // input array
	int n           // array size
)
{
    float(*__restrict resmin) = min;
    float(*__restrict resmax) = max;
    float(*__restrict resx)  = x;
	#pragma aligned (resmin, 64)    // indicating arrays (pointers) are all aligned to 64bytes.
	#pragma aligned (resmax, 64)    // this will improve compiler's auto vectorization.
	#pragma aligned (resx, 64)     // see section 4.7.2 of Xtensa C/C++ Compiler User's Guide.
	*resmin = *resmax = resx[0];
	for (int i = 1; i < n; i++) {
		if (resx[i] < *resmin)
			*resmin = resmax[i];
		if (resx[i] > *max)
			*resmax = resx[i];
	}
}

/**
 *
 * Function to normalize the images between 0 and 255
 *
 **/
void image_normalization(
		const float *I0,  // input image0
		const float *I1,  // input image1
		float *I0n,       // normalized output image0
		float *I1n,       // normalized output image1
		int size          // size of the image
		)
{
	float max0, max1, min0, min1;

	// obtain the max and min of each image
	unsigned long long cyclesStart = 0;
	unsigned long long cyclesEnd   = 0;
	getminmax(&min0, &max0, I0, size);
	getminmax(&min1, &max1, I1, size);
	//printf("get min max time:  %llu \n",cyclesEnd - cyclesStart);
    //printf("max0%d, max1%d, min0%d, min1%d",max0, max1, min0, min1);
	// obtain the max and min of both images
	const float max = (max0 > max1)? max0 : max1;
	const float min = (min0 < min1)? min0 : min1;
	const float den = max - min;

	
	xb_vecN_2xf32 * __restrict pvfresI0n;
	xb_vecN_2xf32 * __restrict pvfresI1n;
	xb_vecN_2xf32 * __restrict pvfresI0;
	xb_vecN_2xf32 * __restrict pvfresI1;
	
	// float * __restrict fdst1;
	// float * __restrict fdst2;

	xb_vecN_2xf32 vfmin = min;
	xb_vecN_2xf32 vfden = (float)(1/den);
	xb_vecN_2xf32 vf255 = (float)(255.0);

	pvfresI0n = (xb_vecN_2xf32*)I0n;
	pvfresI1n = (xb_vecN_2xf32*)I1n;
	pvfresI0  = (xb_vecN_2xf32*)I0;
	pvfresI1  = (xb_vecN_2xf32*)I1;


	

	SVP_DSP_TIME_STAMP(cyclesStart);
	if (den > 0){
		for (int i = 0; i < size/16; i++)
		{

			*pvfresI0n = (*pvfresI0-vfmin)*vf255*vfden;
			*pvfresI1n = (*pvfresI1-vfmin)*vf255*vfden;	


			IVP_OLTN_2XF32(*pvfresI0n,*pvfresI1n);

			pvfresI0++;
			pvfresI1++;
			pvfresI0n++;
			pvfresI1n++;
		}
	}
	else{
		// copy the original images
		for (int i = 0; i < size; i++)
		{
			*pvfresI0n = *pvfresI0;
			*pvfresI1n = *pvfresI1;
			pvfresI0++;
			pvfresI1++;
			pvfresI0n++;
			pvfresI1n++;
		}
	}
	SVP_DSP_TIME_STAMP(cyclesEnd);
	printf("normalize time %llu \n",cyclesEnd-cyclesStart);

}


/**
 *
 * Function to compute the optical flow using multiple scales
 *
 **/
void Dual_TVL1_optic_flow_multiscale(
		float *I0,           // source image
		float *I1,           // target image
		float *u1,           // x component of the optical flow
		float *u2,           // y component of the optical flow
		int   nxx,     		 // image width
		int   nyy,     		 // image height
		float tau,     		 // time step
		float lambda,  		 // weight parameter for the data term
		float theta,   		 // weight parameter for (u - v)²
		int   nscales, 		 // number of scales
		float zfactor, 		 // factor for building the image piramid
		int   warps,   		 // number of warpings per scale
		float epsilon, 		 // tolerance for numerical convergence
		bool  verbose  		 // enable/disable the verbose mode
)
{

	unsigned long long cyclesStart = 0,cyclesStop = 0;
    unsigned long long totalCycles = 0;
	// printf("nxx %d nyy %d tau %f lambda %f theata %f nscale %d zfactor %f warps %d epsilon %f verbose %d",nxx,nyy,tau,lambda,theta,nscales,zfactor,warps,epsilon,verbose);
	// printf("2 nscales %d\n",nscales);
	int size = nxx * nyy;
	// allocate memory for the pyramid structure
	float **I0s = xmalloc(nscales * sizeof(float*));
	float **I1s = xmalloc(nscales * sizeof(float*));
	float **u1s = xmalloc(nscales * sizeof(float*));
	float **u2s = xmalloc(nscales * sizeof(float*));
	int    *nx  = xmalloc(nscales * sizeof(int));
	int    *ny  = xmalloc(nscales * sizeof(int));

	I0s[0] = xmalloc(size*sizeof(float));
	I1s[0] = xmalloc(size*sizeof(float));

	u1s[0] = u1;
	u2s[0] = u2;
	nx [0] = nxx;
	ny [0] = nyy;

	// normalize the images between 0 and 255
	image_normalization(I0, I1, I0s[0], I1s[0], size);




	// pre-smooth the original images
	gaussian(I0s[0], nx[0], ny[0], PRESMOOTHING_SIGMA);
	gaussian(I1s[0], nx[0], ny[0], PRESMOOTHING_SIGMA);




	// create the scales
	for (int s = 1; s < nscales; s++)
	{
		zoom_size(nx[s-1], ny[s-1], &nx[s], &ny[s], zfactor);
		const int sizes = nx[s] * ny[s];

		// allocate memory
		I0s[s] = xmalloc(sizes*sizeof(float));
		I1s[s] = xmalloc(sizes*sizeof(float));
		u1s[s] = xmalloc(sizes*sizeof(float));
		u2s[s] = xmalloc(sizes*sizeof(float));

		// zoom in the images to create the pyramidal structure
		zoom_out(I0s[s-1], I0s[s], nx[s-1], ny[s-1], zfactor);
		zoom_out(I1s[s-1], I1s[s], nx[s-1], ny[s-1], zfactor);
	}




	SVP_DSP_TIME_STAMP(cyclesStart);
	// initialize the flow at the coarsest scale
	for (int i = 0; i < nx[nscales-1] * ny[nscales-1]; i++)
		u1s[nscales-1][i] = u2s[nscales-1][i] = 0.0;

	// pyramidal structure for computing the optical flow

	printf("nscales %d\n",nscales);
	for (int s = nscales-1; s >= 0; s--)
	{
		if (verbose)
			printf( "Scale %d: %dx%d\n", s, nx[s], ny[s]);

		// compute the optical flow at the current scale
		Dual_TVL1_optic_flow(
				I0s[s], I1s[s], u1s[s], u2s[s], nx[s], ny[s],
				tau, lambda, theta, warps, epsilon, verbose
		);

		// if this was the last scale, finish now
		if (!s) break;

		// otherwise, upsample the optical flow

		// zoom the optical flow for the next finer scale
		zoom_in(u1s[s], u1s[s-1], nx[s], ny[s], nx[s-1], ny[s-1]);
		zoom_in(u2s[s], u2s[s-1], nx[s], ny[s], nx[s-1], ny[s-1]);

		// scale the optical flow with the appropriate zoom factor
		for (int i = 0; i < nx[s-1] * ny[s-1]; i++)
		{
			u1s[s-1][i] *= (float) 1.0 / zfactor;
			u2s[s-1][i] *= (float) 1.0 / zfactor;
		}
	}


    SVP_DSP_TIME_STAMP(cyclesStop);
    totalCycles = (cyclesStop - cyclesStart);
    printf("Dual_TVL1_optic_flow  %llu\n",totalCycles);
	// delete allocated memory
	for (int i = 1; i < nscales; i++)
	{
		free(I0s[i]);
		free(I1s[i]);
		free(u1s[i]);
		free(u2s[i]);
	}
	free(I0s[0]);
	free(I1s[0]);

	free(I0s);
	free(I1s);
	free(u1s);
	free(u2s);
	free(nx);
	free(ny);
}


#endif//DUAL_TVL1_OPTIC_FLOW_H
