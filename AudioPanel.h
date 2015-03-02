#pragma once

// Direct2D Header Files
#include <d2d1.h>

 
/// Manages the drawing of audio data in audio panel that includes beam angle and
/// sound source angle gauges, and an oscilloscope visualization of audio data.
 
/// Note that all panel elements are laid out directly in an {X,Y} coordinate space
/// where X and Y are both in [0.0,1.0] interval, and whole panel is later re-scaled
/// to fit available area via a scaling transform.
class AudioPanel {
public:
    AudioPanel();

    virtual ~AudioPanel();

     
    /// Set the window to draw to as well as the amount of energy to expect to have to display.
    /// <param name="hWnd">window to draw to.</param>
    /// <param name="pD2DFactory">already created D2D factory object.</param>
    /// <param name="energyToDisplay">Number of energy samples to display at any given time.</param>
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT Initialize(const HWND hwnd, ID2D1Factory* pD2DFactory, UINT energyToDisplay);
     
    /// Draws audio panel.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT Draw();
     
    /// Update the beam angle being displayed in panel.
    /// <param name="beamAngle">new beam angle to display.</param>
    void SetBeam(const float & beamAngle);

     
    /// Update the sound source angle and confidence being displayed in panel.
    /// <param name="soundSourceAngle">new sound source angle to display.</param>
    /// <param name="soundSourceConfidence">new sound source confidence to display.</param>
    void SetSoundSource(const float & soundSourceAngle, const float & soundSourceConfidence);

    /// Update the audio energy being displayed in panel.
    /// <param name="pEnergy">new energy buffer to render.</param>
    /// <param name="energyLength">length of energy buffer to render.</param>
    void UpdateEnergy(const float *pEnergy, const UINT energyLength);

private:
    // Main application window
    HWND                        m_hWnd;

    // Direct2D objects
    ID2D1Factory*               m_pD2DFactory;
    ID2D1HwndRenderTarget*      m_pRenderTarget;
    D2D_MATRIX_3X2_F            m_RenderTargetTransform;
    UINT                        m_uiEnergyDisplayWidth;
    UINT                        m_uiEnergyDisplayHeight;
    BYTE*                       m_pEnergyBackground;
    UINT                        m_uiEnergyBackgroundStride;
    BYTE*                       m_pEnergyForeground;
    UINT                        m_uiEnergyForegroundStride;
    ID2D1Bitmap*                m_pEnergyDisplay;
    D2D1_RECT_F                 m_EnergyDisplayPosition;
    ID2D1PathGeometry*          m_pBeamGauge;
    ID2D1RadialGradientBrush*   m_pBeamGaugeFill;
    ID2D1PathGeometry*          m_pBeamNeedle;
    ID2D1LinearGradientBrush*   m_pBeamNeedleFill;
    D2D_MATRIX_3X2_F            m_BeamNeedleTransform;
    ID2D1PathGeometry*          m_pSourceGauge;
    ID2D1LinearGradientBrush*   m_pSourceGaugeFill;
    D2D_MATRIX_3X2_F            m_SourceGaugeTransform;
    ID2D1PathGeometry*          m_pPanelOutline;
    ID2D1SolidColorBrush*       m_pPanelOutlineStroke;

    /// Dispose of Direct2d resources.
    void DiscardResources( );

     
    /// Ensure necessary Direct2d resources are created.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT EnsureResources();
     
    /// Create oscilloscope display for audio energy data.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateEnergyDisplay();

    /// Create gauge used to display beam angle.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateBeamGauge();

    /// Create gauge needle used to display beam angle.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateBeamGaugeNeedle();

    /// Create gauge (with position cloud) used to display sound source angle.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateSourceGauge();

    /// Create gradient used to represent sound source confidence, with the
    /// specified width.
    /// <param name="width">Width of gradient, specified in [0.0,1.0] interval.</param>
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateSourceGaugeFill(const float & width);
     
    /// Create outline that frames both gauges and energy display into a cohesive panel.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreatePanelOutline();
};