// <copyright file="SkeletonBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <strsafe.h>
#include "SkeletonBasics.h"
#include "resource.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <SFML/Network.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

static const float g_JointThickness = 3.0f;
static const float g_TrackedBoneThickness = 6.0f;
static const float g_InferredBoneThickness = 1.0f;
int count = 1;

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    CSkeletonBasics application;
    application.Run(hInstance, nCmdShow);
}

/// <summary>
/// Constructor
/// </summary>
CSkeletonBasics::CSkeletonBasics() :
    m_pD2DFactory(NULL),
    m_hNextSkeletonEvent(INVALID_HANDLE_VALUE),
    m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE),
    m_bSeatedMode(false),
    m_pRenderTarget(NULL),
    m_pBrushJointTracked(NULL),
    m_pBrushJointInferred(NULL),
    m_pBrushBoneTracked(NULL),
    m_pBrushBoneInferred(NULL),
	m_hNextDepthFrameEvent(INVALID_HANDLE_VALUE),
	m_pDepthStreamHandle(INVALID_HANDLE_VALUE),
    m_pNuiSensor(NULL),
	m_bSaveScreenshot(false),
	m_pColorStreamHandle(INVALID_HANDLE_VALUE),
	m_pDrawColor(NULL)
{
    ZeroMemory(m_Points,sizeof(m_Points));
	m_depthRGBX = new BYTE[cDepthWidth*cDepthHeight*cBytesPerPixel];
	rgbrunfloat = new float[cDepthWidth*cDepthHeight*cBytesPerPixel];
	imgToClassify = cv::Mat{ 640, 480, CV_8UC1 };
	imToClassifyInt.resize(50 * 50);

	sf::Socket::Status status = sock.connect("127.0.0.1", 31000);
	if (status != sf::Socket::Done) {
		std::cout << "erro on connect \n";
	}
}

/// <summary>
/// Destructor
/// </summary>
CSkeletonBasics::~CSkeletonBasics()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hNextSkeletonEvent);
    }

    // clean up Direct2D objects
    DiscardDirect2DResources();

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    SafeRelease(m_pNuiSensor);

	// clean up Direct2D renderer
	delete m_pDrawColor;
	m_pDrawColor = NULL;

}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CSkeletonBasics::Run(HINSTANCE hInstance, int nCmdShow)
{
    MSG       msg = {0};
    WNDCLASS  wc  = {0};

    // Dialog custom window class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"SkeletonBasicsAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        hInstance,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CSkeletonBasics::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    const int eventCount = 1;
    HANDLE hEvents[eventCount];

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        hEvents[0] = m_hNextSkeletonEvent;

        // Check to see if we have either a message (by passing in QS_ALLEVENTS)
        // Or a Kinect event (hEvents)
        // Update() will check for Kinect events individually, in case more than one are signalled
        DWORD dwEvent = MsgWaitForMultipleObjects(eventCount, hEvents, FALSE, INFINITE, QS_ALLINPUT);

        // Check if this is an event we're waiting on and not a timeout or message
        if (WAIT_OBJECT_0 == dwEvent)
        {
            Update();
        }

        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if ((hWndApp != NULL) && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Main processing function
/// </summary>
void CSkeletonBasics::Update()
{
    if (NULL == m_pNuiSensor)
    {
        return;
    }

    // Wait for 0ms, just quickly test if it is time to process a skeleton
    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0) )
    {
        ProcessSkeleton();
    }

	/*if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextColorFrameEvent, 0))
	{
		ProcessColor(count);
	}*/

//	m_pDrawDepth->DetectHandRight();
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CSkeletonBasics::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSkeletonBasics* pThis = NULL;

    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CSkeletonBasics*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CSkeletonBasics*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CSkeletonBasics::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

			// Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
			// We'll use this to draw the data we receive from the Kinect to the screen
			m_pDrawDepth = new ImageRenderer();
			HRESULT hr_new = m_pDrawDepth->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_pD2DFactory, cDepthWidth, cDepthHeight, cDepthWidth * sizeof(long));
			
			if (FAILED(hr_new))
			{
				SetStatusMessage(L"Failed to initialize the Direct2D draw device.");
			}

			m_pDrawColor = new ImageRenderer();
			HRESULT hr = m_pDrawColor->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(long));

			if(FAILED(hr))
			{
				SetStatusMessage(L"Failed to initialize the Direct2D draw device.");
			}

            // Look for a connected Kinect, and create it if found
            CreateFirstConnected();
        }
        break;

        // If the titlebar X is clicked, destroy app
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // Quit the main message pump
        PostQuitMessage(0);
        break;

	/*
        // Handle button press
    case WM_COMMAND:
        // If it was for the near mode control and a clicked event, change near mode
        if (IDC_CHECK_SEATED == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
        {
            // Toggle out internal state for near mode
            m_bSeatedMode = !m_bSeatedMode;

            if (NULL != m_pNuiSensor)
            {
                // Set near mode for sensor based on our internal state
                m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, m_bSeatedMode ? NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT : 0);
            }
        }
        break;
    */

	//Implementando botão de print
	// Handle button press
    case WM_COMMAND:
			// If it was for the screenshot control and a button clicked event, save a screenshot next frame 
			if (IDC_BUTTON_SCREENSHOT == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
			{
				m_bSaveScreenshot = true;
			}
			break;
}

    return FALSE;
}

