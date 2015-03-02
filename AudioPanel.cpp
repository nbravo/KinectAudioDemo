#include "stdafx.h"
#include "AudioPanel.h"

/// Constructor
AudioPanel::AudioPanel() : 
    m_hWnd(0),
    m_pD2DFactory(NULL), 
    m_pRenderTarget(NULL),
    m_RenderTargetTransform(D2D1::Matrix3x2F::Identity()),
    m_pBeamGauge(NULL),
    m_pBeamGaugeFill(NULL),
    m_pBeamNeedle(NULL),
    m_pBeamNeedleFill(NULL),
    m_BeamNeedleTransform(D2D1::Matrix3x2F::Identity()),
    m_pPanelOutline(NULL),
    m_pPanelOutlineStroke(NULL) {
}

/// Destructor
AudioPanel::~AudioPanel() {
    DiscardResources();
    SafeRelease(m_pD2DFactory);
}

/// Set the window to draw to as well as the video format
/// Implied bits per pixel is 32
/// <param name="hWnd">window to draw to</param>
/// <param name="pD2DFactory">already created D2D factory object</param>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::Initialize(const HWND hWnd, ID2D1Factory* pD2DFactory) {
    if (NULL == pD2DFactory) {
        return E_INVALIDARG;
    }

    m_hWnd = hWnd;

    // One factory for the entire application so save a pointer here
    m_pD2DFactory = pD2DFactory;

    m_pD2DFactory->AddRef();

    return S_OK;
}

/// Draws audio panel.
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::Draw() {
    // create the resources for this draw device. They will be recreated if previously lost.
    HRESULT hr = EnsureResources();

    if (FAILED(hr)) {
        return hr;
    }
    
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->SetTransform(m_RenderTargetTransform);

    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Draw audio beam gauge
    m_pRenderTarget->FillGeometry(m_pBeamGauge, m_pBeamGaugeFill, NULL);
    m_pRenderTarget->SetTransform(m_BeamNeedleTransform * m_RenderTargetTransform);
    m_pRenderTarget->FillGeometry(m_pBeamNeedle, m_pBeamNeedleFill, NULL);
    m_pRenderTarget->SetTransform(m_RenderTargetTransform);

    // Draw panel outline
    m_pRenderTarget->DrawGeometry(m_pPanelOutline, m_pPanelOutlineStroke, 0.001f);
            
    hr = m_pRenderTarget->EndDraw();

    // Device lost, need to recreate the render target. We'll dispose it now and retry drawing.
    if (hr == D2DERR_RECREATE_TARGET) {
        hr = S_OK;
        DiscardResources();
    }

    return hr;
}

void AudioPanel::SetBeam(const float & beamAngle) {
    if (m_pRenderTarget == NULL) {
        return;
    }

    m_BeamNeedleTransform = D2D1::Matrix3x2F::Rotation(-beamAngle, D2D1::Point2F(0.5f,0.0f));
}

/// Dispose of Direct2d resources.
void AudioPanel::DiscardResources() {
    SafeRelease(m_pRenderTarget);
    SafeRelease(m_pBeamGauge);
    SafeRelease(m_pBeamGaugeFill);
    SafeRelease(m_pBeamNeedle);
    SafeRelease(m_pBeamNeedleFill);
    SafeRelease(m_pPanelOutline);
    SafeRelease(m_pPanelOutlineStroke);
}

/// Ensure necessary Direct2d resources are created
/// <returns>indicates success or failure</returns>
HRESULT AudioPanel::EnsureResources() {
    HRESULT hr = S_OK;

    if (NULL == m_pRenderTarget) {
        // Get the panel size
        RECT panelRect = {0};
        GetWindowRect(m_hWnd, &panelRect);
        UINT panelWidth = panelRect.right - panelRect.left;
        UINT panelHeight = panelRect.bottom - panelRect.top;
        D2D1_SIZE_U size = D2D1::SizeU(panelWidth, panelHeight);

        m_RenderTargetTransform = D2D1::Matrix3x2F::Scale(D2D1::SizeF((FLOAT)panelWidth, (FLOAT)panelWidth));

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        // Create a hWnd render target, in order to render to the window set in initialize
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(m_hWnd, size),
            &m_pRenderTarget
           );

        if (SUCCEEDED(hr)) {
            hr = CreateBeamGauge();

            if (SUCCEEDED(hr)) {
                hr = CreatePanelOutline();
            }
        }
    }

    if (FAILED(hr)) {
        DiscardResources();
    }

    return hr;
}

