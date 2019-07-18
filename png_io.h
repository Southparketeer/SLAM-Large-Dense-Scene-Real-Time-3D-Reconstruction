#pragma once
#include "lodepng.h"
#include <assert.h>
#include <iostream>
#include "ppl.h"
class PNG_IO
{
public:
	static void  PNG_to_array_depth(const char * s_name, _Out_cap_c_(w * h) float* depth,  int w, int h)
	{
		std::vector<unsigned char> image; //the raw pixels
		unsigned width, height;
		unsigned error = lodepng::decode(image, width, height, s_name, LCT_GREY, 16);
		assert(width == w);
		assert(height == h);
		if(error)
		{
			std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
			return;
		}
		Concurrency::parallel_for( 0, w * h, [&](int k){
			float d = (float)(image[k * 2] * 256 + image[k * 2 + 1]);
			depth[k] = ( d  > 3000 || d  < 1000 ) ? 0 : d ;
		});
		return;
	}
};