/// <summary>
/// Create the first connected Kinect found 
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CSkeletonBasics::CreateFirstConnected()
{
    INuiSensor * pNuiSensor;

    int iSensorCount = 0;
    HRESULT hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using skeleton
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR);
        if (SUCCEEDED(hr))
        {
			// Create an event that will be signaled when depth data is available
			m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Open a depth image stream to receive depth frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_DEPTH,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextDepthFrameEvent,
				&m_pDepthStreamHandle);
			
			///////////////////////////////////////////

            // Create an event that will be signaled when skeleton data is available
            m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

            // Open a skeleton stream to receive skeleton data
            hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0); 

			// Create an event that will be signaled when color data is available
			m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Open a color image stream to receive color frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_COLOR,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextColorFrameEvent,
				&m_pColorStreamHandle);
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!");
        return E_FAIL;
    }

    return hr;
}

//Implementação do botão de print
HRESULT GetScreenshotFileName(wchar_t *screenshotName, UINT screenshotNameSize)
{
	wchar_t *knownPath = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &knownPath);

	if (SUCCEEDED(hr))
	{
		// Get the time
		wchar_t timeString[MAX_PATH];
		GetTimeFormatEx(NULL, 0, NULL, L"hh'-'mm'-'ss", timeString, _countof(timeString));

		// File name will be KinectSnapshot-HH-MM-SS.wav
		StringCchPrintfW(screenshotName, screenshotNameSize, L"%s\\KinectSnapshot-%s.bmp", knownPath, timeString);
	}

	CoTaskMemFree(knownPath);
	return hr;
}


void writePPMFloat(float *image, std::string(ppmfile), int width, int height)

{
	FILE *arq = fopen(ppmfile.c_str(), "w");

	fprintf(arq, "P3\n%d %d\n255\n", width, height);
	
	for (int y = 0, k = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++, k = k + 4)
		{
			fprintf(arq, "%d %d %d ", (int)(((float)image[k] * 255)), (int)(((float)image[k + 1] * 255)), (int)(((float)image[k + 2] * 255)));
		}
		fprintf(arq, "\n");
	}

	fclose(arq);
	//printf("\nmin: %d max: %d",minv,maxv);
} 

