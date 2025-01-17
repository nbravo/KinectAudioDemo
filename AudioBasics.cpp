﻿#include "stdafx.h"
#include "AudioBasics.h"
#include "resource.h"

// For StringCch* and such
#include <strsafe.h>

// For INT_MAX
#include <limits.h>

// For M_PI and log definitions
#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>

#include <Windows.h>
#include <iostream>
#include <sstream>
#define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugString( os_.str().c_str() );  \
}

/// Entry point for the application
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
        {
            CAudioBasics application;
            application.Run(hInstance, nCmdShow);
        }

        CoUninitialize();
    }

    return EXIT_SUCCESS;
}

/// Constructor
CAudioBasics::CAudioBasics() :
    m_pD2DFactory(NULL),
    m_pAudioPanel(NULL),
    m_pNuiSensor(NULL),
    m_pNuiAudioSource(NULL),
    m_pDMO(NULL),
    m_pPropertyStore(NULL) {
}

 /// Destructor
 CAudioBasics::~CAudioBasics() {
    if (m_pNuiSensor) {
        m_pNuiSensor->NuiShutdown();
    }

    // clean up Direct2D renderer
    delete m_pAudioPanel;
    m_pAudioPanel = NULL;

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    SafeRelease(m_pNuiSensor);
    SafeRelease(m_pNuiAudioSource);
    SafeRelease(m_pDMO);
    SafeRelease(m_pPropertyStore);
}

/// Creates the main window and begins processing
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CAudioBasics::Run(HINSTANCE hInstance, int nCmdShow) {
    MSG       msg = {0};
    WNDCLASS  wc;

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"AudioBasicsAppDlgWndClass";

    if (!RegisterClassW(&wc)) {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        hInstance,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CAudioBasics::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    // Main message loop
    while (WM_QUIT != msg.message) {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            // If a dialog message will be taken care of by the dialog proc
            if ((hWndApp != NULL) && IsDialogMessageW(hWndApp, &msg)) {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// Handles window messages, passes most to the class instance to handle
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CAudioBasics::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CAudioBasics* pThis = NULL;

    if (WM_INITDIALOG == uMsg) {
        pThis = reinterpret_cast<CAudioBasics*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else {
        pThis = reinterpret_cast<CAudioBasics*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (NULL != pThis) {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/// Handle windows messages for the class instance
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CAudioBasics::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pAudioPanel = new AudioPanel();
            HRESULT hr = m_pAudioPanel->Initialize(GetDlgItem(m_hWnd, IDC_AUDIOVIEW), m_pD2DFactory);
            if (FAILED(hr)) {
                SetStatusMessage(L"Failed to initialize the Direct2D draw device.");
                break;
            }

            // Look for a connected Kinect, and create it if found
            hr = CreateFirstConnected();
            if (FAILED(hr)) {
                break;
            }

            SetTimer(m_hWnd, iAudioReadTimerId, iAudioReadTimerInterval, NULL);
            SetTimer(m_hWnd, iEnergyRefreshTimerId, iEnergyRefreshTimerInterval, NULL);
        }
        break;

        // Capture all available audio or update audio panel each time timer fires
        case WM_TIMER:
          if (wParam == iAudioReadTimerId) {
              ProcessAudio();
          }
          else if(wParam == iEnergyRefreshTimerId) {
              Update();
          }
          break;

          // If the titlebar X is clicked, destroy app
          case WM_CLOSE:
              KillTimer(m_hWnd, iAudioReadTimerId);
              KillTimer(m_hWnd, iEnergyRefreshTimerId);
              DestroyWindow(hWnd);
              break;

          case WM_DESTROY:
              // Quit the main message pump
              PostQuitMessage(0);
              break;
    }

    return FALSE;
}

/// Create the first connected Kinect found.
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT CAudioBasics::CreateFirstConnected() {
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr)) {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i) {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr)) {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr) {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor) {
        // Initialize the Kinect and specify that we'll be using audio signal
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_AUDIO); 
        if (FAILED(hr)) {
            // Some other application is streaming from the same Kinect sensor
            SafeRelease(m_pNuiSensor);
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr)) {
        SetStatusMessage(L"No ready Kinect found!");
        return E_FAIL;
    }

    return  InitializeAudioSource();
}

