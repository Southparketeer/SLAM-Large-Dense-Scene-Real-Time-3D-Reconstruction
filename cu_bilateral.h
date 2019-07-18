#pragma once 
#include "Image.h"
typedef unsigned int uint;
namespace loo
{
	//template<typename To, typename Ti>
	// __declspec(dllexport)
	//void BilateralFilter( Image<To> dOut, const Image<Ti> dIn, float gs, float gr, uint size);
	template<typename To, typename Ti>
	__declspec(dllexport)
		void BilateralFilter( Image<To> dOut, const Image<Ti> dIn, float gs, float gr, uint size);

	template<typename To, typename Ti>
	__declspec(dllexport)
		void BilateralFilter(Image<To> dOut, const Image<Ti> dIn, float gs, float gr, uint size, Ti minval);

	template<typename To, typename Ti, typename Ti2>
	__declspec(dllexport)
		void BilateralFilter(Image<To> dOut, const Image<Ti> dIn, const Image<Ti2> dImg, float gs, float gr, float gc, uint size);
}