/// <summary>
/// Handle new skeleton data
/// </summary>
void CSkeletonBasics::ProcessSkeleton()
{
	////////// ESQUELETO //////////////////////////////
    NUI_SKELETON_FRAME skeletonFrame = {0};

    HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
    if ( FAILED(hr) )
    {
        return;
    }


    // smooth out the skeleton data
    m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);
	/////////////////////////////////////////

	// DEPTH DATA //////////////////////////////////
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the depth frame
	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	BOOL nearMode;
	INuiFrameTexture* pTexture;

	// Get the depth image pixel texture
	hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(
		m_pDepthStreamHandle, &imageFrame, &nearMode, &pTexture);
	if (FAILED(hr))
	{
		goto ReleaseFrame;
	}

	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	// Make sure we've received valid data
	if (LockedRect.Pitch != 0)
	{
		// Get the min and max reliable depth for the current frame
		int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
		int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

		BYTE * rgbrun = m_depthRGBX;


		const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

		// end pixel is start + width*height - 1
		const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (cDepthWidth * cDepthHeight);
		int posimg = 0;
		while (pBufferRun < pBufferEnd)
		{
			// discard the portion of the depth that contains only the player index
			USHORT depth = pBufferRun->depth;

			// To convert to a byte, we're discarding the most-significant
			// rather than least-significant bits.
			// We're preserving detail, although the intensity will "wrap."
			// Values outside the reliable depth range are mapped to 0 (black).

			// Note: Using conditionals in this loop could degrade performance.
			// Consider using a lookup table instead when writing production code.
			BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth < 2000+minDepth ? ((depth-minDepth) % 2000)/2000.*255.0 : 0);

			//Alterei a profundidade de 2500 para 3000
			float intensityfloat = (depth >= minDepth && depth < 3000 + minDepth ? ((depth - minDepth) % 3000) / 3000.0 : 0);

			rgbrunfloat[posimg++] = intensityfloat;
			rgbrunfloat[posimg++] = intensityfloat;
			rgbrunfloat[posimg++] = intensityfloat;
			posimg++;
			
			// Write out blue byte
			*(rgbrun++) = intensity;
			
			// Write out green byte
			*(rgbrun++) = intensity;

			// Write out red byte
			*(rgbrun++) = intensity;

			// We're outputting BGR, the last byte in the 32 bits is unused so skip it
			// If we were outputting BGRA, we would write alpha here.
			++rgbrun;

			// Increment our index into the Kinect's depth buffer
			++pBufferRun;
		}

		
		//Implementação do botão de print
		if (m_bSaveScreenshot)
		{
			WCHAR statusMessage[cStatusMessageMaxLen];

			// Retrieve the path to My Photos
			WCHAR screenshotPath[MAX_PATH];
			GetScreenshotFileName(screenshotPath, _countof(screenshotPath));

			// Write out the bitmap to disk
			hr = SaveBitmapToFile(static_cast<BYTE *>(LockedRect.pBits), cColorWidth, cColorHeight, 32, screenshotPath);

			if (SUCCEEDED(hr))
			{
				// Set the status bar to show where the screenshot was saved
				StringCchPrintf(statusMessage, cStatusMessageMaxLen, L"Screenshot saved to %s", screenshotPath);
			}
			else
			{
				StringCchPrintf(statusMessage, cStatusMessageMaxLen, L"Failed to write screenshot to %s", screenshotPath);
			}

			SetStatusMessage(statusMessage);

			// toggle off so we don't save a screenshot again next frame
			m_bSaveScreenshot = false;
		}
	

		// Endure Direct2D is ready to draw
		hr = EnsureDirect2DResources();
		if (FAILED(hr))
		{
			return;
		}

		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->Clear();

		RECT rct;
		GetClientRect(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), &rct);
		int width = rct.right;
		int height = rct.bottom;

		D2D1_POINT_2F pos2dMaoDireita;
		

		/////////////////// PEGANDO O ESQUELETO /////////////////////////
		for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
		{
			int contador = 0;
			NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;

			if (NUI_SKELETON_TRACKED == trackingState)
			{
				if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HAND_RIGHT] != NUI_SKELETON_POSITION_TRACKED)
					continue;

				// We're tracking the skeleton, draw it
				DrawSkeleton(skeletonFrame.SkeletonData[i], width, height);

				pos2dMaoDireita = SkeletonToScreen(skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], cDepthWidth, cDepthHeight);
				
				// EXTRAIR SUBIMAGEM
				
				float profundidade = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z;
				
				float r = 62.0 / profundidade;
				int raio = ceil(r);
				int linha = pos2dMaoDireita.y;
				int coluna = pos2dMaoDireita.x; 

				float *img_segmentada;
				float *img_binarizada;

				img_segmentada = new float[4 * (2 * raio + 1)*(2 * raio + 1)];
				img_binarizada = new float[4 * (2 * raio + 1)*(2 * raio + 1)];

				int giva = 0;

				if (linha - raio < 0 || linha + raio >= 480)
					continue;
				if (coluna - raio < 0 || coluna + raio >= 640)
					continue;

				for (int j = linha - raio; j <= linha + raio; ++j)
				{
					for (int k = coluna - raio; k <= coluna + raio; ++k)
					{
						img_segmentada[giva] = rgbrunfloat[(j*cDepthWidth + k) * 4];
						img_segmentada[giva + 1] = rgbrunfloat[(j*cDepthWidth + k) * 4 + 1];
						img_segmentada[giva + 2] = rgbrunfloat[(j*cDepthWidth + k) * 4 + 2];
						//img_segmentada[giva + 3] = rgbrunfloat[(j*cDepthWidth + k) * 4 + 3];
						giva += 4;
					}
				}

				giva = 0;

				for (int j = linha - raio; j <= linha + raio; ++j)
				{
					for (int k = coluna - raio; k <= coluna + raio; ++k)
					{
						if (rgbrunfloat[(j*cDepthWidth + k) * 4] > 0 && rgbrunfloat[(j*cDepthWidth + k) * 4] < rgbrunfloat[(linha*cDepthWidth + coluna) * 4] + 0.05 )
						{
							img_binarizada[giva] = 0;
							img_binarizada[giva + 1] = 0;
							img_binarizada[giva + 2] = 0;
							
						}
						else
						{
							img_binarizada[giva] = 1.0;
							img_binarizada[giva + 1] = 1.0;
							img_binarizada[giva + 2] = 1.0;
						}
						giva += 4;
					}
				}

					
				rgbrunfloat[(linha*cDepthWidth + coluna) * 4] = 1.0;
				rgbrunfloat[(linha*cDepthWidth + coluna) * 4 + 1] = 0.0;
				rgbrunfloat[(linha*cDepthWidth + coluna) * 4 + 2] = 0.0;
				rgbrunfloat[(linha*cDepthWidth + coluna) * 4 + 3] = 0.0;

				cv::Mat src = cv::Mat{ raio * 2, raio * 2, CV_8UC1,  cv::Scalar(0)};
				for (int y = 0; y < raio*2; ++y)
				{
					for (int x = 0; x < raio*2; ++x)
					{
						uint8_t val = img_binarizada[x + y * (raio * 2)] * 255;
						src.at<uint8_t>(y, x) = val;
					}
				}

				cv::Mat dst = cv::Mat{50, 50, CV_8UC1,  cv::Scalar(0)};
				cv::resize(src, dst, {50, 50});
				
				for (int y = 0; y < 50; ++y)
				{
					for (int x = 0; x < 50; ++x)
					{
						uint8_t val = dst.at<uint8_t>(y, x);
						imToClassifyInt[x + y * 50] = val;
					}
				}

				count++;
				if (count>500)
				{
					exit(0);
				}

				vector_classifier = ClassifyImgNet(sock, imToClassifyInt, cv::Size{50, 50});
				std::ofstream file;
				file.open("C:/Users/Givanildo Lima/Documents/results.txt", std::ios::app | std::ios::out);
				file << "[" << vector_classifier[0] << ", " << vector_classifier[1] << "] ";
				file.close();

				//writePPMFloat(rgbrunfloat, std::string("imagens/profundidade/profundidade") + std::to_string(count+500) + std::string(".ppm"), cDepthWidth, cDepthHeight);
				//writePPMFloat(img_segmentada, std::string("imagens/segmentada/segmentada") + std::to_string(count+500) + std::string(".ppm"), 2 * raio + 1, 2 * raio + 1);
				//writePPMFloat(img_binarizada, std::string("imagens/binarizada/binarizada") + std::to_string(count+500) + std::string(".ppm"), 2 * raio + 1, 2 * raio + 1);
				
				//ProcessColor(count+500);

				//writePPMFloat(img_cortada, "img_cortada.ppm", 2 * raio + 1, 2 * raio + 1);
	     		//writePPMFloat(rgbrunfloat, "img_inteira.ppm", cDepthWidth, cDepthHeight);

				//delete[] img_cortada;
				//delete[] img_binarizada;
			}
		}
		
		// Draw the data with Direct2D
		m_pDrawDepth->Draw(m_depthRGBX, cDepthWidth * cDepthHeight * cBytesPerPixel);
	}


	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	pTexture->Release();

