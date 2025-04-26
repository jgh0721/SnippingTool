#ifndef DXGIMGR_HPP
#define DXGIMGR_HPP

#include <dxgi1_2.h>
#include <windef.h>
#include <sal.h>
#include <vector>

#include <atlbase.h>
#include <Shlwapi.h>

#include <dxgi1_2.h>
#include <d3d11.h>

#include <d2d1.h>
#include <wincodec.h>
#include <QtWidgets>

// macros
#define RESET_POINTER_EX(p, v)      if (nullptr != (p)) { *(p) = (v); }
#define RESET_POINTER(p)            RESET_POINTER_EX(p, nullptr)
#define CHECK_POINTER_EX(p, hr)     if (nullptr == (p)) { return (hr); }
#define CHECK_POINTER(p)            CHECK_POINTER_EX(p, E_POINTER)
#define CHECK_HR_BREAK(hr)          if (FAILED(hr)) { break; }
#define CHECK_HR_RETURN(hr)         { HRESULT hr_379f4648 = hr; if (FAILED(hr_379f4648)) { return hr_379f4648; } }

namespace nsDXGI
{
    // enum tagFrameSizeMode_e
    typedef enum tagFrameSizeMode_e : UINT
    {
        tagFrameSizeMode_Normal         = 0x0,
        tagFrameSizeMode_StretchImage   = 0x1,
        tagFrameSizeMode_AutoSize       = 0x2,
        tagFrameSizeMode_CenterImage    = 0x3,
        tagFrameSizeMode_Zoom           = 0x4,
    } tagFrameSizeMode;

    // enum tagFrameRotationMode_e
    typedef enum tagFrameRotationMode_e : UINT
    {
        tagFrameRotationMode_Auto       = 0x0,
        tagFrameRotationMode_Identity   = 0x1,
        tagFrameRotationMode_90         = 0x2,
        tagFrameRotationMode_180        = 0x3,
        tagFrameRotationMode_270        = 0x4,
    } tagFrameRotationMode;

    // Holds info about the pointer/cursor
    // struct tagMouseInfo_s
    typedef struct tagMouseInfo_s
    {
        UINT ShapeBufferSize;
        _Field_size_bytes_( ShapeBufferSize ) BYTE* PtrShapeBuffer;
        DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;
        POINT Position;
        bool Visible;
        UINT WhoUpdatedPositionLast;
        LARGE_INTEGER LastTimeStamp;
    } tagMouseInfo;

    // struct tagFrameSize_s
    typedef struct tagFrameSize_s
    {
        LONG Width;
        LONG Height;
    } tagFrameSize;

    // struct tagBounds_s
    typedef struct tagFrameBounds_s
    {
        LONG X;
        LONG Y;
        LONG Width;
        LONG Height;
    } tagFrameBounds;

    // struct tagFrameBufferInfo_s
    typedef struct tagFrameBufferInfo_s
    {
        UINT                                 BufferSize;
        _Field_size_bytes_( BufferSize ) BYTE* Buffer;
        INT                                  BytesPerPixel;
        tagFrameBounds                       Bounds;
        INT                                  Pitch;
    } tagFrameBufferInfo;

    // struct tagDublicatorMonitorInfo_s
    typedef struct tagDublicatorMonitorInfo_s
    {
        INT             Idx;
        WCHAR           DisplayName[ 64 ];
        INT             RotationDegrees;
        tagFrameBounds  Bounds;
        HMONITOR        Handle;
    } tagDublicatorMonitorInfo;

    typedef std::vector<tagDublicatorMonitorInfo*> DublicatorMonitorInfoVec;

    // struct tagScreenCaptureFilterConfig_s
    typedef struct tagScreenCaptureFilterConfig_s
    {
    public:
        INT                     MonitorIdx;
        INT                     ShowCursor;
        nsDXGI::tagFrameRotationMode    RotationMode;
        nsDXGI::tagFrameSizeMode        SizeMode;
        tagFrameSize            OutputSize; /* Discard for tagFrameSizeMode_AutoSize */
    } tagScreenCaptureFilterConfig;

    // struct tagRendererInfo_s
    typedef struct tagRendererInfo_s
    {
        INT                     MonitorIdx;
        INT                     ShowCursor;
        nsDXGI::tagFrameRotationMode    RotationMode;
        nsDXGI::tagFrameSizeMode        SizeMode;
        tagFrameSize            OutputSize;

        FLOAT                   RotationDegrees;
        FLOAT                   ScaleX;
        FLOAT                   ScaleY;
        DXGI_FORMAT             SrcFormat;
        tagFrameBounds          SrcBounds;
        tagFrameBounds          DstBounds;
    } tagRendererInfo;


// class DXGICaptureHelper
class DXGICaptureHelper
{
public:
    static COM_DECLSPEC_NOTHROW HRESULT ConvertDxgiOutputToMonitorInfo( _In_ const DXGI_OUTPUT_DESC* pDxgiOutput, _In_ int monitorIdx, _Out_ tagDublicatorMonitorInfo* pOutVal );
    static COM_DECLSPEC_NOTHROW BOOL    IsEqualMonitorInfo( _In_ const tagDublicatorMonitorInfo* p1, _In_ const tagDublicatorMonitorInfo* p2 );
    static COM_DECLSPEC_NOTHROW HRESULT IsRendererInfoValid( _In_ const tagRendererInfo* pRendererInfo );
    static COM_DECLSPEC_NOTHROW HRESULT CalculateRendererInfo( _In_ const DXGI_OUTDUPL_DESC* pDxgiOutputDuplDesc, _Inout_ tagRendererInfo* pRendererInfo );
    // ResizeFrameBuffer
    static COM_DECLSPEC_NOTHROW HRESULT ResizeFrameBuffer( _Inout_ tagFrameBufferInfo* pBufferInfo, _In_ UINT uiNewSize );
    // GetMouse
    static COM_DECLSPEC_NOTHROW HRESULT GetMouse( _In_ IDXGIOutputDuplication* pOutputDuplication, _Inout_ tagMouseInfo* PtrInfo, _In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo, UINT MonitorIdx, INT OffsetX, INT OffsetY );
    // ProcessMouseMask
    static COM_DECLSPEC_NOTHROW HRESULT ProcessMouseMask( _In_ const tagMouseInfo* PtrInfo, _In_ const DXGI_OUTPUT_DESC* DesktopDesc, _Inout_ tagFrameBufferInfo* pBufferInfo );