/// Create gauge (with needle) used to display beam angle.
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateBeamGauge() {
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pBeamGauge);

    // Create gauge background shape
    if (SUCCEEDED(hr)) {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pBeamGauge->Open(&pGeometrySink);

        if (SUCCEEDED(hr)) {
            pGeometrySink->BeginFigure(D2D1::Point2F(0.1503f,0.2832f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.228f,0.2203f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.772f,0.2203f), D2D1::SizeF(0.35f,0.35f), 102, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->AddLine(D2D1::Point2F(0.8497f,0.2832f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.1503f,0.2832f), D2D1::SizeF(0.45f,0.45f), 102, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
            hr = pGeometrySink->Close();

            // Create gauge background brush
            if (SUCCEEDED(hr)) {
                ID2D1GradientStopCollection *pGradientStops = NULL;
                D2D1_GRADIENT_STOP gradientStops[4];
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[1].position = 0.34f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::WhiteSmoke, 1);
                gradientStops[2].position = 0.37f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF::WhiteSmoke, 1);
                gradientStops[3].position = 1.0f;
                hr = m_pRenderTarget->CreateGradientStopCollection(
                    gradientStops,
                    4,
                    &pGradientStops
                   );

                if (SUCCEEDED(hr)) {
                    hr = m_pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(0.5f,0.0f), D2D1::Point2F(0.0f,0.0f), 1.0f, 1.0f), pGradientStops, &m_pBeamGaugeFill);

                    if (SUCCEEDED(hr)) {
                        // Create gauge needle shape and fill brush
                        hr = CreateBeamGaugeNeedle();
                    }
                }

                SafeRelease(pGradientStops);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}

/// Create gauge needle used to display beam angle.
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateBeamGaugeNeedle() {
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pBeamNeedle);

    // Create gauge needle shape
    if (SUCCEEDED(hr)) {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pBeamNeedle->Open(&pGeometrySink);

        if (SUCCEEDED(hr)) { 
            pGeometrySink->BeginFigure(D2D1::Point2F(0.495f,0.35f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.505f,0.35f));
            pGeometrySink->AddLine(D2D1::Point2F(0.5f,0.44f));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
            hr = pGeometrySink->Close();

            // Create gauge needle brush
            if (SUCCEEDED(hr)) {
                ID2D1GradientStopCollection *pGradientStops = NULL;
                D2D1_GRADIENT_STOP gradientStops[4];
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[1].position = 0.35f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::BlueViolet, 1);
                gradientStops[2].position = 0.395f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF::BlueViolet, 1);
                gradientStops[3].position = 1.0f;
                hr = m_pRenderTarget->CreateGradientStopCollection(
                    gradientStops,
                    4,
                    &pGradientStops
                   );

                if (SUCCEEDED(hr)) {
                    hr = m_pRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.5f,0.0f), D2D1::Point2F(0.5f,1.0f)), pGradientStops, &m_pBeamNeedleFill);
                }

                SafeRelease(pGradientStops);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}

/// Create outline that frames both gauges and energy display into a cohesive panel.
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreatePanelOutline() {
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pPanelOutline);

    // Create panel outline path
    if (SUCCEEDED(hr)) {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pPanelOutline->Open(&pGeometrySink);

        if (SUCCEEDED(hr)) {
            /// Draw left wave display frame
            pGeometrySink->BeginFigure(D2D1::Point2F(0.15f,0.0353f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.13f,0.0353f));
            pGeometrySink->AddLine(D2D1::Point2F(0.13f,0.2203f));
            pGeometrySink->AddLine(D2D1::Point2F(0.2280f,0.2203f));

            // Draw gauge outline
            pGeometrySink->AddLine(D2D1::Point2F(0.1270f,0.3021f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.8730f,0.3021f), D2D1::SizeF(0.48f,0.48f), 102, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->AddLine(D2D1::Point2F(0.7720f,0.2203f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.2280f,0.2203f), D2D1::SizeF(0.35f,0.35f), 102, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

            // Reposition geometry without drawing
            pGeometrySink->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_UNSTROKED);
            pGeometrySink->AddLine(D2D1::Point2F(0.7720f,0.2203f));
            pGeometrySink->SetSegmentFlags(D2D1_PATH_SEGMENT_NONE);

            // Draw right wave display frame
            pGeometrySink->AddLine(D2D1::Point2F(0.87f,0.2203f));
            pGeometrySink->AddLine(D2D1::Point2F(0.87f,0.0353f));
            pGeometrySink->AddLine(D2D1::Point2F(0.85f,0.0353f));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_OPEN);
            hr = pGeometrySink->Close();

            // Create panel outline brush
            if (SUCCEEDED(hr)) {
                hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &m_pPanelOutlineStroke);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}