ReleaseFrame:
	// Release the frame
	m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);


	
	/*
    for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
    {
        NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;

        if (NUI_SKELETON_TRACKED == trackingState)
        {
            // We're tracking the skeleton, draw it
            DrawSkeleton(skeletonFrame.SkeletonData[i], width, height);
			
			pos2dMaoDireita = SkeletonToScreen(skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], width, height);


        }
        else if (NUI_SKELETON_POSITION_ONLY == trackingState)
        {
            // we've only received the center point of the skeleton, draw that
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                SkeletonToScreen(skeletonFrame.SkeletonData[i].Position, width, height),
                g_JointThickness,
                g_JointThickness
                );

            m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointTracked);
        }
    }
	*/
    hr = m_pRenderTarget->EndDraw();

    // Device lost, need to recreate the render target
    // We'll dispose it now and retry drawing
    if (D2DERR_RECREATE_TARGET == hr)
    {
        hr = S_OK;
        DiscardDirect2DResources();
    }
}

////////////////////////////////////// CLASSIFICADOR ////////////////////////////////////////////////

std::vector<float> CSkeletonBasics::ClassifyImgNet(sf::TcpSocket &sock, const std::vector<int>& im, cv::Size imSize)
{
	size_t reciLen;
	char predictBuff[50000];

	for (int y = 0; y < imSize.height; ++y)
	{
		auto val = sizeof(int) * imSize.width;
		if (sock.send((void *)(&im[y * imSize.width]),
			sizeof(int) * imSize.width, reciLen) != sf::Socket::Done)
		{
			std::cout << "error " << std::endl;
			exit(4);
		}
	}
	predictBuff[0] = '\0';

	sock.receive(predictBuff, sizeof(char) * 50000, reciLen);

	predictBuff[reciLen] = '\0';

	std::vector<float> resultVec;

	std::string str{ predictBuff };

	for (size_t i = 0; i < 2; ++i)
	{
		float a = std::stof(str.substr(i * 9, 7));
		resultVec.push_back(a);
	}
	
	return resultVec;

	if (resultVec[0] == 1)
	{
		m_pRenderTarget->DrawLine(m_Points[NUI_SKELETON_POSITION_HIP_LEFT], m_Points[NUI_SKELETON_POSITION_KNEE_LEFT], m_pBrushBoneInferred, g_InferredBoneThickness);
		m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
	}
	else
	{
		m_pRenderTarget->DrawLine(m_Points[NUI_SKELETON_POSITION_HIP_LEFT], m_Points[NUI_SKELETON_POSITION_KNEE_LEFT], m_pBrushBoneInferred, g_InferredBoneThickness);
		m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
	}
}

