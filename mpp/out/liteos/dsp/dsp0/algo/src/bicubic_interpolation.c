// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2012, Javier Sánchez Pérez <jsanchez@dis.ulpgc.es>
// All rights reserved.


#ifndef BICUBIC_INTERPOLATION_C
#define BICUBIC_INTERPOLATION_C
#include "hi_dsp.h"
#include "svp_dsp_frm.h"
#include "svp_dsp_def.h"
#include "svp_dsp_tile.h"
#include "svp_dsp_tm.h"
#include "svp_dsp_performance.h"
#include "svp_dsp_trace.h"
#include <stdbool.h>

#define BOUNDARY_CONDITION 0
//0 Neumann
//1 Periodic
//2 Symmetric

/**
  *
  * Neumann boundary condition test
  *
**/
static int neumann_bc(int x, int nx, bool *out)
{
	if(x < 0)
	{
	    x = 0;
	    *out = true;
	}
	else if (x >= nx)
	{
	    x = nx - 1;
	    *out = true;
	}

	return x;
}
static int neumann_bc_vec(int x, int nx)
{
	if(x < 0)
	{
	    x = 0;
	}
	else if (x >= nx)
	{
	    x = nx - 1;
	}

	return x;
}

/**
  *
  * Cubic interpolation in one dimension
  *
**/
static float cubic_interpolation_cell (
	float v[4],  //interpolation points
	float x      //point to be interpolated
)
{
	return  v[1] + 0.5 * x * (v[2] - v[0] +
		x * (2.0 *  v[0] - 5.0 * v[1] + 4.0 * v[2] - v[3] +
		x * (3.0 * (v[1] - v[2]) + v[3] - v[0])));
}


/**
  *
  * Bicubic interpolation in two dimensions
  *
**/
static float bicubic_interpolation_cell (
	float p[4][4], //array containing the interpolation points
	float x,       //x position to be interpolated
	float y        //y position to be interpolated
)
{
	float v[4];
	v[0] = cubic_interpolation_cell(p[0], y);
	v[1] = cubic_interpolation_cell(p[1], y);
	v[2] = cubic_interpolation_cell(p[2], y);
	v[3] = cubic_interpolation_cell(p[3], y);
	return cubic_interpolation_cell(v, x);
}

/**
  *
  * Compute the bicubic interpolation of a point in an image.
  * Detect if the point goes outside the image domain.
  *
**/
float bicubic_interpolation_at(
	const float *input, //image to be interpolated
	const float  uu,    //x component of the vector field
	const float  vv,    //y component of the vector field
	const int    nx,    //image width
	const int    ny,    //image height
	bool         border_out //if true, return zero outside the region
)
{
	const int sx = (uu < 0)? -1: 1;
	const int sy = (vv < 0)? -1: 1;

	int x, y, mx, my, dx, dy, ddx, ddy;
	bool out[1] = {false};

 			x   = neumann_bc((int) uu, nx, out);
			y   = neumann_bc((int) vv, ny, out);
			mx  = neumann_bc((int) uu - sx, nx, out);
			my  = neumann_bc((int) vv - sx, ny, out);
			dx  = neumann_bc((int) uu + sx, nx, out);
			dy  = neumann_bc((int) vv + sy, ny, out);
			ddx = neumann_bc((int) uu + 2*sx, nx, out);
			ddy = neumann_bc((int) vv + 2*sy, ny, out);


		//obtain the interpolation points of the image
		const float p11 = input[mx  + nx * my];
		const float p12 = input[x   + nx * my];
		const float p13 = input[dx  + nx * my];
		const float p14 = input[ddx + nx * my];

		const float p21 = input[mx  + nx * y];
		const float p22 = input[x   + nx * y];
		const float p23 = input[dx  + nx * y];
		const float p24 = input[ddx + nx * y];

		const float p31 = input[mx  + nx * dy];
		const float p32 = input[x   + nx * dy];
		const float p33 = input[dx  + nx * dy];
		const float p34 = input[ddx + nx * dy];

		const float p41 = input[mx  + nx * ddy];
		const float p42 = input[x   + nx * ddy];
		const float p43 = input[dx  + nx * ddy];
		const float p44 = input[ddx + nx * ddy];

		//create array
		float pol[4][4] = {
			{p11, p21, p31, p41},
			{p12, p22, p32, p42},
			{p13, p23, p33, p43},
			{p14, p24, p34, p44}
		};

		//return interpolation
		return bicubic_interpolation_cell(pol, uu-x, vv-y);
	
}

