#define _USE_MATH_DEFINES
#include <iostream>
#include <math.h>
#include <time.h>
#include <cstdlib>
#include "CImg.h"

using namespace cimg_library;

#define SOME_TIME 10000

#ifndef cimg_imagepath
#define cimg_imagepath "img/"
#endif

#ifndef PI
 #ifdef M_PI
  #define PI M_PI
 #else
  #define PI 3.14159265358979
 #endif
#endif

#define COEFFS(Cu,Cv,u,v) { \
	if (u == 0) Cu = 1.0 / sqrt(2.0); else Cu = 1.0; \
	if (v == 0) Cv = 1.0 / sqrt(2.0); else Cv = 1.0; \
}

void dct_1d(double *in, double *out, const int count)
{
	int x, u;

	for (u=0; u<count; u++)
	{
		double z = 0;

		for (x=0; x<count; x++)
		{
			z += in[x] * cos(PI * (double)u * (double)(2*x+1)
				/ (double)(2*count));
		}

		if (u == 0) z *= 1.0 / sqrt(2.0);
		out[u] = z/2.0;
	}
}

void dct(const CImg<double> *image, double** blockBuffer,
	const int xpos, const int ypos, const int blockSize)
{
	int i,j;

	double* in = new double[blockSize];
	double* out = new double[blockSize];

	double **rows;
	rows = new double* [blockSize];
	for(int i=0;i<blockSize;i++)
		*(rows+i) = new double[blockSize];

	/* transform rows */
	for (j=0; j<blockSize; j++)
	{
		for (i=0; i<blockSize; i++)
			in[i] = *image->data(xpos+i,ypos+j,0);
		dct_1d(in, out, blockSize);
		for (i=0; i<blockSize; i++) rows[j][i] = out[i];
	}

	/* transform columns */
	for (j=0; j<blockSize; j++)
	{
		for (i=0; i<blockSize; i++)
			in[i] = rows[i][j];
		dct_1d(in, out, blockSize);
		for (i=0; i<blockSize; i++) blockBuffer[i][j] = out[i];
	}
}

void idct(CImg<double> *image, double** blockBuffer,
	const int xpos, const int ypos, const int blockSize)
{
	int u,v,x,y;

	for (y=0; y<blockSize; y++)
	for (x=0; x<blockSize; x++)
	{
		double z = 0.0;

		for (v=0; v<blockSize; v++)
		for (u=0; u<blockSize; u++)
		{
			double S, q;
			double Cu, Cv;
			
			COEFFS(Cu,Cv,u,v);
			S = blockBuffer[v][u];

			q = Cu * Cv * S *
				cos((double)(2*x+1) * (double)u * PI/(2*blockSize)) *
				cos((double)(2*y+1) * (double)v * PI/(2*blockSize));

			z += q;
		}

		switch(blockSize) {
		   case 2: z /= 0.25; break;
		   case 4: z /= 1.0; break;
		   case 8: z /= 4.0; break;
		   case 16: z /= 16.0; break;
		   case 32: z /= 64.0; break;
		  }
		// clipping
		if (z > 255.0) z = 255.0;
		if (z < 0) z = 0.0;

		*image->data(x+xpos,y+ypos,0) = z;
	}
}

void encrypt( double** blockBuffer, unsigned int blockSize, unsigned int diag, bool flag)
{
 unsigned int x,y;

 for (y=0; y<blockSize; y++) {
  for (x=0; x<blockSize; x++) {

   double a = rand() % 10;
   if(rand()%1) a = !a;

   if(flag) {
    if (x > diag || y > diag) blockBuffer[y][x] = blockBuffer[y][x] + a;
   } else {
    if (x < diag || y < diag) blockBuffer[y][x] = blockBuffer[y][x] + a;
   }
  }
 }
}

void attack( double** blockBuffer, unsigned int blockSize, unsigned int diag,bool flag) {
 
 unsigned int x,y;

 for (y=0; y<blockSize; y++) {
  for (x=0; x<blockSize; x++) {
   if(flag) {
    if (x < diag || y < diag) blockBuffer[y][x] = 0;
   } else {
    if (x > diag || y > diag) blockBuffer[y][x] = 0;
   }
  }
 }
}

int main(int argc,char **argv) {
	srand(time(NULL));

	cimg_usage("Parzielle Verschlüsselung...");

	int blockSize;
	int diag;
	int value;
	bool flag;

	std::cout<< "Geben Sie eine Blockgroesse ein (2, 4, 8, 16, 32) : ";
	std::cin>> blockSize;

	std::cout<< "diag: ";
	std::cin>> diag;

	std::cout<< "Oberhalb der Diagonale verschlüsseln? => 0 || 1 ";
	std::cin>> flag;

	value=0;

	// Load image
	CImg<double> image(cimg_imagepath "input.jpg");
	// Image to grayscale
	image.channel(0);
	// Resize Image depending on blockSize
	image.resize(image.width() - (image.width()%blockSize), image.height() - (image.height()%blockSize));

	image.save_jpeg(cimg_imagepath "original.jpg");

	CImg<double> originalImage(image);


	double **blockBuffer;

	blockBuffer = new double* [blockSize];
	for(int i=0;i<blockSize;i++)
		*(blockBuffer+i) = new double[blockSize];

	
	for (int y = 0; y < image.height(); y += blockSize){
		for (int x = 0; x < image.width(); x += blockSize) {
			dct(&image, blockBuffer, x, y, blockSize);
			encrypt(blockBuffer, blockSize, diag, flag);
			idct(&image, blockBuffer, x, y, blockSize);
		}
	}
	CImg<double> attackedImage(image);

		for (int y = 0; y < image.height(); y += blockSize){
		for (int x = 0; x < image.width(); x += blockSize) {
			dct(&attackedImage, blockBuffer, x, y, blockSize);
			attack(blockBuffer, blockSize, diag, flag);
			idct(&attackedImage, blockBuffer, x, y, blockSize);
		}
	}

	CImgDisplay original_disp(originalImage,"Original",0);
	CImgDisplay encrypted_disp(image,"Verschlüsselt",0);
	CImgDisplay attacked_disp(attackedImage,"Attack",0);
	
	image.save_jpeg(cimg_imagepath "encrypted.jpg");

	//Wait SOME_TIME before the Window close => defined on top
	cimg::wait(SOME_TIME);
    return 0;
}
