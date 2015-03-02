#pragma once

#include "AudioPanel.h"
#include "resource.h"

// For IMediaObject and related interfaces
#include <dmo.h>

// For configuring DMO properties
#include <wmcodecdsp.h>

// For WAVEFORMATEX
#include <mmreg.h>

// For FORMAT_WaveFormatEx and such
#include <uuids.h>

// For Kinect SDK APIs
#include <NuiApi.h>

// Format of Kinect audio stream
static const WORD       AudioFormat = WAVE_FORMAT_PCM;

// Number of channels in Kinect audio stream
static const WORD       AudioChannels = 1;

// Samples per second in Kinect audio stream
static const DWORD      AudioSamplesPerSecond = 16000;

// Average bytes per second in Kinect audio stream
static const DWORD      AudioAverageBytesPerSecond = 32000;

// Block alignment in Kinect audio stream
static const WORD       AudioBlockAlign = 2;

// Bits per audio sample in Kinect audio stream
static const WORD       AudioBitsPerSample = 16;

/// IMediaBuffer implementation for a statically allocated buffer.
class CStaticMediaBuffer : public IMediaBuffer {
public:
    // Constructor
    CStaticMediaBuffer() : m_dataLength(0) {}

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IUnknown) {
            AddRef();
            *ppv = (IUnknown*)this;
            return NOERROR;
        }
        else if (riid == IID_IMediaBuffer) {
            AddRef();
            *ppv = (IMediaBuffer*)this;
            return NOERROR;
        }
        else {
            return E_NOINTERFACE;
        }
    }

    // IMediaBuffer methods
    STDMETHODIMP SetLength(DWORD length) {m_dataLength = length; return NOERROR;}
    STDMETHODIMP GetMaxLength(DWORD *pMaxLength) {*pMaxLength = sizeof(m_pData); return NOERROR;}
    STDMETHODIMP GetBufferAndLength(BYTE **ppBuffer, DWORD *pLength) {
        if (ppBuffer) {
            *ppBuffer = m_pData;
        }
        if (pLength) {
            *pLength = m_dataLength;
        }
        return NOERROR;
    }
    void Init(ULONG ulData) {
        m_dataLength = ulData;
    }

protected:
    // Statically allocated buffer used to hold audio data returned by IMediaObject
    BYTE m_pData[AudioSamplesPerSecond * AudioBlockAlign];

    // Amount of data currently being held in m_pData
    ULONG m_dataLength;
};

/// Main application class for AudioBasics sample.
class CAudioBasics {
public:
    
    /// Constructor
    CAudioBasics();

    /// Destructor
    ~CAudioBasics();

    /// Handles window messages, passes most to the class instance to handle
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    static LRESULT CALLBACK MessageRouter(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    /// Handle windows messages for a class instance
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    LRESULT CALLBACK        DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Creates the main window and begins processing
    /// <param name="hInstance">handle to the application instance</param>
    /// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
    int                     Run(HINSTANCE hInstance, int nCmdShow);

private:
    // ID of timer that drives audio capture.
    static const int        iAudioReadTimerId = 1;

    // Time interval, in milliseconds, for timer that drives audio capture.
    static const int        iAudioReadTimerInterval = 50;

    // ID of timer that drives energy stream display.
    static const int        iEnergyRefreshTimerId = 2;

    // Time interval, in milliseconds, for timer that drives energy stream display.
    static const int        iEnergyRefreshTimerInterval = 10;

    // Main application dialog window.
    HWND                    m_hWnd;

    // Factory used to create Direct2D objects.
    ID2D1Factory*           m_pD2DFactory;

    // Object that controls displaying Kinect audio data.
    AudioPanel*             m_pAudioPanel;    

    // Current Kinect sensor.
    INuiSensor*             m_pNuiSensor;

    // Audio source used to query Kinect audio beam and sound source angles.
    INuiAudioBeam*          m_pNuiAudioSource;

    // Media object from which Kinect audio stream is captured.
    IMediaObject*           m_pDMO;

    // Property store used to configure Kinect audio properties.
    IPropertyStore*         m_pPropertyStore;

    // Buffer to hold captured audio data.
    CStaticMediaBuffer      m_csmCaptureBuffer;

    /// Create the first connected Kinect found.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT                 CreateFirstConnected();
    
    /// Initialize Kinect audio capture/control objects.
    /// <returns> S_OK on success, otherwise failure code.</returns>
    HRESULT                 InitializeAudioSource();

    /// Capture new audio data.
    void                    ProcessAudio();

    /// Display latest audio data.
    void                    Update();

    /// Set the status bar message.
    /// <param name="szMessage">message to display.</param>
    void                    SetStatusMessage(WCHAR* szMessage);
};