/// Initialize Kinect audio capture/control objects.
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CAudioBasics::InitializeAudioSource() {
    // Get the audio source
    HRESULT hr = m_pNuiSensor->NuiGetAudioSource(&m_pNuiAudioSource);
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&m_pDMO);
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&m_pPropertyStore);
    if (FAILED(hr)) {
        return hr;
    }

    // Set AEC-MicArray DMO system mode. This must be set for the DMO to work properly.
    // Possible values are:
    //   SINGLE_CHANNEL_AEC = 0
    //   OPTIBEAM_ARRAY_ONLY = 2
    //   OPTIBEAM_ARRAY_AND_AEC = 4
    //   SINGLE_CHANNEL_NSAGC = 5
    PROPVARIANT pvSysMode;
    PropVariantInit(&pvSysMode);
    pvSysMode.vt = VT_I4;
    pvSysMode.lVal = (LONG)(2); // Use OPTIBEAM_ARRAY_ONLY setting. Set OPTIBEAM_ARRAY_AND_AEC instead if you expect to have sound playing from speakers.
    m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
    PropVariantClear(&pvSysMode);

    // Set DMO output format
    WAVEFORMATEX wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};
    DMO_MEDIA_TYPE mt = {0};
    hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
    if (FAILED(hr)) {
        return hr;
    }

    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.lSampleSize = 0;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.formattype = FORMAT_WaveFormatEx;	
    memcpy_s(mt.pbFormat, sizeof(WAVEFORMATEX), &wfxOut, sizeof(WAVEFORMATEX));

    hr = m_pDMO->SetOutputType(0, &mt, 0); 
    MoFreeMediaType(&mt);

    return hr;
}

/// Capture new audio data.
void CAudioBasics::ProcessAudio() {
    // Bottom portion of computed energy signal that will be discarded as noise.
    // Only portion of signal above noise floor will be displayed.
    const float cEnergyNoiseFloor = 0.2f;

    ULONG cbProduced = 0;
    BYTE *pProduced = NULL;
    DWORD dwStatus = 0;
    DMO_OUTPUT_DATA_BUFFER outputBuffer = {0};
    outputBuffer.pBuffer = &m_csmCaptureBuffer;

    HRESULT hr = S_OK;

    do {
        m_csmCaptureBuffer.Init(0);
        outputBuffer.dwStatus = 0;
        hr = m_pDMO->ProcessOutput(0, 1, &outputBuffer, &dwStatus);
        if (FAILED(hr)) {
            SetStatusMessage(L"Failed to process audio output.");
            break;
        }

        if (hr == S_FALSE) {
            cbProduced = 0;
        }
        else {
            m_csmCaptureBuffer.GetBufferAndLength(&pProduced, &cbProduced);
        }

        if (cbProduced > 0) {
            double beamAngle, sourceAngle, sourceConfidence;

            // Obtain beam angle from INuiAudioBeam afforded by microphone array
            m_pNuiAudioSource->GetBeam(&beamAngle);
            m_pNuiAudioSource->GetPosition(&sourceAngle, &sourceConfidence);

            // Convert angles to degrees and set values in audio panel
            float beamAngleDegrees = static_cast<float>((180.0 * beamAngle) / M_PI);
            float sourceAngleDegrees = static_cast<float>((180.0 * sourceAngle) / M_PI);
            m_pAudioPanel->SetBeam(beamAngleDegrees);

            DBOUT("Beam Angle: " << beamAngleDegrees << "\n");
            DBOUT("Source Angle: " << sourceAngleDegrees << "\n");
        }

    } while (outputBuffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE);
}

/// Display latest audio data.
void CAudioBasics::Update() {
    m_pAudioPanel->Draw();
}

/// Set the status bar message
/// <param name="szMessage">message to display</param>
void CAudioBasics::SetStatusMessage(WCHAR * szMessage) {
    SendDlgItemMessageW(m_hWnd, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szMessage);
}