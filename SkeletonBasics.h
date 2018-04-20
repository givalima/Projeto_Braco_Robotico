//------------------------------------------------------------------------------
// <copyright file="SkeletonBasics.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "resource.h"
#include "NuiApi.h"
#define IDC_BUTTON_SCREENSHOT           1011

#include "ImageRenderer.h"
#include <SFML/Network.hpp> 
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>


class CSkeletonBasics
{
    static const int        cScreenWidth  = 320;
    static const int        cScreenHeight = 240;

	static const int        cDepthWidth = 640;
	static const int        cDepthHeight = 480;
	static const int        cBytesPerPixel = 4;

    static const int        cStatusMessageMaxLen = MAX_PATH*2;

	static const int        cColorWidth = 640;
	static const int        cColorHeight = 480;


	float *rgbrunfloat;

public:
    /// <summary>
    /// Constructor
    /// </summary>
	CSkeletonBasics();

		/// <summary>
		/// Destructor
		/// </summary>
		~CSkeletonBasics();



    /// <summary>
    /// Handles window messages, passes most to the class instance to handle
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    static LRESULT CALLBACK MessageRouter(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// Handle windows messages for a class instance
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    LRESULT CALLBACK        DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// Creates the main window and begins processing
    /// </summary>
    /// <param name="hInstance"></param>
    /// <param name="nCmdShow"></param>
    int                     Run(HINSTANCE hInstance, int nCmdShow);
	sf::TcpSocket sock; ///////////////////// sock

	///////////////////////
	std::vector<float> ClassifyImgNet(sf::TcpSocket &sock, const std::vector<int> &im, cv::Size imSize);
	std::vector<float> vector_classifier;

private:
	cv::Mat imgToClassify;
	std::vector<int> imToClassifyInt;

    HWND                    m_hWnd;

    bool                    m_bSeatedMode;

	bool                    m_bSaveScreenshot;

    // Current Kinect
    INuiSensor*             m_pNuiSensor;

	BYTE*                   m_depthRGBX;
	float*                   m_depthRGBXfloat;

	HANDLE                  m_pDepthStreamHandle;
	HANDLE                  m_pColorStreamHandle;

    // Skeletal drawing
    ID2D1HwndRenderTarget*   m_pRenderTarget;
    ID2D1SolidColorBrush*    m_pBrushJointTracked;
    ID2D1SolidColorBrush*    m_pBrushJointInferred;
    ID2D1SolidColorBrush*    m_pBrushBoneTracked;
    ID2D1SolidColorBrush*    m_pBrushBoneInferred;
    D2D1_POINT_2F            m_Points[NUI_SKELETON_POSITION_COUNT];

	HANDLE                  m_hNextDepthFrameEvent;
	HANDLE                  m_hNextColorFrameEvent;

    // Direct2D
    ID2D1Factory*           m_pD2DFactory;
    
    HANDLE                  m_pSkeletonStreamHandle;
    HANDLE                  m_hNextSkeletonEvent;

	Vector4                 m_rightHandPosition;

	ImageRenderer*          m_pDrawDepth;
	ImageRenderer*          m_pDrawColor;
    
    /// <summary>
    /// Main processing function
    /// </summary>
    void                    Update();

    /// <summary>
    /// Create the first connected Kinect found 
    /// </summary>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 CreateFirstConnected();

    /// <summary>
    /// Handle new skeleton data
    /// </summary>
    void                    ProcessSkeleton();
	void					ProcessColor(int contador);

    /// <summary>
    /// Ensure necessary Direct2d resources are created
    /// </summary>
    /// <returns>S_OK if successful, otherwise an error code</returns>
    HRESULT                 EnsureDirect2DResources( );

    /// <summary>
    /// Dispose Direct2d resources 
    /// </summary>
    void                    DiscardDirect2DResources( );

    /// <summary>
    /// Draws a bone line between two joints
    /// </summary>
    /// <param name="skel">skeleton to draw bones from</param>
    /// <param name="joint0">joint to start drawing from</param>
    /// <param name="joint1">joint to end drawing at</param>
    void                    DrawBone(const NUI_SKELETON_DATA & skel, NUI_SKELETON_POSITION_INDEX bone0, NUI_SKELETON_POSITION_INDEX bone1);

    /// <summary>
    /// Draws a skeleton
    /// </summary>
    /// <param name="skel">skeleton to draw</param>
    /// <param name="windowWidth">width (in pixels) of output buffer</param>
    /// <param name="windowHeight">height (in pixels) of output buffer</param>
    void                    DrawSkeleton(const NUI_SKELETON_DATA & skel, int windowWidth, int windowHeight);

    /// <summary>
    /// Converts a skeleton point to screen space
    /// </summary>
    /// <param name="skeletonPoint">skeleton point to tranform</param>
    /// <param name="width">width (in pixels) of output buffer</param>
    /// <param name="height">height (in pixels) of output buffer</param>
    /// <returns>point in screen-space</returns>
    D2D1_POINT_2F           SkeletonToScreen(Vector4 skeletonPoint, int width, int height);


    /// <summary>
    /// Set the status bar message
    /// </summary>
    /// <param name="szMessage">message to display</param>
    void                    SetStatusMessage(WCHAR* szMessage);

	/// <summary>
	/// Save passed in image data to disk as a bitmap
	/// </summary>
	/// <param name="pBitmapBits">image data to save</param>
	/// <param name="lWidth">width (in pixels) of input image data</param>
	/// <param name="lHeight">height (in pixels) of input image data</param>
	/// <param name="wBitsPerPixel">bits per pixel of image data</param>
	/// <param name="lpszFilePath">full file path to output bitmap to</param>
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                 SaveBitmapToFile(BYTE* pBitmapBits, LONG lWidth, LONG lHeight, WORD wBitsPerPixel, LPCTSTR lpszFilePath);
};
