
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



#include "mask.c"
#include "bicubic_interpolation.c"
#include "zoom.c"

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

    unsigned long long cyclesStart = 0,cyclesStop = 0;
    unsigned long long totalCycles = 0;
	SVP_DSP_TIME_STAMP(cyclesStart);

	centered_gradient(I1, I1x, I1y, nx, ny);
	SVP_DSP_TIME_STAMP(cyclesStop);
	totalCycles = (cyclesStop - cyclesStart);
	printf("BICUBICINTERPOLATION  %llu\n",totalCycles);


	// initialization of p
	xb_vecN_2xf32 *  pvfpp11;	
	xb_vecN_2xf32 *  pvfpp12;
	xb_vecN_2xf32 *  pvfpp21;	
	xb_vecN_2xf32 *  pvfpp22;
	xb_vecN_2xf32 vfzero = (float)0.0;
	pvfpp11=(xb_vecN_2xf32*)p11;
	pvfpp12=(xb_vecN_2xf32*)p12;
	pvfpp21=(xb_vecN_2xf32*)p21;
	pvfpp22=(xb_vecN_2xf32*)p22;

	for (int i = 0; i < size/16; i++)
	{
		*pvfpp11++ = vfzero;
		*pvfpp12++ = vfzero;
		*pvfpp21++ = vfzero;
		*pvfpp22++ = vfzero;
	}

	xb_vecN_2xf32 *  pvfI1wx;	
	xb_vecN_2xf32 *  pvfI1wy;
	xb_vecN_2xf32 *  pvfgrad;
	xb_vecN_2xf32 *  pvfrho_c;
	xb_vecN_2xf32 *  pvfu1;
	xb_vecN_2xf32 *  pvfu2;
	xb_vecN_2xf32 *  pvfI0;
	xb_vecN_2xf32 *  pvfI1w;

	xb_vecN_2xf32 vfIx2;
	xb_vecN_2xf32 vfIy2;

	xb_vecN_2xf32 *  pvfu1x;
	xb_vecN_2xf32 *  pvfu2x;
	xb_vecN_2xf32 *  pvfu1y;
	xb_vecN_2xf32 *  pvfu2y;
	xb_vecN_2xf32 * pvfv1;
	xb_vecN_2xf32 * pvfv2;
	xb_vecN_2xf32 * pvfdivp1;
	xb_vecN_2xf32 * pvfdivp2;


	for (int warpings = 0; warpings < warps; warpings++)
	{
		// compute the warping of the target image and its derivatives

		bicubic_interpolation_warp(I1,  u1, u2, I1w,  nx, ny, false);
		bicubic_interpolation_warp(I1x, u1, u2, I1wx, nx, ny, false);
		bicubic_interpolation_warp(I1y, u1, u2, I1wy, nx, ny, false);


				// compute the warping of the target image and its derivatives

		pvfI1wx=(xb_vecN_2xf32*)I1wx;
		pvfI1wy=(xb_vecN_2xf32*)I1wy;
		pvfgrad=(xb_vecN_2xf32*)grad;
		pvfrho_c=(xb_vecN_2xf32*)rho_c;
		pvfI1w=(xb_vecN_2xf32*)I1w;
		pvfu1=(xb_vecN_2xf32*)u1;
		pvfu2=(xb_vecN_2xf32*)u2;
		pvfI0=(xb_vecN_2xf32*)I0;
		for (int i = 0; i < size/16; i++)
		{
			vfIx2 = (*pvfI1wx) * (*pvfI1wx);
			vfIy2 = (*pvfI1wy) * (*pvfI1wy);

			// store the |Grad(I1)|^2
			*pvfgrad = vfIx2+vfIy2;

			// compute the constant part of the rho function
			*pvfrho_c = (*pvfI1w) - ((*pvfI1wx) * (*pvfu1)) - ((*pvfI1wy) * (*pvfu2)) - (*pvfI0);
			
			pvfI1wx++;
			pvfI1wy++;
			pvfgrad++;
			pvfrho_c++;
			pvfI1w++;
			pvfu1++;
			pvfu2++;
			pvfI0++;
		}

		int n = 0;
		float error = INFINITY;
		float errorp[16];
		xb_vecN_2xf32 * pvferror = errorp;
			SVP_DSP_TIME_STAMP(cyclesStart);
		while (error > epsilon * epsilon && n < MAX_ITERATIONS)
		{
			n++;
			// estimate the values of the variable (v1, v2)
			// (thresholding opterator TH)



			pvfI1wx=(xb_vecN_2xf32*)I1wx;
			pvfI1wy=(xb_vecN_2xf32*)I1wy;
			pvfgrad=(xb_vecN_2xf32*)grad;
			pvfrho_c=(xb_vecN_2xf32*)rho_c;
			pvfI1w=(xb_vecN_2xf32*)I1w;
			pvfu1=(xb_vecN_2xf32*)u1;
			pvfu2=(xb_vecN_2xf32*)u2;
			pvfv1=(xb_vecN_2xf32*)v1;
			pvfv2=(xb_vecN_2xf32*)v2;


			
			for(int i = 0;i < size/16;i++){
				xb_vecN_2xf32 vfrho = *pvfrho_c + ((*pvfI1wx) * (*pvfu1) + (*pvfI1wy) * (*pvfu2));

				xb_vecN_2xf32 mvfrho = IVP_NEGN_2XF32(vfrho);
				xb_vecN_2xf32 vfd1 = (float) 0.0;
				xb_vecN_2xf32 vfd2 = (float) 0.0;
				xb_vecN_2xf32 vfl_t = (float)(l_t);
				xb_vecN_2xf32 mvfl_t = (float)(-l_t);
				xb_vecN_2xf32 vfzero = (float) 0.0;
				xb_vecN_2xf32 vfgradiszero = (float)(GRAD_IS_ZERO);

 				//布尔向量， 用来进行批量条件判断
				vboolN_2 if1;  //储存第一次判断中的真值
				vboolN_2 nif1; //储存第一次判断中的假值
				vboolN_2 if2;
				vboolN_2 nif2;
				vboolN_2 rif2;
				vboolN_2 if3;
				vboolN_2 nif3;
				vboolN_2 rif3;
				vboolN_2 rnif3;
				if1 = IVP_OLTN_2XF32(vfrho,mvfl_t * (*pvfgrad)); //批量判断vfrho 中的值是否小于 pvfgrad中的值
				if2 = IVP_OLTN_2XF32(vfl_t * (*pvfgrad),vfrho);
				if3 = IVP_OLTN_2XF32((*pvfgrad) ,vfgradiszero);
				nif1 = IVP_NOTBN_2(if1); //对于假值，直接对真值进行取反即可。
				nif2 = IVP_NOTBN_2(if2);
				nif3 = IVP_NOTBN_2(if3);
				vfd1 = IVP_MOVN_2XF32T( vfl_t* (*pvfI1wx),vfd1,if1); //根据布尔数组，挑选值为1的位，对vfd1中对应位置进行赋值
				vfd2 = IVP_MOVN_2XF32T( vfl_t* (*pvfI1wy),vfd2,if1);
				rif2 = IVP_ANDBN_2(if2,nif1); //通过布尔运算，得到第二次判断中的真值
				vfd1 = IVP_MOVN_2XF32T(mvfl_t* (*pvfI1wx),vfd1,rif2);
				vfd2 = IVP_MOVN_2XF32T(mvfl_t* (*pvfI1wy),vfd2,rif2);
				rif3 = IVP_ANDBN_2(IVP_ANDBN_2(nif2,nif1),if3);
				vfd1 = IVP_MOVN_2XF32T(vfzero,vfd1,rif3);
				vfd2 = IVP_MOVN_2XF32T(vfzero,vfd2,rif3);	
				rnif3 = IVP_ANDBN_2(IVP_ANDBN_2(nif2,nif1),nif3);

				float fi[16];
				float invgrad[16];

				xb_vecN_2xf32 * vffi;
				vffi = (xb_vecN_2xf32*)fi;

				xb_vecN_2xf32 * invpvfgrad ;
				invpvfgrad = (xb_vecN_2xf32*) invgrad;
				*invpvfgrad = IVP_DIV0N_2XF32(*pvfgrad); //第一步处理除数
				*vffi = *invpvfgrad;
				IVP_DIVNN_2XF32(*vffi,vfrho,*invpvfgrad); //第二部，被除数除以除数
				//*vffi = mvfrho * *invpvfgrad;

				vfd1 = IVP_MOVN_2XF32T(*vffi* (*pvfI1wx),vfd1,rnif3);
				vfd2 = IVP_MOVN_2XF32T(*vffi* (*pvfI1wy),vfd2,rnif3);
/*
				for(int z = 0;z<16;z++){
					printf("%f %f %f\n",fi[z],invgrad[z],grad[i*16+z]);

				}*/
				*pvfv1 = *pvfu1 + vfd1;
				*pvfv2 = *pvfu2 + vfd2;
				pvfv1++;
				pvfv2++;
				pvfgrad++;
				pvfrho_c++;
				pvfI1wx++;
				pvfu1++;
				pvfI1wy++;
				pvfu2++;



				/*float tmp[16];
				bool tmpb[16];
				xb_vecN_2xf32 * vftmp = (xb_vecN_2xf32 *) tmp;
				vboolN_2 * vftmpb = (vboolN_2 *) tmpb;
				*vftmp = vfd1;
				*vftmpb = rnif3;*/
	/*			for(int z = 0;z<16;z++){
					const float rho = rho_c[i*16+z]
					+ (I1wx[i*16+z] * u1[i*16+z] + I1wy[i*16+z] * u2[i*16+z]);
					float d1, d2;

					if (rho < - l_t * grad[i*16+z])
					{
						d1 = l_t * I1wx[i*16+z];
						d2 = l_t * I1wy[i*16+z];
						//printf("if1 %f ",d1);
					}
					else
					{
						if (rho > l_t * grad[i*16+z])
						{
							d1 = -l_t * I1wx[i*16+z];
							d2 = -l_t * I1wy[i*16+z];
							//printf("rif2 %f ",d1);
						}
						else
						{
							if (grad[i*16+z] < GRAD_IS_ZERO){
								d1 = d2 = 0;
								//printf("rif3 %f ",d1);
							}

							else
							{
								float fi = -rho/grad[i*16+z];
								d1 = fi * I1wx[i*16+z];
								d2 = fi * I1wy[i*16+z];
								//printf("nif3 %f ",d1);
							}
						}
					}
					//printf("%f %f %d\n",tmp[z],d1,tmpb[z]);
					v1[i*16+z] = u1[i*16+z] + d1;
					v2[i*16+z] = u2[i*16+z] + d2;
				}
				//printf("\n");

*/

			}

		/*	for (int i = 0; i < size; i++)
			{
				const float rho = rho_c[i]
					+ (I1wx[i] * u1[i] + I1wy[i] * u2[i]);

				float d1, d2;

				if (rho < - l_t * grad[i])
				{
					d1 = l_t * I1wx[i];
					d2 = l_t * I1wy[i];
				}
				else
				{
					if (rho > l_t * grad[i])
					{
						d1 = -l_t * I1wx[i];
						d2 = -l_t * I1wy[i];
					}
					else
					{
						if (grad[i] < GRAD_IS_ZERO)
							d1 = d2 = 0;
						else
						{
							float fi = -rho/grad[i];
							d1 = fi * I1wx[i];
							d2 = fi * I1wy[i];
						}
					}
				}

				v1[i] = u1[i] + d1;
				v2[i] = u2[i] + d2;
			}*/

			// compute the divergence of the dual variable (p1, p2)
			divergence(p11, p12, div_p1, nx ,ny);
			divergence(p21, p22, div_p2, nx ,ny);


/*
			// estimate the values of the optical flow (u1, u2)
			error = 0.0;

			for (int i = 0; i < size; i++)
			{
				const float u1k = u1[i];
				const float u2k = u2[i];

				u1[i] = v1[i] + theta * div_p1[i];
				u2[i] = v2[i] + theta * div_p2[i];

				error += (u1[i] - u1k) * (u1[i] - u1k) +(u2[i] - u2k) * (u2[i] - u2k);
			}
			error /= size;

*/
			error = 0;
			for(int j = 0;j<16;j++){
				errorp[j] = 0;
			}
			pvfu1 = (xb_vecN_2xf32 *)u1;
			pvfu2 = (xb_vecN_2xf32 *)u2;
			pvfv1 = (xb_vecN_2xf32 *)v1;
			pvfv2 = (xb_vecN_2xf32 *)v2;
			pvfdivp1 = (xb_vecN_2xf32 *)div_p1;
			pvfdivp2 = (xb_vecN_2xf32 *)div_p2;
			for(int i = 0;i<size/16;i++){
				
				xb_vecN_2xf32 vfu1k = *pvfu1;
				xb_vecN_2xf32 vfu2k = *pvfu2;
				xb_vecN_2xf32 vftheta = (float)theta;
				*pvfu1 = *pvfv1 + vftheta * *pvfdivp1;
				*pvfu2 = *pvfv2 + vftheta * *pvfdivp2;
				*pvferror = *pvferror + (*pvfu1 - vfu1k) * (*pvfu1 - vfu1k) + (*pvfu2 - vfu2k) * (*pvfu2 - vfu2k);
				pvfu1++;
				pvfu2++;
				pvfv1++;
				pvfv2++;
				pvfdivp1++;
				pvfdivp2++;
			}

			for(int j = 0;j<16;j++){
				error += errorp[j];
				//printf("%f %f %d\n",errorp[j],error,size);
			}
			error/=size;




			// compute the gradient of the optical flow (Du1, Du2)
			forward_gradient(u1, u1x, u1y, nx ,ny);
			forward_gradient(u2, u2x, u2y, nx ,ny);

			// estimate the values of the dual variable (p1, p2)

			pvfu1x=(xb_vecN_2xf32*)u1x;
			pvfu1y=(xb_vecN_2xf32*)u1y;
			pvfu2x=(xb_vecN_2xf32*)u2x;
			pvfu2y=(xb_vecN_2xf32*)u2y;
			xb_vecN_2xf32 vfone = (float)1.0;
			xb_vecN_2xf32 vfminone = (float)-1.0;
			pvfpp11=(xb_vecN_2xf32*)p11;
			pvfpp12=(xb_vecN_2xf32*)p12;
			pvfpp21=(xb_vecN_2xf32*)p21;
			pvfpp22=(xb_vecN_2xf32*)p22;
			//printf("begin\n");
			int j = 0;
			for (int i = 0; i < size/16; i++)
			{
				float tmpvftaut[16];
				xb_vecN_2xf32 * vftaut = tmpvftaut;
				*vftaut = (float)(tau/theta);
				float tmpvfg12[16];
				xb_vecN_2xf32 * vfg12 = tmpvfg12;
				*vfg12 = *pvfu1x * *pvfu1x + *pvfu1y * *pvfu1y;
				float tmpvfg22[16];
				xb_vecN_2xf32 * vfg22 = tmpvfg22;
				*vfg22 = *pvfu2x * *pvfu2x + *pvfu2y * *pvfu2y;

				float tmppvfg1[16];
				xb_vecN_2xf32 * pvfg1 = tmppvfg1;
				*pvfg1 = IVP_SQRTN_2XF32(*vfg12);
				
				float tmppvfg2[16];
				xb_vecN_2xf32 * pvfg2 = tmppvfg2;
				*pvfg2 = IVP_SQRTN_2XF32(*vfg22);

				float tmpvfng1[16];
				xb_vecN_2xf32 * vfng1 = tmpvfng1;
				*vfng1 = vfone + *vftaut * *pvfg1;

				float tmpvfng2[16];
				xb_vecN_2xf32 * vfng2 = tmpvfng2;
				*vfng2 = vfone + *vftaut * *pvfg2;


				float tmpnewvfng1[16];
				xb_vecN_2xf32 * newvfng1 = tmpnewvfng1;
				*newvfng1 = IVP_DIV0N_2XF32(*vfng1); 

				float tmpnewvfng2[16];
				xb_vecN_2xf32 * newvfng2 = tmpnewvfng2;
				*newvfng2 = IVP_DIV0N_2XF32(*vfng2); 
				//IVP_DIVNN_2XF32(*pvfpp11,(*pvfpp11 + *vftaut * *pvfu1x),*newvfng1);
				//IVP_DIVNN_2XF32(*pvfpp12,(*pvfpp12 + vftaut * *pvfu1y),newvfng1);
				//IVP_DIVNN_2XF32(*pvfpp21,(*pvfpp21 + vftaut * *pvfu2x),newvfng2);
				//IVP_DIVNN_2XF32(*pvfpp22,(*pvfpp22 + vftaut * *pvfu2y),newvfng2);

				*pvfpp11 = (*pvfpp11 + *vftaut * *pvfu1x) * *newvfng1 ;
				*pvfpp12 = (*pvfpp12 + *vftaut * *pvfu1y) * *newvfng1 ;
				*pvfpp21 = (*pvfpp21 + *vftaut * *pvfu2x) * *newvfng2 ;
				*pvfpp22 = (*pvfpp22 + *vftaut * *pvfu2y) * *newvfng2 ;
				/*for(int z = 0;z<16;z++){
					printf("%f %f\n",tmpnewvfng1[z],tmpvfng1[z]);

				}*/
				//printf("%f\n",p11[0]);
				//printf("%f %f\n",p11[i],p12[i]);
				pvfpp11++;
				pvfpp12++;
				pvfpp21++;
				pvfpp22++;
				pvfu1x++;
				pvfu1y++;
				pvfu2x++;
				pvfu2y++;
				/*
				const float taut = tau / theta;
				const float g1   = hypot(u1x[i], u1y[i]);
				const float g2   = hypot(u2x[i], u2y[i]);
				const float ng1  = 1.0 + taut * g1;
				const float ng2  = 1.0 + taut * g2;
				if(j%16==0)
				//printf("taut%f  g1%f ng1%f newng1%f p11%f \n",taut,g1,ng1,1/ng1,p11[0]);
				j++;
				p11[i] = (p11[i] + taut * u1x[i]) / ng1;
				p12[i] = (p12[i] + taut * u1y[i]) / ng1;
				p21[i] = (p21[i] + taut * u2x[i]) / ng2;
				p22[i] = (p22[i] + taut * u2y[i]) / ng2;*/
			}
		}

		SVP_DSP_TIME_STAMP(cyclesStop);
    	totalCycles = (cyclesStop - cyclesStart);
    	printf("while  %llu\n",totalCycles);


		if (verbose)
			fprintf(stderr, "Warping: %d, "
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
	*min = *max = x[0];
	for (int i = 1; i < n; i++) {
		if (x[i] < *min)
			*min = x[i];
		if (x[i] > *max)
			*max = x[i];
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

	xb_vecN_2xf32 vfmin = min;
	xb_vecN_2xf32 vfden = (float)(1/den); //用常量对向量进行填充
	xb_vecN_2xf32 vf255 = (float)(255.0); 

	pvfresI0n = (xb_vecN_2xf32*)I0n;  //将浮点数指针转化为向量指针
	pvfresI1n = (xb_vecN_2xf32*)I1n;
	pvfresI0  = (xb_vecN_2xf32*)I0;
	pvfresI1  = (xb_vecN_2xf32*)I1;


	

	SVP_DSP_TIME_STAMP(cyclesStart);
	if (den > 0){
		for (int i = 0; i < size/16; i++)  //注意这里由于一次算16个数，所以循环次数要除以16
		{

			*pvfresI0n = (*pvfresI0-vfmin)*vf255*vfden; //向量化计算
			*pvfresI1n = (*pvfresI1-vfmin)*vf255*vfden;	

			pvfresI0++; 	//算完后向量指针移动，每次移动相当于跨过向量长度个数
			pvfresI1++;
			pvfresI0n++;
			pvfresI1n++;
		}
	}
	else{
		// copy the original images
		for (int i = 0; i < size/16; i++)
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
		const int   nxx,     // image width
		const int   nyy,     // image height
		const float tau,     // time step
		const float lambda,  // weight parameter for the data term
		const float theta,   // weight parameter for (u - v)²
		const int   nscales, // number of scales
		const float zfactor, // factor for building the image piramid
		const int   warps,   // number of warpings per scale
		const float epsilon, // tolerance for numerical convergence
		const bool  verbose  // enable/disable the verbose mode
)
{
		unsigned long long cyclesStart = 0;
	unsigned long long cyclesStop   = 0;
		unsigned long long totalCycles   = 0;

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

	SVP_DSP_TIME_STAMP(cyclesStart);
	// pre-smooth the original images
	gaussian(I0s[0], nx[0], ny[0], PRESMOOTHING_SIGMA);

	gaussian(I1s[0], nx[0], ny[0], PRESMOOTHING_SIGMA);


		SVP_DSP_TIME_STAMP(cyclesStop);
    	totalCycles = (cyclesStop - cyclesStart);
    	printf("gaussian  %llu\n",totalCycles);


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

	// initialize the flow at the coarsest scale
	for (int i = 0; i < nx[nscales-1] * ny[nscales-1]; i++)
		u1s[nscales-1][i] = u2s[nscales-1][i] = 0.0;

	// pyramidal structure for computing the optical flow
	for (int s = nscales-1; s >= 0; s--)
	{
		if (verbose)
			fprintf(stderr, "Scale %d: %dx%d\n", s, nx[s], ny[s]);

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