//////////////////////////////////////////////////////////////////////////////////////

void CSkeletonBasics::ProcessColor(int contador)
{
    HRESULT hr;
    NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the color frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pColorStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        // Draw the data with Direct2D
        //m_pDrawColor->Draw(static_cast<BYTE *>(LockedRect.pBits), LockedRect.size);

        // If the user pressed the screenshot button, save a screenshot
		 WCHAR statusMessage[cStatusMessageMaxLen];

		// Retrieve the path to My Photos
		 
		 WCHAR CAMINHO[MAX_PATH];
		 
	    //GetScreenshotFileName(screenshotPath, _countof(screenshotPath));
		
		swprintf_s(CAMINHO, L"imagens/rgb/rgb %d.bmp", contador);
		
		//float myFloat = System.BitConverter.ToSingle(static_cast<BYTE *>(LockedRect.pBits), 0);
		
        // Write out the bitmap to disk
		hr = SaveBitmapToFile(static_cast<BYTE *>(LockedRect.pBits), cColorWidth, cColorHeight, 32, CAMINHO);
		

		if (SUCCEEDED(hr))
		{
			// Set the status bar to show where the screenshot was saved
			StringCchPrintf(statusMessage, cStatusMessageMaxLen, L"Screenshot saved to %s", CAMINHO);
		}
		else
		{
			StringCchPrintf(statusMessage, cStatusMessageMaxLen, L"Failed to write screenshot to %s", CAMINHO);
		}

		SetStatusMessage(statusMessage);
		
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pColorStreamHandle, &imageFrame);
}

