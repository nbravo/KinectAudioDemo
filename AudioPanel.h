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
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT Initialize(const HWND hwnd, ID2D1Factory* pD2DFactory);
     
    /// Draws audio panel.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT Draw();
     
    /// Update the beam angle being displayed in panel.
    /// <param name="beamAngle">new beam angle to display.</param>
    void SetBeam(const float & beamAngle);

private:
    // Main application window
    HWND                        m_hWnd;

    // Direct2D objects
    ID2D1Factory*               m_pD2DFactory;
    ID2D1HwndRenderTarget*      m_pRenderTarget;
    D2D_MATRIX_3X2_F            m_RenderTargetTransform;
    ID2D1PathGeometry*          m_pBeamGauge;
    ID2D1RadialGradientBrush*   m_pBeamGaugeFill;
    ID2D1PathGeometry*          m_pBeamNeedle;
    ID2D1LinearGradientBrush*   m_pBeamNeedleFill;
    D2D_MATRIX_3X2_F            m_BeamNeedleTransform;
    ID2D1PathGeometry*          m_pPanelOutline;
    ID2D1SolidColorBrush*       m_pPanelOutlineStroke;

    /// Dispose of Direct2d resources.
    void DiscardResources( );

    /// Ensure necessary Direct2d resources are created.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT EnsureResources();

    /// Create gauge used to display beam angle.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateBeamGauge();

    /// Create gauge needle used to display beam angle.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreateBeamGaugeNeedle();
     
    /// Create outline that frames both gauges and energy display into a cohesive panel.
    /// <returns>S_OK on success, otherwise failure code.</returns>
    HRESULT CreatePanelOutline();
};