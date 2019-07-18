//#pragma once
//#include"StdAfx.h"
//
//class Depth
//{
//	static const int        cDepthWidth  = 512;
//	static const int        cDepthHeight = 424;
//public:
//	Depth();
//	~Depth();
//	void					Update(float* des, float c_near, float c_far);
//	HRESULT                 InitializeDefaultSensor();
//private:
//	void					ProcessDepth(const UINT16* pBuffer, int nWidth, int nHeight, float c_near, float c_far,float* des);
//	HWND                    m_hWnd;
//	INT64                   m_nStartTime;
//	INT64                   m_nLastCounter;
//	double                  m_fFreq;
//	INT64                   m_nNextStatusTime;
//	DWORD                   m_nFramesSinceUpdate;
//	bool                    m_bSaveScreenshot;
//
//	// Current Kinect
//	IKinectSensor*          m_pKinectSensor;
//
//	// Depth reader
//	IDepthFrameReader*      m_pDepthFrameReader;
//	RGBQUAD*                m_pDepthRGBX;
//};
//
//
//Depth::Depth():
//	m_hWnd(NULL),
//	m_nStartTime(0),
//	m_nLastCounter(0),
//	m_nFramesSinceUpdate(0),
//	m_fFreq(0),
//	m_nNextStatusTime(0LL),
//	m_bSaveScreenshot(false),
//	m_pKinectSensor(NULL),
//	m_pDepthFrameReader(NULL),
//	m_pDepthRGBX(NULL)
//{
//	LARGE_INTEGER qpf = {0};
//	if (QueryPerformanceFrequency(&qpf))
//	{
//		m_fFreq = double(qpf.QuadPart);
//	}
//
//	// create heap storage for depth pixel data in RGBX format
//	m_pDepthRGBX = new RGBQUAD[cDepthWidth * cDepthHeight];
//}
//
//
//Depth::~Depth()
//{
//	if (m_pDepthRGBX)
//	{
//		delete [] m_pDepthRGBX;
//		m_pDepthRGBX = NULL;
//	}
//	SafeRelease(m_pDepthFrameReader);
//	if (m_pKinectSensor)
//	{
//		m_pKinectSensor->Close();
//	}
//
//	SafeRelease(m_pKinectSensor);
//}
//
//void Depth::Update(float* des, float c_near, float c_far)
//{
//	if (!m_pDepthFrameReader)
//	{
//		return;
//	}
//
//	IDepthFrame* pDepthFrame = NULL;
//
//	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
//
//	if (SUCCEEDED(hr))
//	{
//		INT64 nTime = 0;
//		IFrameDescription* pFrameDescription = NULL;
//		int nWidth = 0;
//		int nHeight = 0;
//		USHORT nDepthMinReliableDistance = 0;
//		USHORT nDepthMaxDistance = 0;
//		UINT nBufferSize = 0;
//		UINT16 *pBuffer = NULL;
//
//		hr = pDepthFrame->get_RelativeTime(&nTime);
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pFrameDescription->get_Width(&nWidth);
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pFrameDescription->get_Height(&nHeight);
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			nDepthMaxDistance = USHRT_MAX;
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);            
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			ProcessDepth(pBuffer, nWidth, nHeight, c_near, c_far, des);
//		}
//
//		SafeRelease(pFrameDescription);
//	}
//
//	SafeRelease(pDepthFrame);
//}
//
//
//HRESULT Depth::InitializeDefaultSensor()
//{
//	HRESULT hr;
//
//	hr = GetDefaultKinectSensor(&m_pKinectSensor);
//	if (FAILED(hr))
//	{
//		return hr;
//	}
//
//	if (m_pKinectSensor) 
//	{
//		// Initialize the Kinect and get the depth reader
//		IDepthFrameSource* pDepthFrameSource = NULL;
//
//		hr = m_pKinectSensor->Open();
//
//		if (SUCCEEDED(hr))
//		{
//			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
//		}
//
//		if (SUCCEEDED(hr))
//		{
//			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
//		}
//
//		SafeRelease(pDepthFrameSource);
//	}
//
//	if (!m_pKinectSensor || FAILED(hr))
//	{
//		std::cout<<"No ready Kinect found!"<<std::endl;
//		return E_FAIL;
//	}
//
//	return hr;
//}
//
//
//void Depth::ProcessDepth(const UINT16* pBuffer, int nWidth, int nHeight, float c_near, float c_far,float* des)
//{
//	// Make sure we've received valid data
//	if (m_pDepthRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
//	{
//		Concurrency::parallel_for(0, nWidth * nHeight, [&](int k) {
//			float d = (float) *(pBuffer + k);
//			*(des + k) = ( d > c_far || d < c_near ) ? 0 : d;
//		});
//	}
//}
//
//