/// <summary>
/// Draws a skeleton
/// </summary>
/// <param name="skel">skeleton to draw</param>
/// <param name="windowWidth">width (in pixels) of output buffer</param>
/// <param name="windowHeight">height (in pixels) of output buffer</param>
void CSkeletonBasics::DrawSkeleton(const NUI_SKELETON_DATA & skel, int windowWidth, int windowHeight)
{      
    int i;

    for (i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i)
    {
        m_Points[i] = SkeletonToScreen(skel.SkeletonPositions[i], windowWidth, windowHeight);
    }


    // Render Torso
    DrawBone(skel, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
    DrawBone(skel, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

    // Left Arm
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

    // Right Arm
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

    // Left Leg
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

    // Right Leg
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);

    // Draw the joints in a different color
    for (i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i)
    {
        
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(m_Points[NUI_SKELETON_POSITION_HAND_RIGHT], g_JointThickness, g_JointThickness);
		m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointInferred);
	
/*

		D2D1_ELLIPSE ellipse = D2D1::Ellipse( m_Points[i], g_JointThickness, g_JointThickness );
        if ( skel.eSkeletonPositionTrackingState[i] == NUI_SKELETON_POSITION_INFERRED )
        {
            m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointInferred);
        }
        else if ( skel.eSkeletonPositionTrackingState[i] == NUI_SKELETON_POSITION_TRACKED )
        {
            m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointTracked);
        }
		*/
    }

	m_rightHandPosition = skel.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT];
	m_pDrawDepth->SetRightHandPosition(m_rightHandPosition);
}

/// <summary>
/// Draws a bone line between two joints
/// </summary>
/// <param name="skel">skeleton to draw bones from</param>
/// <param name="joint0">joint to start drawing from</param>
/// <param name="joint1">joint to end drawing at</param>
void CSkeletonBasics::DrawBone(const NUI_SKELETON_DATA & skel, NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
    NUI_SKELETON_POSITION_TRACKING_STATE joint0State = skel.eSkeletonPositionTrackingState[joint0];
    NUI_SKELETON_POSITION_TRACKING_STATE joint1State = skel.eSkeletonPositionTrackingState[joint1];

    // If we can't find either of these joints, exit
    if (joint0State == NUI_SKELETON_POSITION_NOT_TRACKED || joint1State == NUI_SKELETON_POSITION_NOT_TRACKED)
    {
        return;
    }

    // Don't draw if both points are inferred
    if (joint0State == NUI_SKELETON_POSITION_INFERRED && joint1State == NUI_SKELETON_POSITION_INFERRED)
    {
        return;
    }

    // We assume all drawn bones are inferred unless BOTH joints are tracked
    if (joint0State == NUI_SKELETON_POSITION_TRACKED && joint1State == NUI_SKELETON_POSITION_TRACKED)
    {
        m_pRenderTarget->DrawLine(m_Points[joint0], m_Points[joint1], m_pBrushBoneTracked, g_TrackedBoneThickness);
    }
    else
    {
        m_pRenderTarget->DrawLine(m_Points[joint0], m_Points[joint1], m_pBrushBoneInferred, g_InferredBoneThickness);
    }
}

