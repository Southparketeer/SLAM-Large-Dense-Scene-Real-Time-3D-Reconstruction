#include "EasyBMP.h"
#include <cuda_runtime.h>

typedef unsigned char BYTE;

void array2bmp(int imageW, int imageH, float* imagesor, std::string name, float scale = 1)
{
	std::cout<<"_------------------------------------"<<std::endl;
	BMP bmp;
	bmp.SetSize(imageW,imageH);
	bmp.SetBitDepth(32);
	RGBApixel Zero;
	Zero.Red = 0;
	Zero.Blue = 0;
	Zero.Green = 0;
	Zero.Alpha = 0;
	for(int y=0; y<imageH ; y++){
		for(int x=0; x<imageW ; x++){

			int base=y*imageW+x;
			if(imagesor[base] < 0.0001) {bmp.SetPixel(x,y,Zero); continue;}
			int d= (int) (imagesor[base] * scale);
			RGBApixel pix_in;
			pix_in.Red= d;
			pix_in.Green= d;
			pix_in.Blue= d;
			pix_in.Alpha=255;
			bmp.SetPixel(x,y,pix_in);

		}
	}
	name.insert(name.size(),".bmp");
	bmp.WriteToFile(name.c_str());
	std::cout<<name.c_str()<<"has been created !"<<std::endl;
}

void array2bmpNorm(int imageW, int imageH, float4* imagesor, std::string name, bool is_normal)
{
	BMP bmp;
	bmp.SetSize(imageW,imageH);
	bmp.SetBitDepth(32);
	for(int y=0; y<imageH ; y++){
		for(int x=0; x<imageW ; x++){
			int base=y*imageW+x;
			RGBApixel pix_in;
			if(is_normal)
			{
				pix_in.Red = (BYTE)(fabsf(imagesor[base].x)* 255);
				pix_in.Green =(BYTE)(fabsf(imagesor[base].y)* 255);
				pix_in.Blue = (BYTE)(fabsf(imagesor[base].z* 255));
				pix_in.Alpha=255;
			}
			else
			{
				pix_in.Red = (BYTE)(fabsf(imagesor[base].x));
				pix_in.Green =(BYTE)(fabsf(imagesor[base].y));
				pix_in.Blue = (BYTE)(fabsf(imagesor[base].z));
				pix_in.Alpha=255;
			}
			bmp.SetPixel(x,y,pix_in);
		}
	}
	name.insert(name.size(),".bmp");
	bmp.WriteToFile(name.c_str());
	std::cout<<name.c_str()<<"has been created !"<<std::endl;
}