    // Draw mouse provided in buffer to backbuffer
    static COM_DECLSPEC_NOTHROW HRESULT DrawMouse( _In_ tagMouseInfo* PtrInfo, _In_ const DXGI_OUTPUT_DESC* DesktopDesc, _Inout_ tagFrameBufferInfo* pTempMouseBuffer, _Inout_ ID3D11Texture2D* pSharedSurf );
    static COM_DECLSPEC_NOTHROW HRESULT CreateBitmap( _In_ ID2D1RenderTarget* pRenderTarget, _In_ ID3D11Texture2D* pSourceTexture, _Outptr_ ID2D1Bitmap** ppOutBitmap );
    static COM_DECLSPEC_NOTHROW HRESULT GetContainerFormatByFileName( _In_ LPCWSTR lpcwFileName, _Out_opt_ GUID* pRetVal = NULL );
    static COM_DECLSPEC_NOTHROW HRESULT SaveImageToFile( _In_ IWICImagingFactory* pWICImagingFactory, _In_ IWICBitmapSource* pWICBitmapSource, _In_ LPCWSTR lpcwFileName );

}; // end class DXGICaptureHelper

class CDXGICapture
{
private:
    ATL::CComAutoCriticalSection    m_csLock;

    BOOL                            m_bInitialized;
    DublicatorMonitorInfoVec        m_monitorInfos;
    tagRendererInfo                 m_rendererInfo;

    tagMouseInfo                    m_mouseInfo;
    tagFrameBufferInfo              m_tempMouseBuffer;
    DXGI_OUTPUT_DESC                m_desktopOutputDesc;

    D3D_FEATURE_LEVEL               m_lD3DFeatureLevel;
    CComPtr<ID3D11Device>           m_ipD3D11Device;
    CComPtr<ID3D11DeviceContext>    m_ipD3D11DeviceContext;

    CComPtr<IDXGIOutputDuplication> m_ipDxgiOutputDuplication;
    CComPtr<ID3D11Texture2D>        m_ipCopyTexture2D;

    CComPtr<ID2D1Device>            m_ipD2D1Device;
    CComPtr<ID2D1Factory>           m_ipD2D1Factory;
    CComPtr<IWICImagingFactory>     m_ipWICImageFactory;
    CComPtr<IWICBitmap>             m_ipWICOutputBitmap;
    CComPtr<ID2D1RenderTarget>      m_ipD2D1RenderTarget;
    tagScreenCaptureFilterConfig    m_config;
public:
    CDXGICapture();
    ~CDXGICapture();

    HRESULT                         Initialize();
    HRESULT                         Terminate();
    HRESULT                         SetConfig( const tagScreenCaptureFilterConfig* pConfig );
    HRESULT                         SetConfig( const tagScreenCaptureFilterConfig& config );

    BOOL                            IsInitialized() const;
    D3D_FEATURE_LEVEL               GetD3DFeatureLevel() const;

    int                             GetDublicatorMonitorInfoCount() const;
    const tagDublicatorMonitorInfo* GetDublicatorMonitorInfo( int index ) const;
    const tagDublicatorMonitorInfo* FindDublicatorMonitorInfo( int monitorIdx ) const;


    HRESULT                         CaptureToFile( _In_ LPCWSTR lpcwOutputFileName, _Out_opt_ BOOL* pRetIsTimeout = NULL, _Out_opt_ UINT* pRetRenderDuration = NULL );
    QPixmap                         CaptureToPixmap( _In_ LPCWSTR lpcwOutputFileName, _Out_opt_ BOOL* pRetIsTimeout = NULL, _Out_opt_ UINT* pRetRenderDuration = NULL );

private:
    HRESULT                         loadMonitorInfos( ID3D11Device* pDevice );
    void                            freeMonitorInfos();

    HRESULT                         createDeviceResource( const tagScreenCaptureFilterConfig* pConfig, const tagDublicatorMonitorInfo* pSelectedMonitorInfo );
    void                            terminateDeviceResource();

    HRESULT                         captureFrame( _Out_opt_ BOOL* pRetIsTimeout = NULL, _Out_opt_ UINT* pRetRenderDuration = NULL );
    QPixmap                         convertWICBitmapToQPixmap( IWICImagingFactory* pWICImagingFactory, IWICBitmapSource* pWICBitmapSource );
};

} // nsDXGI

#endif //DXGIMGR_HPP