/// <summary>
/// Converts a skeleton point to screen space
/// </summary>
/// <param name="skeletonPoint">skeleton point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
D2D1_POINT_2F CSkeletonBasics::SkeletonToScreen(Vector4 skeletonPoint, int width, int height)
{
    LONG x, y;
    USHORT depth;

    // Calculate the skeleton's position on the screen
    // NuiTransformSkeletonToDepthImage returns coordinates in NUI_IMAGE_RESOLUTION_320x240 space
    NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depth);

    float screenPointX = static_cast<float>(x * width) / cScreenWidth;
    float screenPointY = static_cast<float>(y * height) / cScreenHeight;

    return D2D1::Point2F(screenPointX, screenPointY);
}

/// <summary>
/// Ensure necessary Direct2d resources are created
/// </summary>
/// <returns>S_OK if successful, otherwise an error code</returns>
HRESULT CSkeletonBasics::EnsureDirect2DResources()
{
    HRESULT hr = S_OK;

    // If there isn't currently a render target, we need to create one
    if (NULL == m_pRenderTarget)
    {
        RECT rc;
        GetWindowRect( GetDlgItem( m_hWnd, IDC_VIDEOVIEW ), &rc );  

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        D2D1_SIZE_U size = D2D1::SizeU( width, height );
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.pixelFormat = D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        // Create a Hwnd render target, in order to render to the window set in initialize
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(GetDlgItem( m_hWnd, IDC_VIDEOVIEW), size),
            &m_pRenderTarget
            );
        if ( FAILED(hr) )
        {
            SetStatusMessage(L"Couldn't create Direct2D render target!");
            return hr;
        }

        //light green
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);

        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 1.0f), &m_pBrushBoneTracked);
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 1.0f), &m_pBrushBoneInferred);
    }

    return hr;
}

/// <summary>
/// Dispose Direct2d resources 
/// </summary>
void CSkeletonBasics::DiscardDirect2DResources( )
{
    SafeRelease(m_pRenderTarget);

    SafeRelease(m_pBrushJointTracked);
    SafeRelease(m_pBrushJointInferred);
    SafeRelease(m_pBrushBoneTracked);
    SafeRelease(m_pBrushBoneInferred);
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void CSkeletonBasics::SetStatusMessage(WCHAR * szMessage)
{
    SendDlgItemMessageW(m_hWnd, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szMessage);
}

HRESULT CSkeletonBasics::SaveBitmapToFile(BYTE* pBitmapBits, LONG lWidth, LONG lHeight, WORD wBitsPerPixel, LPCWSTR lpszFilePath)
{
	DWORD dwByteCount = lWidth * lHeight * (wBitsPerPixel / 8);

	BITMAPINFOHEADER bmpInfoHeader = { 0 };

	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);  // Size of the header
	bmpInfoHeader.biBitCount = wBitsPerPixel;             // Bit count
	bmpInfoHeader.biCompression = BI_RGB;                    // Standard RGB, no compression
	bmpInfoHeader.biWidth = lWidth;                    // Width in pixels
	bmpInfoHeader.biHeight = -lHeight;                  // Height in pixels, negative indicates it's stored right-side-up
	bmpInfoHeader.biPlanes = 1;                         // Default
	bmpInfoHeader.biSizeImage = dwByteCount;               // Image size in bytes

	BITMAPFILEHEADER bfh = { 0 };

	bfh.bfType = 0x4D42;                                           // 'M''B', indicates bitmap
	bfh.bfOffBits = bmpInfoHeader.biSize + sizeof(BITMAPFILEHEADER);  // Offset to the start of pixel data
	bfh.bfSize = bfh.bfOffBits + bmpInfoHeader.biSizeImage;        // Size of image + headers

																   // Create the file on disk to write to
	HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Return if error opening file
	if (NULL == hFile)
	{
		return E_ACCESSDENIED;
	}

	DWORD dwBytesWritten = 0;

	// Write the bitmap file header
	if (!WriteFile(hFile, &bfh, sizeof(bfh), &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Write the bitmap info header
	if (!WriteFile(hFile, &bmpInfoHeader, sizeof(bmpInfoHeader), &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Write the RGB Data
	if (!WriteFile(hFile, pBitmapBits, bmpInfoHeader.biSizeImage, &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Close the file
	CloseHandle(hFile);
	return S_OK;
}