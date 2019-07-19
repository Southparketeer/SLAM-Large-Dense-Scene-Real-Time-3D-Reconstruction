// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__DB2BE556_32C9_4D3A_B33D_D876464739B1__INCLUDED_)
#define AFX_STDAFX_H__DB2BE556_32C9_4D3A_B33D_D876464739B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>
#include <Kinect/Kinect.h>
#include <Shlobj.h>
#include <iostream>
#include "ppl.h"
// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__DB2BE556_32C9_4D3A_B33D_D876464739B1__INCLUDED_)

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
    if (pInterfaceToRelease != NULL)
    {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = NULL;
    }
}