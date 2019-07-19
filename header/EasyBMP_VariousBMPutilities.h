#ifndef _EasyBMP_VariousBMPutilities_h_
#define _EasyBMP_VariousBMPutilities_h_

BMFH GetBMFH( const char* szFileNameIn );
BMIH GetBMIH( const char* szFileNameIn );
void DisplayBitmapInfo( const char* szFileNameIn );
int GetBitmapColorDepth( const char* szFileNameIn );
void PixelToPixelCopy( BMP& From, int FromX, int FromY,  
					  BMP& To, int ToX, int ToY);
void PixelToPixelCopyTransparent( BMP& From, int FromX, int FromY,  
								 BMP& To, int ToX, int ToY,
								 RGBApixel& Transparent );
void RangedPixelToPixelCopy( BMP& From, int FromL , int FromR, int FromB, int FromT, 
							BMP& To, int ToX, int ToY );
void RangedPixelToPixelCopyTransparent( 
	BMP& From, int FromL , int FromR, int FromB, int FromT, 
	BMP& To, int ToX, int ToY ,
	RGBApixel& Transparent );
bool CreateGrayscaleColorTable( BMP& InputImage );

bool Rescale( BMP& InputImage , char mode, int NewDimension );

#endif