/**
  *
  * Compute the bicubic interpolation of an image.
  *
**/
/*
static xb_vecN_2xf32 cubic_interpolation_cell_vec (
	xb_vecN_2xf32 * v0,  //interpolation points
	xb_vecN_2xf32 * v1,      //point to be interpolated
	xb_vecN_2xf32 * v2,
	xb_vecN_2xf32 * v3,
	xb_vecN_2xf32 * x
)
{
	printf("aaa");
	xb_vecN_2xf32  pvf05 = (float)0.5;
	xb_vecN_2xf32  pvf20 = (float)2.0;
	xb_vecN_2xf32  pvf50 = (float)5.0;
	xb_vecN_2xf32  pvf40 = (float)4.0;
	xb_vecN_2xf32  pvf30 = (float)3.0;
	xb_vecN_2xf32  pvfresult;
	pvfresult = *v1 + pvf05 * *x * (*v2 - *v0 + *x * (pvf20 *  *v0 - pvf50 * *v1 + pvf40 * *v2 - *v3 + *x * (pvf30 * (*v1 - *v2) + *v3 - *v0)));
	return pvfresult;
}
*/
void bicubic_interpolation_warp(
	const float *input,     // image to be warped
	const float *u,         // x component of the vector field
	const float *v,         // y component of the vector field
	float       *output,    // image warped with bicubic interpolation
	const int    nx,        // image width
	const int    ny,        // image height
	bool         border_out // if true, put zeros outside the region
)
{/*
	xb_vecN_2xf32 *  pvfresoutput;
	pvfresoutput = (xb_vecN_2xf32 *)output;
	xb_vecN_2xf32 *  pvfp11;	
	xb_vecN_2xf32 *  pvfp12;
	xb_vecN_2xf32 *  pvfp13;
	xb_vecN_2xf32 *  pvfp14;
	xb_vecN_2xf32 *  pvfp21;
	xb_vecN_2xf32 *  pvfp22;
	xb_vecN_2xf32 *  pvfp23;
	xb_vecN_2xf32 *  pvfp24;
	xb_vecN_2xf32 *  pvfp31;	
	xb_vecN_2xf32 *  pvfp32;
	xb_vecN_2xf32 *  pvfp33;
	xb_vecN_2xf32 *  pvfp34;
	xb_vecN_2xf32 *  pvfp41;
	xb_vecN_2xf32 *  pvfp42;
	xb_vecN_2xf32 *  pvfp43;
	xb_vecN_2xf32 *  pvfp44;

	float p11[16];	
	float p12[16];
	float p13[16];
	float p14[16];
	float p21[16];
	float p22[16];
	float p23[16];
	float p24[16];
	float p31[16];
	float p32[16];
	float p33[16];
	float p34[16];
	float p41[16];
	float p42[16];
	float p43[16];
	float p44[16];

	pvfp11=(xb_vecN_2xf32*)p11;
	pvfp12=(xb_vecN_2xf32*)p12;
	pvfp13=(xb_vecN_2xf32*)p13;
	pvfp14=(xb_vecN_2xf32*)p14;
	pvfp21=(xb_vecN_2xf32*)p21;
	pvfp22=(xb_vecN_2xf32*)p22;
	pvfp23=(xb_vecN_2xf32*)p23;
	pvfp24=(xb_vecN_2xf32*)p24;
	pvfp31=(xb_vecN_2xf32*)p31;
	pvfp32=(xb_vecN_2xf32*)p32;
	pvfp33=(xb_vecN_2xf32*)p33;
	pvfp34=(xb_vecN_2xf32*)p34;
	pvfp41=(xb_vecN_2xf32*)p41;
	pvfp42=(xb_vecN_2xf32*)p42;
	pvfp43=(xb_vecN_2xf32*)p43;
	pvfp44=(xb_vecN_2xf32*)p44;
	
	xb_vecN_2xf32  pvfv1;
	xb_vecN_2xf32  pvfv2;
	xb_vecN_2xf32  pvfv3;
	xb_vecN_2xf32  pvfv4;

	float vv_y;
	float uu_x;
	
	xb_vecN_2xf32  pvf05 = (float)0.5;
	xb_vecN_2xf32  pvf20 = (float)2.0;
	xb_vecN_2xf32  pvf50 = (float)5.0;
	xb_vecN_2xf32  pvf40 = (float)4.0;
	xb_vecN_2xf32  pvf30 = (float)3.0;
	xb_vecN_2xf32  pvfvv_y ;
	xb_vecN_2xf32  pvfuu_x ;


	for(int i = 0; i < ny; i++)
		for(int j = 0; j < nx/16; j++)
		{
			for(int k = 0;k<16;k++){
				int p = i * nx + j * 16 + k;
				float uu = (float)(j*16+k+u[p]);
				float vv = (float)(i+v[p]);

				int sx = (uu < 0)? -1: 1;
				int sy = (vv < 0)? -1: 1;
				int x, y, mx, my, dx, dy, ddx, ddy;

						
				x   = neumann_bc_vec((int) uu, nx);
				y   = neumann_bc_vec((int) vv, ny);
				mx  = neumann_bc_vec((int) uu - sx, nx);
				my  = neumann_bc_vec((int) vv - sy, ny);
				dx  = neumann_bc_vec((int) uu + sx, nx);
				dy  = neumann_bc_vec((int) vv + sy, ny);
				ddx = neumann_bc_vec((int) uu + 2*sx, nx);
				ddy = neumann_bc_vec((int) vv + 2*sy, ny);
				vv_y = vv-y;
				uu_x = uu-x;
				p11[k] = input[mx  + nx * my];
				p12[k] = input[x   + nx * my];
				p13[k] = input[dx  + nx * my];
				p14[k] = input[ddx + nx * my];

				p21[k] = input[mx  + nx * y];
				p22[k] = input[x   + nx * y];
				p23[k] = input[dx  + nx * y];
				p24[k] = input[ddx + nx * y];

				p31[k] = input[mx  + nx * dy];
				p32[k] = input[x   + nx * dy];
				p33[k] = input[dx  + nx * dy];
				p34[k] = input[ddx + nx * dy];

				p41[k] = input[mx  + nx * ddy];
				p42[k] = input[x   + nx * ddy];
				p43[k] = input[dx  + nx * ddy];
				p44[k] = input[ddx + nx * ddy];
			}		
			pvfvv_y = (float)vv_y;
			pvfuu_x = (float)uu_x;
			pvfv1 = *pvfp21 + pvf05 * pvfvv_y * (*pvfp31 - *pvfp11 + 
			pvfvv_y * (pvf20 *  *pvfp11 - pvf50 * *pvfp21 + pvf40 * *pvfp31 - *pvfp41 + 
			pvfvv_y * (pvf30 * (*pvfp21 - *pvfp31) + *pvfp41 - *pvfp11)));

			pvfv2 = *pvfp22 + pvf05 * pvfvv_y * (*pvfp32 - *pvfp12 + 
			pvfvv_y * (pvf20 *  *pvfp12 - pvf50 * *pvfp22 + pvf40 * *pvfp32 - *pvfp42 + 
			pvfvv_y * (pvf30 * (*pvfp22 - *pvfp32) + *pvfp42 - *pvfp12)));

			pvfv3 = *pvfp23 + pvf05 * pvfvv_y * (*pvfp33 - *pvfp13 + 
			pvfvv_y * (pvf20 *  *pvfp13 - pvf50 * *pvfp23 + pvf40 * *pvfp33 - *pvfp43 + 
			pvfvv_y * (pvf30 * (*pvfp23 - *pvfp33) + *pvfp43 - *pvfp13)));

			pvfv4 = *pvfp24 + pvf05 * pvfvv_y * (*pvfp34 - *pvfp14 + 
			pvfvv_y * (pvf20 *  *pvfp14 - pvf50 * *pvfp24 + pvf40 * *pvfp34 - *pvfp44 + 
			pvfvv_y * (pvf30 * (*pvfp24 - *pvfp34) + *pvfp44 - *pvfp14)));

			*pvfresoutput = pvfv2 + pvf05 * pvfuu_x * (pvfv3 - pvfv1 + 
			pvfuu_x * (pvf20 *  pvfv1 - pvf50 * pvfv2 + pvf40 * pvfv3 - pvfv4 + 
			pvfuu_x * (pvf30 * (pvfv2 - pvfv3) + pvfv4 - pvfv1)));
			pvfresoutput++;
			*/
/*	return  v[1] + 0.5 * x * (v[2] - v[0] +
		x * (2.0 *  v[0] - 5.0 * v[1] + 4.0 * v[2] - v[3] +
		x * (3.0 * (v[1] - v[2]) + v[3] - v[0])));	*/		


	/*		const int   p  = i * nx + j;
			xb_vecN_2xf32 * __restrict pvfresuu;
			xb_vecN_2xf32 * __restrict pvfresvv;
			float resj[16];
			int ressx[16];
			int ressy[16];
			for(int k = 0;k<16;k++){
				resj[k] = (float)(j+k);
				const float uu = (float)(j+k+u[p]);
				const float vv = (float)(i+v[p]);
				ressx[k] = (uu < 0)? -1: 1;
				ressy[k] = (vv < 0)? -1: 1;
			}
			xb_vecN_2xf32 * __restrict pvfresj;
			pvfresj = (xb_vecN_2xf32 *) resj;
			xb_vecN_2xf32 pvfresi = (float)i;
			*pvfresuu = *pvfresj +*pvfresu;
			*pvfresvv = pvfresi +*pvfresv;

		
			pvfresu++;
			pvfresv++;


			xb_vecN_2x16 * __restrict intressx;
			xb_vecN_2x16 * __restrict intressy;
			intressx = (xb_vecN_2x16 *)ressx;
			intressy = (xb_vecN_2x16 *)ressy;

			xb_vecN_2x16 intresnx = nx;
			xb_vecN_2x16 intresny = ny;

			xb_vecN_2x16 * __restrict intresx;
			xb_vecN_2x16 * __restrict intresy;
			xb_vecN_2x16 * __restrict intresmx;
			xb_vecN_2x16 * __restrict intresmy;
			xb_vecN_2x16 * __restrict intresdx;
			xb_vecN_2x16 * __restrict intresdy;
			xb_vecN_2x16 * __restrict intresddx;
			xb_vecN_2x16 * __restrict intresddy;

			*intresx = *(xb_vecN_2x16 *)pvfresuu;
			*intresy = *(xb_vecN_2x16 *)pvfresvv;
			*intresmx = *(xb_vecN_2x16 *)pvfresuu -*intressx;
			*intresmy = *(xb_vecN_2x16 *)pvfresvv -*intressx;
			*intresdx = *(xb_vecN_2x16 *)pvfresuu +*intressx; 		
			*intresdy = *(xb_vecN_2x16 *)pvfresvv +*intressy;
			*intresddx = *(xb_vecN_2x16 *)pvfresuu +*intressx; 
			*intresddy = *(xb_vecN_2x16 *)pvfresvv +*intressy;

			xb_vecN_2xf32 * __restrict pvfp11;	
			xb_vecN_2xf32 * __restrict pvfp12;
			xb_vecN_2xf32 * __restrict pvfp13;
			xb_vecN_2xf32 * __restrict pvfp14;
			xb_vecN_2xf32 * __restrict pvfp21;
			xb_vecN_2xf32 * __restrict pvfp22;
			xb_vecN_2xf32 * __restrict pvfp23;
			xb_vecN_2xf32 * __restrict pvfp24;
			xb_vecN_2xf32 * __restrict pvfp31;	
			xb_vecN_2xf32 * __restrict pvfp32;
			xb_vecN_2xf32 * __restrict pvfp33;
			xb_vecN_2xf32 * __restrict pvfp34;
			xb_vecN_2xf32 * __restrict pvfp41;
			xb_vecN_2xf32 * __restrict pvfp42;
			xb_vecN_2xf32 * __restrict pvfp43;
			xb_vecN_2xf32 * __restrict pvfp44;	

			xb_vecN_2x16 * __restrict intp11;	
			xb_vecN_2x16 * __restrict intp12;
			xb_vecN_2x16 * __restrict intp13;
			xb_vecN_2x16 * __restrict intp14;
			xb_vecN_2x16 * __restrict intp21;
			xb_vecN_2x16 * __restrict intp22;
			xb_vecN_2x16 * __restrict intp23;
			xb_vecN_2x16 * __restrict intp24;
			xb_vecN_2x16 * __restrict intp31;	
			xb_vecN_2x16 * __restrict intp32;
			xb_vecN_2x16 * __restrict intp33;
			xb_vecN_2x16 * __restrict intp34;
			xb_vecN_2x16 * __restrict intp41;
			xb_vecN_2x16 * __restrict intp42;
			xb_vecN_2x16 * __restrict intp43;
			xb_vecN_2x16 * __restrict intp44;	
*/
            
	/*
			// obtain the bicubic interpolation at position (uu, vv)
	for(int i = 0; i < ny; i++)
		for(int j = 0; j < nx; j++)
		{
			const int   p  = i * nx + j;
			const float uu = (float) (j + u[p]);
			const float vv = (float) (i + v[p]);
				const int sx = (uu < 0)? -1: 1;
				const int sy = (vv < 0)? -1: 1;
				int x, y, mx, my, dx, dy, ddx, ddy;
				bool out[1] = {false};

						x   = neumann_bc((int) uu, nx, out);
						y   = neumann_bc((int) vv, ny, out);
						mx  = neumann_bc((int) uu - sx, nx, out);
						my  = neumann_bc((int) vv - sx, ny, out);
						dx  = neumann_bc((int) uu + sx, nx, out);
						dy  = neumann_bc((int) vv + sy, ny, out);
						ddx = neumann_bc((int) uu + 2*sx, nx, out);
						ddy = neumann_bc((int) vv + 2*sy, ny, out);
					//obtain the interpolation points of the image
					const float p11 = input[mx  + nx * my];
					const float p12 = input[x   + nx * my];
					const float p13 = input[dx  + nx * my];
					const float p14 = input[ddx + nx * my];

					const float p21 = input[mx  + nx * y];
					const float p22 = input[x   + nx * y];
					const float p23 = input[dx  + nx * y];
					const float p24 = input[ddx + nx * y];

					const float p31 = input[mx  + nx * dy];
					const float p32 = input[x   + nx * dy];
					const float p33 = input[dx  + nx * dy];
					const float p34 = input[ddx + nx * dy];

					const float p41 = input[mx  + nx * ddy];
					const float p42 = input[x   + nx * ddy];
					const float p43 = input[dx  + nx * ddy];
					const float p44 = input[ddx + nx * ddy];

					//create array
					double pol[4][4] = {
						{p11, p21, p31, p41},
						{p12, p22, p32, p42},
						{p13, p23, p33, p43},
						{p14, p24, p34, p44}
					};

					//return interpolation
					//output[p] = bicubic_interpolation_cell(pol, uu-x, vv-y);
						double v[4];
						v[0] = cubic_interpolation_cell(pol[0], vv-y);
						v[1] = cubic_interpolation_cell(pol[1], vv-y);
						v[2] = cubic_interpolation_cell(pol[2], vv-y);
						v[3] = cubic_interpolation_cell(pol[3], vv-y);
						output[p] =  cubic_interpolation_cell(v, uu-x);
				*/


						for(int i = 0; i < ny; i++)
							for(int j = 0; j < nx; j++)
							{
								const int   p  = i * nx + j;
								const float uu = (float) (j + u[p]);
								const float vv = (float) (i + v[p]);

								// obtain the bicubic interpolation at position (uu, vv)
								output[p] = bicubic_interpolation_at(input,
										uu, vv, nx, ny, border_out);
							
						

		}
}

#endif//BICUBIC_INTERPOLATION_C