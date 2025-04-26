#include "dxgiMgr.hpp"

#include <d2d1_1.h>
#include <ShellScalingAPI.h>
// #include <qpa/qplatformscreen.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d2d1.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "windowscodecs.lib" )

// Driver types supported
const D3D_DRIVER_TYPE g_DriverTypes[] =
{
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_REFERENCE,
};
const UINT g_NumDriverTypes = ARRAYSIZE( g_DriverTypes );

// Feature levels supported
const D3D_FEATURE_LEVEL g_FeatureLevels[] =
{
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_1
};

const UINT g_NumFeatureLevels = ARRAYSIZE( g_FeatureLevels );

#define AUTOLOCK()                  ATL::CComCritSecLock<ATL::CComAutoCriticalSection> auto_lock((ATL::CComAutoCriticalSection&)(m_csLock))
#define D3D_FEATURE_LEVEL_INVALID  ((D3D_FEATURE_LEVEL)0x0)

///////////////////////////////////////////////////////////////////////////////
/// class DXGICaptureHelper
//
COM_DECLSPEC_NOTHROW HRESULT nsDXGI::DXGICaptureHelper::ConvertDxgiOutputToMonitorInfo( _In_ const DXGI_OUTPUT_DESC* pDxgiOutput, _In_ int monitorIdx, _Out_ tagDublicatorMonitorInfo* pOutVal )
{
    CHECK_POINTER( pOutVal );
    // reset output parameter
    RtlZeroMemory( pOutVal, sizeof( tagDublicatorMonitorInfo ) );
    CHECK_POINTER_EX( pDxgiOutput, E_INVALIDARG );

    switch( pDxgiOutput->Rotation )
    {
        case DXGI_MODE_ROTATION_UNSPECIFIED:
        case DXGI_MODE_ROTATION_IDENTITY:
            pOutVal->RotationDegrees = 0;
            break;

        case DXGI_MODE_ROTATION_ROTATE90:
            pOutVal->RotationDegrees = 90;
            break;

        case DXGI_MODE_ROTATION_ROTATE180:
            pOutVal->RotationDegrees = 180;
            break;

        case DXGI_MODE_ROTATION_ROTATE270:
            pOutVal->RotationDegrees = 270;
            break;
    }

    pOutVal->Idx            = monitorIdx;
    pOutVal->Bounds.X       = pDxgiOutput->DesktopCoordinates.left;
    pOutVal->Bounds.Y       = pDxgiOutput->DesktopCoordinates.top;
    pOutVal->Bounds.Width   = pDxgiOutput->DesktopCoordinates.right - pDxgiOutput->DesktopCoordinates.left;
    pOutVal->Bounds.Height  = pDxgiOutput->DesktopCoordinates.bottom - pDxgiOutput->DesktopCoordinates.top;
    pOutVal->Handle         = pDxgiOutput->Monitor;
    wsprintfW( pOutVal->DisplayName, L"Display %d: %ldx%ld @ %ld,%ld"
        , monitorIdx + 1
        , pOutVal->Bounds.Width, pOutVal->Bounds.Height
        , pOutVal->Bounds.X, pOutVal->Bounds.Y );

    return S_OK;
}
COM_DECLSPEC_NOTHROW BOOL nsDXGI::DXGICaptureHelper::IsEqualMonitorInfo( _In_ const tagDublicatorMonitorInfo* p1, _In_ const tagDublicatorMonitorInfo* p2 )
{
    if( nullptr == p1 )
    {
        return ( nullptr == p2 );
    }
    if( nullptr == p2 )
    {
        return FALSE;
    }

    return memcmp( ( const void* )p1, ( const void* )p2, sizeof( tagDublicatorMonitorInfo ) ) == 0;
} // IsEqualMonitorInfo

COM_DECLSPEC_NOTHROW HRESULT nsDXGI::DXGICaptureHelper::IsRendererInfoValid( const tagRendererInfo* pRendererInfo )
{
    CHECK_POINTER_EX( pRendererInfo, E_INVALIDARG );

    if( pRendererInfo->SrcFormat != DXGI_FORMAT_B8G8R8A8_UNORM )
    {
        return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    if( pRendererInfo->SizeMode != nsDXGI::tagFrameSizeMode_Normal )
    {
        if( ( pRendererInfo->OutputSize.Width <= 0 ) || ( pRendererInfo->OutputSize.Height <= 0 ) )
        {
            return D2DERR_BITMAP_BOUND_AS_TARGET;
        }
    }

    if( ( pRendererInfo->DstBounds.Width <= 0 ) || ( pRendererInfo->DstBounds.Height <= 0 ) ||
        ( pRendererInfo->SrcBounds.Width <= 0 ) || ( pRendererInfo->SrcBounds.Height <= 0 ) )
    {
        return D2DERR_ORIGINAL_TARGET_NOT_BOUND;
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::CalculateRendererInfo( const DXGI_OUTDUPL_DESC* pDxgiOutputDuplDesc, tagRendererInfo* pRendererInfo )
{
    CHECK_POINTER_EX( pDxgiOutputDuplDesc, E_INVALIDARG );
    CHECK_POINTER_EX( pRendererInfo, E_INVALIDARG );

    pRendererInfo->SrcFormat = pDxgiOutputDuplDesc->ModeDesc.Format;
    // get rotate state
    switch( pDxgiOutputDuplDesc->Rotation )
    {
        case DXGI_MODE_ROTATION_ROTATE90:
            pRendererInfo->RotationDegrees = 90.0f;
            pRendererInfo->SrcBounds.X      = 0;
            pRendererInfo->SrcBounds.Y      = 0;
            pRendererInfo->SrcBounds.Width  = pDxgiOutputDuplDesc->ModeDesc.Height;
            pRendererInfo->SrcBounds.Height = pDxgiOutputDuplDesc->ModeDesc.Width;
            break;
        case DXGI_MODE_ROTATION_ROTATE180:
            pRendererInfo->RotationDegrees = 180.0;
            pRendererInfo->SrcBounds.X      = 0;
            pRendererInfo->SrcBounds.Y      = 0;
            pRendererInfo->SrcBounds.Width  = pDxgiOutputDuplDesc->ModeDesc.Width;
            pRendererInfo->SrcBounds.Height = pDxgiOutputDuplDesc->ModeDesc.Height;
            break;
        case DXGI_MODE_ROTATION_ROTATE270:
            pRendererInfo->RotationDegrees = 270.0f;
            pRendererInfo->SrcBounds.X      = 0;
            pRendererInfo->SrcBounds.Y      = 0;
            pRendererInfo->SrcBounds.Width  = pDxgiOutputDuplDesc->ModeDesc.Height;
            pRendererInfo->SrcBounds.Height = pDxgiOutputDuplDesc->ModeDesc.Width;
            break;
        default: // OR DXGI_MODE_ROTATION_IDENTITY:
            pRendererInfo->RotationDegrees = 0.0f;
            pRendererInfo->SrcBounds.X      = 0;
            pRendererInfo->SrcBounds.Y      = 0;
            pRendererInfo->SrcBounds.Width  = pDxgiOutputDuplDesc->ModeDesc.Width;
            pRendererInfo->SrcBounds.Height = pDxgiOutputDuplDesc->ModeDesc.Height;
            break;
    }

    // force rotate
    switch( pRendererInfo->RotationMode )
    {
        case nsDXGI::tagFrameRotationMode::tagFrameRotationMode_Identity:
            pRendererInfo->RotationDegrees = 0.0f;
            break;
        case nsDXGI::tagFrameRotationMode::tagFrameRotationMode_90:
            pRendererInfo->RotationDegrees = 90.0f;
            break;
        case nsDXGI::tagFrameRotationMode::tagFrameRotationMode_180:
            pRendererInfo->RotationDegrees = 180.0f;
            break;
        case nsDXGI::tagFrameRotationMode::tagFrameRotationMode_270:
            pRendererInfo->RotationDegrees = 270.0f;
            break;
        default: // tagFrameRotationMode::tagFrameRotationMode_Auto
            break;
    }

    if( pRendererInfo->SizeMode == nsDXGI::tagFrameSizeMode_Zoom )
    {
        FLOAT fSrcAspect, fOutAspect, fScaleFactor;

        // center for output
        pRendererInfo->DstBounds.Width  = pRendererInfo->SrcBounds.Width;
        pRendererInfo->DstBounds.Height = pRendererInfo->SrcBounds.Height;
        pRendererInfo->DstBounds.X      = ( pRendererInfo->OutputSize.Width - pRendererInfo->SrcBounds.Width ) >> 1;
        pRendererInfo->DstBounds.Y      = ( pRendererInfo->OutputSize.Height - pRendererInfo->SrcBounds.Height ) >> 1;

        fOutAspect = ( FLOAT )pRendererInfo->OutputSize.Width / pRendererInfo->OutputSize.Height;

        if( ( pRendererInfo->RotationDegrees == 0.0f ) || ( pRendererInfo->RotationDegrees == 180.0f ) )
        {
            fSrcAspect = ( FLOAT )pRendererInfo->SrcBounds.Width / pRendererInfo->SrcBounds.Height;

            if( fSrcAspect > fOutAspect )
            {
                fScaleFactor = ( FLOAT )pRendererInfo->OutputSize.Width / pRendererInfo->SrcBounds.Width;
            }
            else
            {
                fScaleFactor = ( FLOAT )pRendererInfo->OutputSize.Height / pRendererInfo->SrcBounds.Height;
            }
        }
        else // 90 or 270 degree
        {
            fSrcAspect = ( FLOAT )pRendererInfo->SrcBounds.Height / pRendererInfo->SrcBounds.Width;

            if( fSrcAspect > fOutAspect )
            {
                fScaleFactor = ( FLOAT )pRendererInfo->OutputSize.Width / pRendererInfo->SrcBounds.Height;
            }
            else
            {
                fScaleFactor = ( FLOAT )pRendererInfo->OutputSize.Height / pRendererInfo->SrcBounds.Width;
            }
        }

        pRendererInfo->ScaleX = fScaleFactor;
        pRendererInfo->ScaleY = fScaleFactor;
    }
    else if( pRendererInfo->SizeMode == nsDXGI::tagFrameSizeMode_CenterImage )
    {
        // center for output
        pRendererInfo->DstBounds.Width  = pRendererInfo->SrcBounds.Width;
        pRendererInfo->DstBounds.Height = pRendererInfo->SrcBounds.Height;
        pRendererInfo->DstBounds.X      = ( pRendererInfo->OutputSize.Width - pRendererInfo->SrcBounds.Width ) >> 1;
        pRendererInfo->DstBounds.Y      = ( pRendererInfo->OutputSize.Height - pRendererInfo->SrcBounds.Height ) >> 1;
    }
    else if( pRendererInfo->SizeMode == nsDXGI::tagFrameSizeMode_AutoSize )
    {
        // set the destination bounds
        pRendererInfo->DstBounds.Width  = pRendererInfo->SrcBounds.Width;
        pRendererInfo->DstBounds.Height = pRendererInfo->SrcBounds.Height;

        if( ( pRendererInfo->RotationDegrees == 0.0f ) || ( pRendererInfo->RotationDegrees == 180.0f ) )
        {
            // same as the source size
            pRendererInfo->OutputSize.Width  = pRendererInfo->SrcBounds.Width;
            pRendererInfo->OutputSize.Height = pRendererInfo->SrcBounds.Height;
        }
        else // 90 or 270 degree
        {
            // same as the source size
            pRendererInfo->OutputSize.Width  = pRendererInfo->SrcBounds.Height;
            pRendererInfo->OutputSize.Height = pRendererInfo->SrcBounds.Width;

            // center for output
            pRendererInfo->DstBounds.X = ( pRendererInfo->OutputSize.Width - pRendererInfo->SrcBounds.Width ) >> 1;
            pRendererInfo->DstBounds.Y = ( pRendererInfo->OutputSize.Height - pRendererInfo->SrcBounds.Height ) >> 1;
        }
    }
    else if( pRendererInfo->SizeMode == nsDXGI::tagFrameSizeMode_StretchImage )
    {
        // center for output
        pRendererInfo->DstBounds.Width  = pRendererInfo->SrcBounds.Width;
        pRendererInfo->DstBounds.Height = pRendererInfo->SrcBounds.Height;
        pRendererInfo->DstBounds.X      = ( pRendererInfo->OutputSize.Width - pRendererInfo->SrcBounds.Width ) >> 1;
        pRendererInfo->DstBounds.Y      = ( pRendererInfo->OutputSize.Height - pRendererInfo->SrcBounds.Height ) >> 1;

        if( ( pRendererInfo->RotationDegrees == 0.0f ) || ( pRendererInfo->RotationDegrees == 180.0f ) )
        {
            pRendererInfo->ScaleX = ( FLOAT )pRendererInfo->OutputSize.Width / pRendererInfo->DstBounds.Width;
            pRendererInfo->ScaleY = ( FLOAT )pRendererInfo->OutputSize.Height / pRendererInfo->DstBounds.Height;
        }
        else // 90 or 270 degree
        {
            pRendererInfo->ScaleX = ( FLOAT )pRendererInfo->OutputSize.Width / pRendererInfo->DstBounds.Height;
            pRendererInfo->ScaleY = ( FLOAT )pRendererInfo->OutputSize.Height / pRendererInfo->DstBounds.Width;
        }
    }
    else // tagFrameSizeMode_Normal
    {
        pRendererInfo->DstBounds.Width  = pRendererInfo->SrcBounds.Width;
        pRendererInfo->DstBounds.Height = pRendererInfo->SrcBounds.Height;

        if( pRendererInfo->RotationDegrees == 90 )
        {
            // set destination origin (bottom-left)
            pRendererInfo->DstBounds.X = ( pRendererInfo->OutputSize.Width - pRendererInfo->OutputSize.Height ) >> 1;
            pRendererInfo->DstBounds.Y = ( ( pRendererInfo->OutputSize.Width + pRendererInfo->OutputSize.Height ) >> 1 ) - pRendererInfo->DstBounds.Height;
        }
        else if( pRendererInfo->RotationDegrees == 180.0f )
        {
            // set destination origin (bottom-right)
            pRendererInfo->DstBounds.X = pRendererInfo->OutputSize.Width - pRendererInfo->DstBounds.Width;
            pRendererInfo->DstBounds.Y = pRendererInfo->OutputSize.Height - pRendererInfo->DstBounds.Height;
        }
        else if( pRendererInfo->RotationDegrees == 270 )
        {
            // set destination origin (top-right)
            pRendererInfo->DstBounds.Y = ( pRendererInfo->OutputSize.Height - pRendererInfo->OutputSize.Width ) >> 1;
            pRendererInfo->DstBounds.X = pRendererInfo->OutputSize.Width - pRendererInfo->DstBounds.Width - ( ( pRendererInfo->OutputSize.Width - pRendererInfo->OutputSize.Height ) >> 1 );
        }
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::ResizeFrameBuffer( tagFrameBufferInfo* pBufferInfo, UINT uiNewSize )
{
    CHECK_POINTER( pBufferInfo );

    if( uiNewSize <= pBufferInfo->BufferSize )
    {
        return S_FALSE; // no change
    }

    if( nullptr != pBufferInfo->Buffer )
    {
        delete[] pBufferInfo->Buffer;
        pBufferInfo->Buffer = nullptr;
    }

    pBufferInfo->Buffer = new ( std::nothrow ) BYTE[ uiNewSize ];
    if( !( pBufferInfo->Buffer ) )
    {
        pBufferInfo->BufferSize = 0;
        return E_OUTOFMEMORY;
    }
    pBufferInfo->BufferSize = uiNewSize;

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::GetMouse( IDXGIOutputDuplication* pOutputDuplication, tagMouseInfo* PtrInfo, DXGI_OUTDUPL_FRAME_INFO* FrameInfo, UINT MonitorIdx, INT OffsetX, INT OffsetY )
{
    CHECK_POINTER_EX( pOutputDuplication, E_INVALIDARG );
    CHECK_POINTER_EX( PtrInfo, E_INVALIDARG );
    CHECK_POINTER_EX( FrameInfo, E_INVALIDARG );

    // A non-zero mouse update timestamp indicates that there is a mouse position update and optionally a shape change
    if( FrameInfo->LastMouseUpdateTime.QuadPart == 0 )
    {
        return S_OK;
    }

    bool UpdatePosition = true;

    // Make sure we don't update pointer position wrongly
    // If pointer is invisible, make sure we did not get an update from another output that the last time that said pointer
    // was visible, if so, don't set it to invisible or update.
    if( !FrameInfo->PointerPosition.Visible && ( PtrInfo->WhoUpdatedPositionLast != MonitorIdx ) )
    {
        UpdatePosition = false;
    }

    // If two outputs both say they have a visible, only update if new update has newer timestamp
    if( FrameInfo->PointerPosition.Visible && PtrInfo->Visible && ( PtrInfo->WhoUpdatedPositionLast != MonitorIdx ) && ( PtrInfo->LastTimeStamp.QuadPart > FrameInfo->LastMouseUpdateTime.QuadPart ) )
    {
        UpdatePosition = false;
    }

    // Update position
    if( UpdatePosition )
    {
        PtrInfo->Position.x             = FrameInfo->PointerPosition.Position.x - OffsetX;
        PtrInfo->Position.y             = FrameInfo->PointerPosition.Position.y - OffsetY;
        PtrInfo->WhoUpdatedPositionLast = MonitorIdx;
        PtrInfo->LastTimeStamp          = FrameInfo->LastMouseUpdateTime;
        PtrInfo->Visible                = FrameInfo->PointerPosition.Visible != 0;
    }

    // No new shape
    if( FrameInfo->PointerShapeBufferSize == 0 )
    {
        return S_OK;
    }

    // Old buffer too small
    if( FrameInfo->PointerShapeBufferSize > PtrInfo->ShapeBufferSize )
    {
        if( PtrInfo->PtrShapeBuffer != nullptr )
        {
            delete[] PtrInfo->PtrShapeBuffer;
            PtrInfo->PtrShapeBuffer = nullptr;
        }
        PtrInfo->PtrShapeBuffer = new ( std::nothrow ) BYTE[ FrameInfo->PointerShapeBufferSize ];
        if( PtrInfo->PtrShapeBuffer == nullptr )
        {
            PtrInfo->ShapeBufferSize = 0;
            return E_OUTOFMEMORY;
        }

        // Update buffer size
        PtrInfo->ShapeBufferSize = FrameInfo->PointerShapeBufferSize;
    }

    // Get shape
    UINT    BufferSizeRequired;
    HRESULT hr = pOutputDuplication->GetFramePointerShape(
                                                          FrameInfo->PointerShapeBufferSize,
                                                          reinterpret_cast< VOID* >( PtrInfo->PtrShapeBuffer ),
                                                          &BufferSizeRequired,
                                                          &( PtrInfo->ShapeInfo )
                                                         );
    if( FAILED( hr ) )
    {
        delete[] PtrInfo->PtrShapeBuffer;
        PtrInfo->PtrShapeBuffer  = nullptr;
        PtrInfo->ShapeBufferSize = 0;
        return hr;
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::ProcessMouseMask( const tagMouseInfo* PtrInfo, const DXGI_OUTPUT_DESC* DesktopDesc, tagFrameBufferInfo* pBufferInfo )
{
    CHECK_POINTER_EX( PtrInfo, E_INVALIDARG );
    CHECK_POINTER_EX( DesktopDesc, E_INVALIDARG );
    CHECK_POINTER_EX( pBufferInfo, E_INVALIDARG );

    if( !PtrInfo->Visible )
    {
        return S_FALSE;
    }

    HRESULT hr            = S_OK;
    INT     DesktopWidth  = ( INT )( DesktopDesc->DesktopCoordinates.right - DesktopDesc->DesktopCoordinates.left );
    INT     DesktopHeight = ( INT )( DesktopDesc->DesktopCoordinates.bottom - DesktopDesc->DesktopCoordinates.top );

    pBufferInfo->Bounds.X      = PtrInfo->Position.x;
    pBufferInfo->Bounds.Y      = PtrInfo->Position.y;
    pBufferInfo->Bounds.Width  = PtrInfo->ShapeInfo.Width;
    pBufferInfo->Bounds.Height = ( PtrInfo->ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME )
                                     ? ( INT )( PtrInfo->ShapeInfo.Height / 2 )
                                     : ( INT )PtrInfo->ShapeInfo.Height;
    pBufferInfo->Pitch = pBufferInfo->Bounds.Width * 4;

    switch( PtrInfo->ShapeInfo.Type )
    {
        case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
        {
            // Resize mouseshape buffer (if necessary)
            hr = DXGICaptureHelper::ResizeFrameBuffer( pBufferInfo, PtrInfo->ShapeBufferSize );
            if( FAILED( hr ) )
            {
                return hr;
            }

            // use current mouseshape buffer
            // Copy mouseshape buffer
            memcpy_s( ( void* )pBufferInfo->Buffer, pBufferInfo->BufferSize, ( const void* )PtrInfo->PtrShapeBuffer, PtrInfo->ShapeBufferSize );
            break;
        }

        case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
        {
            // Resize mouseshape buffer (if necessary)
            hr = DXGICaptureHelper::ResizeFrameBuffer( pBufferInfo, pBufferInfo->Bounds.Height * pBufferInfo->Pitch );
            if( FAILED( hr ) )
            {
                return hr;
            }

            UINT* InitBuffer32 = reinterpret_cast< UINT* >( pBufferInfo->Buffer );

            for( INT Row = 0; Row < pBufferInfo->Bounds.Height; ++Row )
            {
                // Set mask
                BYTE Mask = 0x80;
                for( INT Col = 0; Col < pBufferInfo->Bounds.Width; ++Col )
                {
                    BYTE XorMask = PtrInfo->PtrShapeBuffer[ ( Col / 8 ) + ( ( Row + ( PtrInfo->ShapeInfo.Height / 2 ) ) * ( PtrInfo->ShapeInfo.Pitch ) ) ] & Mask;

                    // Set new pixel
                    InitBuffer32[ ( Row * pBufferInfo->Bounds.Width ) + Col ] = ( XorMask ) ? 0xFFFFFFFF : 0x00000000;

                    // Adjust mask
                    if( Mask == 0x01 )
                    {
                        Mask = 0x80;
                    }
                    else
                    {
                        Mask = Mask >> 1;
                    }
                }
            }

            break;
        }

        case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
        {
            // Resize mouseshape buffer (if necessary)
            hr = DXGICaptureHelper::ResizeFrameBuffer( pBufferInfo, pBufferInfo->Bounds.Height * pBufferInfo->Pitch );
            if( FAILED( hr ) )
            {
                return hr;
            }

            UINT* InitBuffer32  = reinterpret_cast< UINT* >( pBufferInfo->Buffer );
            UINT* ShapeBuffer32 = reinterpret_cast< UINT* >( PtrInfo->PtrShapeBuffer );

            for( INT Row = 0; Row < pBufferInfo->Bounds.Height; ++Row )
            {
                for( INT Col = 0; Col < pBufferInfo->Bounds.Width; ++Col )
                {
                    InitBuffer32[ ( Row * pBufferInfo->Bounds.Width ) + Col ] = ShapeBuffer32[ Col + ( Row * ( PtrInfo->ShapeInfo.Pitch / sizeof( UINT ) ) ) ] | 0xFF000000;
                }
            }

            break;
        }

        default:
            return E_INVALIDARG;

    }

    UINT* InitBuffer32 = reinterpret_cast< UINT* >( pBufferInfo->Buffer );
    UINT  width        = ( UINT )pBufferInfo->Bounds.Width;
    UINT  height       = ( UINT )pBufferInfo->Bounds.Height;

    switch( DesktopDesc->Rotation )
    {
        case DXGI_MODE_ROTATION_ROTATE90:
        {
            // Rotate -90 or +270
            for( UINT i = 0; i < width; i++ )
            {
                for( UINT j = 0; j < height; j++ )
                {
                    UINT I = j;
                    UINT J = width - 1 - i;
                    while( ( i * height + j ) > ( I * width + J ) )
                    {
                        UINT p     = I * width + J;
                        UINT tmp_i = p / height;
                        UINT tmp_j = p % height;
                        I          = tmp_j;
                        J          = width - 1 - tmp_i;
                    }
                    std::swap( *( InitBuffer32 + ( i * height + j ) ), *( InitBuffer32 + ( I * width + J ) ) );
                }
            }

            // translate bounds
            std::swap( pBufferInfo->Bounds.Width, pBufferInfo->Bounds.Height );
            INT nX                = pBufferInfo->Bounds.Y;
            INT nY                = DesktopWidth - ( INT )( pBufferInfo->Bounds.X + pBufferInfo->Bounds.Height );
            pBufferInfo->Bounds.X = nX;
            pBufferInfo->Bounds.Y = nY;
            pBufferInfo->Pitch    = pBufferInfo->Bounds.Width * 4;
        } break;
        case DXGI_MODE_ROTATION_ROTATE180:
        {
            // Rotate -180 or +180
            if( height % 2 != 0 )
            {
                //If N is odd reverse the middle row in the matrix
                UINT j = height >> 1;
                for( UINT i = 0; i < ( width >> 1 ); i++ )
                {
                    std::swap( InitBuffer32[ j * width + i ], InitBuffer32[ j * width + width - i - 1 ] );
                }
            }

            for( UINT j = 0; j < ( height >> 1 ); j++ )
            {
                for( UINT i = 0; i < width; i++ )
                {
                    std::swap( InitBuffer32[ j * width + i ], InitBuffer32[ ( height - j - 1 ) * width + width - i - 1 ] );
                }
            }

            // translate position
            INT nX                = DesktopWidth - ( INT )( pBufferInfo->Bounds.X + pBufferInfo->Bounds.Width );
            INT nY                = DesktopHeight - ( INT )( pBufferInfo->Bounds.Y + pBufferInfo->Bounds.Height );
            pBufferInfo->Bounds.X = nX;
            pBufferInfo->Bounds.Y = nY;
        } break;
        case DXGI_MODE_ROTATION_ROTATE270:
        {
            // Rotate -270 or +90
            for( UINT i = 0; i < width; i++ )
            {
                for( UINT j = 0; j < height; j++ )
                {
                    UINT I = height - 1 - j;
                    UINT J = i;
                    while( ( i * height + j ) > ( I * width + J ) )
                    {
                        int p     = I * width + J;
                        int tmp_i = p / height;
                        int tmp_j = p % height;
                        I         = height - 1 - tmp_j;
                        J         = tmp_i;
                    }
                    std::swap( *( InitBuffer32 + ( i * height + j ) ), *( InitBuffer32 + ( I * width + J ) ) );
                }
            }

            // translate bounds
            std::swap( pBufferInfo->Bounds.Width, pBufferInfo->Bounds.Height );
            INT nX                = DesktopHeight - ( pBufferInfo->Bounds.Y + pBufferInfo->Bounds.Width );
            INT nY                = pBufferInfo->Bounds.X;
            pBufferInfo->Bounds.X = nX;
            pBufferInfo->Bounds.Y = nY;
            pBufferInfo->Pitch    = pBufferInfo->Bounds.Width * 4;
        } break;
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::DrawMouse( tagMouseInfo* PtrInfo, const DXGI_OUTPUT_DESC* DesktopDesc, tagFrameBufferInfo* pTempMouseBuffer, ID3D11Texture2D* pSharedSurf )
{
    CHECK_POINTER_EX( PtrInfo, E_INVALIDARG );
    CHECK_POINTER_EX( DesktopDesc, E_INVALIDARG );
    CHECK_POINTER_EX( pTempMouseBuffer, E_INVALIDARG );
    CHECK_POINTER_EX( pSharedSurf, E_INVALIDARG );

    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC FullDesc;
    pSharedSurf->GetDesc( &FullDesc );

    INT SurfWidth  = FullDesc.Width;
    INT SurfHeight = FullDesc.Height;
    INT SurfPitch  = FullDesc.Width * 4;

    hr = DXGICaptureHelper::ProcessMouseMask( PtrInfo, DesktopDesc, pTempMouseBuffer );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // Buffer used if necessary (in case of monochrome or masked pointer)
    BYTE* InitBuffer = pTempMouseBuffer->Buffer;

    // Clipping adjusted coordinates / dimensions
    INT PtrWidth  = ( INT )pTempMouseBuffer->Bounds.Width;
    INT PtrHeight = ( INT )pTempMouseBuffer->Bounds.Height;

    INT PtrLeft  = ( INT )pTempMouseBuffer->Bounds.X;
    INT PtrTop   = ( INT )pTempMouseBuffer->Bounds.Y;
    INT PtrPitch = ( INT )pTempMouseBuffer->Pitch;

    INT SrcLeft   = 0;
    INT SrcTop    = 0;
    INT SrcWidth  = PtrWidth;
    INT SrcHeight = PtrHeight;

    if( PtrLeft < 0 )
    {
        // crop mouseshape left
        SrcLeft = -PtrLeft;
        // new mouse x position for drawing
        PtrLeft = 0;
    }
    else if( PtrLeft + PtrWidth > SurfWidth )
    {
        // crop mouseshape width
        SrcWidth = SurfWidth - PtrLeft;
    }

    if( PtrTop < 0 )
    {
        // crop mouseshape top
        SrcTop = -PtrTop;
        // new mouse y position for drawing
        PtrTop = 0;
    }
    else if( PtrTop + PtrHeight > SurfHeight )
    {
        // crop mouseshape height
        SrcHeight = SurfHeight - PtrTop;
    }

    // QI for IDXGISurface
    CComPtr<IDXGISurface> ipCopySurface;
    hr = pSharedSurf->QueryInterface( __uuidof( IDXGISurface ), ( void** )&ipCopySurface );
    if( SUCCEEDED( hr ) )
    {
        // Map pixels
        DXGI_MAPPED_RECT MappedSurface;
        hr = ipCopySurface->Map( &MappedSurface, DXGI_MAP_READ | DXGI_MAP_WRITE );
        if( SUCCEEDED( hr ) )
        {
            // 0xAARRGGBB
            UINT* SrcBuffer32 = reinterpret_cast< UINT* >( InitBuffer );
            UINT* DstBuffer32 = reinterpret_cast< UINT* >( MappedSurface.pBits ) + PtrTop * SurfWidth + PtrLeft;

            // Alpha blending masks
            const UINT AMask    = 0xFF000000;
            const UINT RBMask   = 0x00FF00FF;
            const UINT GMask    = 0x0000FF00;
            const UINT AGMask   = AMask | GMask;
            const UINT OneAlpha = 0x01000000;
            UINT       uiPixel1;
            UINT       uiPixel2;
            UINT       uiAlpha;
            UINT       uiNAlpha;
            UINT       uiRedBlue;
            UINT       uiAlphaGreen;

            for( INT Row = SrcTop; Row < SrcHeight; ++Row )
            {
                for( INT Col = SrcLeft; Col < SrcWidth; ++Col )
                {
                    // Alpha blending
                    uiPixel1     = DstBuffer32[ ( ( Row - SrcTop ) * SurfWidth ) + ( Col - SrcLeft ) ];
                    uiPixel2     = SrcBuffer32[ ( Row * PtrWidth ) + Col ];
                    uiAlpha      = ( uiPixel2 & AMask ) >> 24;
                    uiNAlpha     = 255 - uiAlpha;
                    uiRedBlue    = ( ( uiNAlpha * ( uiPixel1 & RBMask ) ) + ( uiAlpha * ( uiPixel2 & RBMask ) ) ) >> 8;
                    uiAlphaGreen = ( uiNAlpha * ( ( uiPixel1 & AGMask ) >> 8 ) ) + ( uiAlpha * ( OneAlpha | ( ( uiPixel2 & GMask ) >> 8 ) ) );

                    DstBuffer32[ ( ( Row - SrcTop ) * SurfWidth ) + ( Col - SrcLeft ) ] = ( ( uiRedBlue & RBMask ) | ( uiAlphaGreen & AGMask ) );
                }
            }
        }

        // Done with resource
        hr = ipCopySurface->Unmap();
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::CreateBitmap( ID2D1RenderTarget* pRenderTarget, ID3D11Texture2D* pSourceTexture, ID2D1Bitmap** ppOutBitmap )
{
    CHECK_POINTER( ppOutBitmap );
    *ppOutBitmap = nullptr;
    CHECK_POINTER_EX( pRenderTarget, E_INVALIDARG );
    CHECK_POINTER_EX( pSourceTexture, E_INVALIDARG );

    HRESULT                  hr = S_OK;
    CComPtr<ID3D11Texture2D> ipSourceTexture( pSourceTexture );
    CComPtr<IDXGISurface>    ipCopySurface;
    CComPtr<ID2D1Bitmap>     ipD2D1SourceBitmap;

    // QI for IDXGISurface
    hr = ipSourceTexture->QueryInterface( __uuidof( IDXGISurface ), ( void** )&ipCopySurface );
    CHECK_HR_RETURN( hr );

    // Map pixels
    DXGI_MAPPED_RECT MappedSurface;
    hr = ipCopySurface->Map( &MappedSurface, DXGI_MAP_READ );
    CHECK_HR_RETURN( hr );

    D3D11_TEXTURE2D_DESC destImageDesc;
    ipSourceTexture->GetDesc( &destImageDesc );

    hr = pRenderTarget->CreateBitmap(
                                     D2D1::SizeU( destImageDesc.Width, destImageDesc.Height ),
                                     ( const void* )MappedSurface.pBits,
                                     MappedSurface.Pitch,
                                     D2D1::BitmapProperties( D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED ) ),
                                     &ipD2D1SourceBitmap );
    if( FAILED( hr ) )
    {
        // Done with resource
        hr = ipCopySurface->Unmap();
        return hr;
    }

    // Done with resource
    hr = ipCopySurface->Unmap();
    CHECK_HR_RETURN( hr );

    // set return value
    *ppOutBitmap = ipD2D1SourceBitmap.Detach();

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::GetContainerFormatByFileName( LPCWSTR lpcwFileName, GUID* pRetVal )
{
    RESET_POINTER_EX( pRetVal, GUID_NULL );
    CHECK_POINTER_EX( lpcwFileName, E_INVALIDARG );

    if( lstrlenW( lpcwFileName ) == 0 )
    {
        return E_INVALIDARG;
    }

    LPCWSTR lpcwExtension = ::PathFindExtensionW( lpcwFileName );
    if( lstrlenW( lpcwExtension ) == 0 )
    {
        return MK_E_INVALIDEXTENSION; // ERROR_MRM_INVALID_FILE_TYPE
    }

    if( lstrcmpiW( lpcwExtension, L".bmp" ) == 0 )
    {
        RESET_POINTER_EX( pRetVal, GUID_ContainerFormatBmp );
    }
    else if( ( lstrcmpiW( lpcwExtension, L".tif" ) == 0 ) ||
             ( lstrcmpiW( lpcwExtension, L".tiff" ) == 0 ) )
    {
        RESET_POINTER_EX( pRetVal, GUID_ContainerFormatTiff );
    }
    else if( lstrcmpiW( lpcwExtension, L".png" ) == 0 )
    {
        RESET_POINTER_EX( pRetVal, GUID_ContainerFormatPng );
    }
    else if( ( lstrcmpiW( lpcwExtension, L".jpg" ) == 0 ) ||
             ( lstrcmpiW( lpcwExtension, L".jpeg" ) == 0 ) )
    {
        RESET_POINTER_EX( pRetVal, GUID_ContainerFormatJpeg );
    }
    else
    {
        return ERROR_MRM_INVALID_FILE_TYPE;
    }

    return S_OK;
}

HRESULT nsDXGI::DXGICaptureHelper::SaveImageToFile( IWICImagingFactory* pWICImagingFactory, IWICBitmapSource* pWICBitmapSource, LPCWSTR lpcwFileName )
{
    CHECK_POINTER_EX( pWICImagingFactory, E_INVALIDARG );
    CHECK_POINTER_EX( pWICBitmapSource, E_INVALIDARG );

    HRESULT hr = S_OK;
    GUID    guidContainerFormat;

    hr = GetContainerFormatByFileName( lpcwFileName, &guidContainerFormat );
    if( FAILED( hr ) )
    {
        return hr;
    }

    WICPixelFormatGUID             format = GUID_WICPixelFormatDontCare;
    CComPtr<IWICImagingFactory>    ipWICImagingFactory( pWICImagingFactory );
    CComPtr<IWICBitmapSource>      ipWICBitmapSource( pWICBitmapSource );
    CComPtr<IWICStream>            ipStream;
    CComPtr<IWICBitmapEncoder>     ipEncoder;
    CComPtr<IWICBitmapFrameEncode> ipFrameEncode;
    unsigned int                   uiWidth  = 0;
    unsigned int                   uiHeight = 0;

    hr = ipWICImagingFactory->CreateStream( &ipStream );
    if( SUCCEEDED( hr ) )
    {
        hr = ipStream->InitializeFromFilename( lpcwFileName, GENERIC_WRITE );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ipWICImagingFactory->CreateEncoder( guidContainerFormat, NULL, &ipEncoder );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipEncoder->Initialize( ipStream, WICBitmapEncoderNoCache );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipEncoder->CreateNewFrame( &ipFrameEncode, NULL );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipFrameEncode->Initialize( NULL );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipWICBitmapSource->GetSize( &uiWidth, &uiHeight );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipFrameEncode->SetSize( uiWidth, uiHeight );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipFrameEncode->SetPixelFormat( &format );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipFrameEncode->WriteSource( ipWICBitmapSource, NULL );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipFrameEncode->Commit();
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ipEncoder->Commit();
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
/// class CDXGICapture
//

namespace nsDXGI
{
    CDXGICapture::CDXGICapture()
        : m_csLock()
        , m_bInitialized( FALSE )
        , m_lD3DFeatureLevel( D3D_FEATURE_LEVEL_INVALID )
    {
        RtlZeroMemory( &m_rendererInfo, sizeof( m_rendererInfo ) );
        RtlZeroMemory( &m_mouseInfo, sizeof( m_mouseInfo ) );
        RtlZeroMemory( &m_tempMouseBuffer, sizeof( m_tempMouseBuffer ) );
        RtlZeroMemory( &m_desktopOutputDesc, sizeof( m_desktopOutputDesc ) );
    }

    CDXGICapture::~CDXGICapture()
    {
        this->Terminate();
    }

    HRESULT CDXGICapture::loadMonitorInfos( ID3D11Device* pDevice )
    {
        CHECK_POINTER( pDevice );

        HRESULT hr = S_OK;
        CComPtr<ID3D11Device> ipDevice( pDevice );

        // Get DXGI device
        CComPtr<IDXGIDevice> ipDxgiDevice;
        hr = ipDevice->QueryInterface( IID_PPV_ARGS( &ipDxgiDevice ) );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // Get DXGI adapter
        CComPtr<IDXGIAdapter> ipDxgiAdapter;
        hr = ipDxgiDevice->GetParent( IID_PPV_ARGS( &ipDxgiAdapter ) );
        if( FAILED( hr ) )
        {
            return hr;
        }
        ipDxgiDevice = nullptr;

        CComPtr<IDXGIOutput> ipDxgiOutput;
        for( UINT i = 0; SUCCEEDED( hr ); ++i )
        {
            ipDxgiOutput = nullptr;
            hr = ipDxgiAdapter->EnumOutputs( i, &ipDxgiOutput );
            if( ( nullptr != ipDxgiOutput ) && ( hr != DXGI_ERROR_NOT_FOUND ) )
            {
                DXGI_OUTPUT_DESC DesktopDesc;
                hr = ipDxgiOutput->GetDesc( &DesktopDesc );
                if( FAILED( hr ) )
                {
                    continue;
                }

                tagDublicatorMonitorInfo* pInfo;
                pInfo = new ( std::nothrow ) tagDublicatorMonitorInfo;
                if( nullptr == pInfo )
                {
                    return E_OUTOFMEMORY;
                }

                hr = DXGICaptureHelper::ConvertDxgiOutputToMonitorInfo( &DesktopDesc, i, pInfo );
                if( FAILED( hr ) )
                {
                    delete pInfo;
                    continue;
                }

                m_monitorInfos.push_back( pInfo );
            }
        }

        ipDxgiOutput = nullptr;
        ipDxgiAdapter = nullptr;

        return S_OK;
    }

    void CDXGICapture::freeMonitorInfos()
    {
        size_t nCount = m_monitorInfos.size();
        if( nCount == 0 )
        {
            return;
        }
        DublicatorMonitorInfoVec::iterator it = m_monitorInfos.begin();
        DublicatorMonitorInfoVec::iterator end = m_monitorInfos.end();
        for( size_t i = 0; ( i < nCount ) && ( it != end ); i++, it++ )
        {
            tagDublicatorMonitorInfo* pInfo = *it;
            if( nullptr != pInfo )
            {
                delete pInfo;
            }
        }
        m_monitorInfos.clear();
    }

    HRESULT CDXGICapture::createDeviceResource( const tagScreenCaptureFilterConfig* pConfig, const tagDublicatorMonitorInfo* pSelectedMonitorInfo )
    {
        HRESULT hr = S_OK;

        CComPtr<IDXGIOutputDuplication> ipDxgiOutputDuplication;
        CComPtr<ID3D11Texture2D>        ipCopyTexture2D;
        CComPtr<ID2D1Device>            ipD2D1Device;
        CComPtr<ID2D1DeviceContext>     ipD2D1DeviceContext;
        CComPtr<ID2D1Factory>           ipD2D1Factory;
        CComPtr<IWICImagingFactory>     ipWICImageFactory;
        CComPtr<IWICBitmap>             ipWICOutputBitmap;
        CComPtr<ID2D1RenderTarget>      ipD2D1RenderTarget;
        DXGI_OUTPUT_DESC                dgixOutputDesc;
        tagRendererInfo                 rendererInfo;

        RtlZeroMemory( &dgixOutputDesc, sizeof( dgixOutputDesc ) );
        RtlZeroMemory( &rendererInfo, sizeof( rendererInfo ) );

        // copy configuration to renderer info
        rendererInfo.MonitorIdx = pConfig->MonitorIdx;
        rendererInfo.ShowCursor = pConfig->ShowCursor;
        rendererInfo.RotationMode = pConfig->RotationMode;
        rendererInfo.SizeMode = pConfig->SizeMode;
        rendererInfo.OutputSize = pConfig->OutputSize;
        // default
        rendererInfo.ScaleX = 1.0f;
        rendererInfo.ScaleY = 1.0f;

        do
        {
            // Get DXGI factory
            CComPtr<IDXGIDevice> ipDxgiDevice;
            hr = m_ipD3D11Device->QueryInterface( IID_PPV_ARGS( &ipDxgiDevice ) );
            CHECK_HR_BREAK( hr );

            CComPtr<IDXGIAdapter> ipDxgiAdapter;
            hr = ipDxgiDevice->GetParent( IID_PPV_ARGS( &ipDxgiAdapter ) );
            CHECK_HR_BREAK( hr );

            // Get output
            CComPtr<IDXGIOutput> ipDxgiOutput;
            hr = ipDxgiAdapter->EnumOutputs( rendererInfo.MonitorIdx, &ipDxgiOutput );
            CHECK_HR_BREAK( hr );

            // Get output description
            hr = ipDxgiOutput->GetDesc( &dgixOutputDesc );
            CHECK_HR_BREAK( hr );

            tagDublicatorMonitorInfo curMonInfo;
            hr = DXGICaptureHelper::ConvertDxgiOutputToMonitorInfo( &dgixOutputDesc, rendererInfo.MonitorIdx, &curMonInfo );
            CHECK_HR_BREAK( hr );

            if( !DXGICaptureHelper::IsEqualMonitorInfo( pSelectedMonitorInfo, &curMonInfo ) )
            {
                hr = E_INVALIDARG; // Monitor settings have changed ???
                break;
            }

            // QI for Output 1
            CComPtr<IDXGIOutput1> ipDxgiOutput1;
            hr = ipDxgiOutput->QueryInterface( IID_PPV_ARGS( &ipDxgiOutput1 ) );
            CHECK_HR_BREAK( hr );

            // Create desktop duplication
            hr = ipDxgiOutput1->DuplicateOutput( m_ipD3D11Device, &ipDxgiOutputDuplication );
            CHECK_HR_BREAK( hr );

            DXGI_OUTDUPL_DESC dxgiOutputDuplDesc;
            ipDxgiOutputDuplication->GetDesc( &dxgiOutputDuplDesc );

            hr = DXGICaptureHelper::CalculateRendererInfo( &dxgiOutputDuplDesc, &rendererInfo );
            CHECK_HR_BREAK( hr );

            // Create CPU access texture
            D3D11_TEXTURE2D_DESC desc;
            desc.Width = rendererInfo.SrcBounds.Width;
            desc.Height = rendererInfo.SrcBounds.Height;
            desc.Format = rendererInfo.SrcFormat;
            desc.ArraySize = 1;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.MipLevels = 1;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            desc.Usage = D3D11_USAGE_STAGING;

            hr = m_ipD3D11Device->CreateTexture2D( &desc, NULL, &ipCopyTexture2D );
            CHECK_HR_BREAK( hr );

            if( nullptr == ipCopyTexture2D )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

    #pragma region <For_2D_operations>

            // Create D2D1 device
            UINT uiFlags = m_ipD3D11Device->GetCreationFlags();
            D2D1_CREATION_PROPERTIES d2d1Props = D2D1::CreationProperties
            (
                ( uiFlags & D3D11_CREATE_DEVICE_SINGLETHREADED )
                ? D2D1_THREADING_MODE_SINGLE_THREADED
                : D2D1_THREADING_MODE_MULTI_THREADED,
                D2D1_DEBUG_LEVEL_NONE,
                ( uiFlags & D3D11_CREATE_DEVICE_SINGLETHREADED )
                ? D2D1_DEVICE_CONTEXT_OPTIONS_NONE
                : D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS
            );
            hr = D2D1CreateDevice( ipDxgiDevice, d2d1Props, &ipD2D1Device );
            CHECK_HR_BREAK( hr );

            // Get D2D1 factory
            ipD2D1Device->GetFactory( &ipD2D1Factory );

            if( nullptr == ipD2D1Factory )
            {
                hr = D2DERR_INVALID_CALL;
                break;
            }

            //create WIC factory
            hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IWICImagingFactory,
                reinterpret_cast< void** >( &ipWICImageFactory )
            );
            CHECK_HR_BREAK( hr );

            // create D2D1 target bitmap for render
            hr = ipWICImageFactory->CreateBitmap(
                ( UINT )rendererInfo.OutputSize.Width,
                ( UINT )rendererInfo.OutputSize.Height,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapCacheOnDemand,
                &ipWICOutputBitmap );
            CHECK_HR_BREAK( hr );

            if( nullptr == ipWICOutputBitmap )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // create a D2D1 render target (for D2D1 drawing)
            D2D1_RENDER_TARGET_PROPERTIES d2d1RenderTargetProp = D2D1::RenderTargetProperties
            (
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED ),
                0.0f, // default dpi
                0.0f, // default dpi
                D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE
            );
            hr = ipD2D1Factory->CreateWicBitmapRenderTarget(
                ipWICOutputBitmap,
                d2d1RenderTargetProp,
                &ipD2D1RenderTarget
            );
            CHECK_HR_BREAK( hr );

    #pragma endregion </For_2D_operations>

        } while( false );

        if( SUCCEEDED( hr ) )
        {
            // copy output parameters
            memcpy_s( ( void* )&m_rendererInfo, sizeof( m_rendererInfo ), ( const void* )&rendererInfo, sizeof( m_rendererInfo ) );

            // set parameters
            m_desktopOutputDesc = dgixOutputDesc;

            m_ipDxgiOutputDuplication = ipDxgiOutputDuplication;
            m_ipCopyTexture2D = ipCopyTexture2D;

            m_ipD2D1Device = ipD2D1Device;
            m_ipD2D1Factory = ipD2D1Factory;
            m_ipWICImageFactory = ipWICImageFactory;
            m_ipWICOutputBitmap = ipWICOutputBitmap;
            m_ipD2D1RenderTarget = ipD2D1RenderTarget;
        }

        return S_OK;
    }

    void CDXGICapture::terminateDeviceResource()
    {
        m_ipDxgiOutputDuplication = nullptr;
        m_ipCopyTexture2D = nullptr;

        m_ipD2D1Device = nullptr;
        m_ipD2D1Factory = nullptr;
        m_ipWICImageFactory = nullptr;
        m_ipWICOutputBitmap = nullptr;
        m_ipD2D1RenderTarget = nullptr;

        // clear config parameters
        RtlZeroMemory( &m_rendererInfo, sizeof( m_rendererInfo ) );

        // clear mouse information parameters
        if( m_mouseInfo.PtrShapeBuffer != nullptr )
        {
            delete[] m_mouseInfo.PtrShapeBuffer;
            m_mouseInfo.PtrShapeBuffer = nullptr;
        }
        RtlZeroMemory( &m_mouseInfo, sizeof( m_mouseInfo ) );

        // clear temp temp buffer
        if( m_tempMouseBuffer.Buffer != nullptr )
        {
            delete[] m_tempMouseBuffer.Buffer;
            m_tempMouseBuffer.Buffer = nullptr;
        }
        RtlZeroMemory( &m_tempMouseBuffer, sizeof( m_tempMouseBuffer ) );

        // clear desktop output desc
        RtlZeroMemory( &m_desktopOutputDesc, sizeof( m_desktopOutputDesc ) );
    }

    HRESULT CDXGICapture::captureFrame( BOOL* pRetIsTimeout, UINT* pRetRenderDuration )
    {
        AUTOLOCK();
        HRESULT hRet = S_OK;

        do
        {
            if( nullptr != pRetIsTimeout )
                *pRetIsTimeout = FALSE;

            if( nullptr != pRetRenderDuration )
                *pRetRenderDuration = 0xFFFFFFFF;

            if( !m_bInitialized )
            {
                hRet = D2DERR_NOT_INITIALIZED;
                break;
            }
            if( nullptr == ( m_ipDxgiOutputDuplication ) )
            {
                hRet = E_INVALIDARG;
                break;
            }

            hRet = DXGICaptureHelper::IsRendererInfoValid( &m_rendererInfo );
            if( FAILED( hRet ) )
                break;

            DXGI_OUTDUPL_FRAME_INFO     FrameInfo;
            CComPtr<IDXGIResource>      ipDesktopResource;
            CComPtr<ID3D11Texture2D>    ipAcquiredDesktopImage;
            CComPtr<ID2D1Bitmap>        ipD2D1SourceBitmap;

            std::chrono::high_resolution_clock::time_point startTick;
            if( nullptr != pRetRenderDuration )
            {
                startTick = std::chrono::high_resolution_clock::now();
            }

            while( true )
            {
                // Get new frame
                m_ipDxgiOutputDuplication->ReleaseFrame();
                Sleep( 50 );
                hRet = m_ipDxgiOutputDuplication->AcquireNextFrame( 1000, &FrameInfo, &ipDesktopResource );
                if( FAILED( hRet ) )
                {
                    if( hRet != DXGI_ERROR_WAIT_TIMEOUT )
                    {
                        if( ipDesktopResource )
                        {
                            ipDesktopResource.Release();
                        }
                    }
                    continue;
                }

                if( FrameInfo.LastPresentTime.QuadPart )
                    break;

            }

            // QI for ID3D11Texture2D
            hRet = ipDesktopResource->QueryInterface( IID_PPV_ARGS( &ipAcquiredDesktopImage ) );
            ipDesktopResource = nullptr;
            CHECK_HR_RETURN( hRet );

            if( nullptr == ipAcquiredDesktopImage )
            {
                // release frame
                m_ipDxgiOutputDuplication->ReleaseFrame();
                return E_OUTOFMEMORY;
            }

            // Copy needed full part of desktop image
            m_ipD3D11DeviceContext->CopyResource( m_ipCopyTexture2D, ipAcquiredDesktopImage );
            if( m_rendererInfo.ShowCursor )
            {
                hRet = DXGICaptureHelper::GetMouse( m_ipDxgiOutputDuplication, &m_mouseInfo, &FrameInfo, ( UINT )m_rendererInfo.MonitorIdx, m_desktopOutputDesc.DesktopCoordinates.left, m_desktopOutputDesc.DesktopCoordinates.top );
                if( SUCCEEDED( hRet ) && m_mouseInfo.Visible )
                {
                    hRet = DXGICaptureHelper::DrawMouse( &m_mouseInfo, &m_desktopOutputDesc, &m_tempMouseBuffer, m_ipCopyTexture2D );
                }

                if( FAILED( hRet ) )
                {
                    // release frame
                    m_ipDxgiOutputDuplication->ReleaseFrame();
                    return hRet;
                }
            }

            // release frame
            hRet = m_ipDxgiOutputDuplication->ReleaseFrame();
            CHECK_HR_RETURN( hRet );

            // create D2D1 source bitmap
            hRet = DXGICaptureHelper::CreateBitmap( m_ipD2D1RenderTarget, m_ipCopyTexture2D, &ipD2D1SourceBitmap );
            CHECK_HR_RETURN( hRet );

            D2D1_RECT_F rcSource = D2D1::RectF( ( FLOAT )m_rendererInfo.SrcBounds.X,
                                                ( FLOAT )m_rendererInfo.SrcBounds.Y,
                                                ( FLOAT )( m_rendererInfo.SrcBounds.X + m_rendererInfo.SrcBounds.Width ),
                                                ( FLOAT )( m_rendererInfo.SrcBounds.Y + m_rendererInfo.SrcBounds.Height ) );
            D2D1_RECT_F rcTarget = D2D1::RectF( ( FLOAT )m_rendererInfo.DstBounds.X,
                                                ( FLOAT )m_rendererInfo.DstBounds.Y,
                                                ( FLOAT )( m_rendererInfo.DstBounds.X + m_rendererInfo.DstBounds.Width ),
                                                ( FLOAT )( m_rendererInfo.DstBounds.Y + m_rendererInfo.DstBounds.Height ) );
            D2D1_POINT_2F ptTransformCenter = D2D1::Point2F( m_rendererInfo.OutputSize.Width / 2.0f, m_rendererInfo.OutputSize.Height / 2.0f );

            // Apply the rotation transform to the render target.
            D2D1::Matrix3x2F rotate = D2D1::Matrix3x2F::Rotation(
                m_rendererInfo.RotationDegrees,
                ptTransformCenter
            );

            D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(
                D2D1::SizeF( m_rendererInfo.ScaleX, m_rendererInfo.ScaleY ),
                ptTransformCenter
            );

            // Priority: first rotate, after scale...
            m_ipD2D1RenderTarget->SetTransform( rotate * scale );

            m_ipD2D1RenderTarget->BeginDraw();
            // clear background color
            m_ipD2D1RenderTarget->Clear( D2D1::ColorF( D2D1::ColorF::Black, 1.0f ) );
            m_ipD2D1RenderTarget->DrawBitmap( ipD2D1SourceBitmap, rcTarget, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rcSource );
            // Reset transform
            //m_ipD2D1RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            // Logo draw sample
            //m_ipD2D1RenderTarget->DrawBitmap(ipBmpLogo, D2D1::RectF(0, 0, 2 * 200, 2 * 46));
            hRet = m_ipD2D1RenderTarget->EndDraw();
            if( FAILED( hRet ) )
            {
                return hRet;
            }

            // calculate render time without save
            if( nullptr != pRetRenderDuration )
            {
                *pRetRenderDuration = ( UINT )( ( std::chrono::high_resolution_clock::now() - startTick ).count() / 10000 );
            }

            hRet = S_OK;

        } while( false );

        return hRet;
    }

    QPixmap CDXGICapture::convertWICBitmapToQPixmap( IWICImagingFactory* pWICImagingFactory, IWICBitmapSource* pWICBitmapSource )
    {
        if( !pWICBitmapSource )
            return QPixmap();

        // 비트맵 크기와 포맷 정보 가져오기
        UINT width = 0, height = 0;
        pWICBitmapSource->GetSize( &width, &height );

        WICPixelFormatGUID pixelFormat;
        pWICBitmapSource->GetPixelFormat( &pixelFormat );

        // 최적화: QImage 객체를 직접 생성하고 해당 버퍼로 바로 복사
        // QImage가 자체적으로 메모리를 할당하고 관리함
        QImage image( width, height, QImage::Format_ARGB32_Premultiplied );

        if( image.isNull() )
            return QPixmap();

        // QImage 버퍼에 직접 복사
        UINT stride = width * 4; // 32bpp = 4 bytes per pixel

        // 최적화: 만약 이미 BGRA 형식이라면 변환 없이 직접 복사
        if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppBGRA ) )
        {
            // 직접 QImage 버퍼로 복사 (convertToFormat을 피함)
            HRESULT hr = pWICBitmapSource->CopyPixels(
                nullptr,
                image.bytesPerLine(),
                image.bytesPerLine() * height,
                static_cast< BYTE* >( image.bits() )
            );

            if( FAILED( hr ) )
                return QPixmap();
        }
        else
        {
            // BGRA로 변환이 필요한 경우
            IWICImagingFactory* pFactory = nullptr;
            IWICFormatConverter* pConverter = nullptr;

            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_IWICImagingFactory,
                ( LPVOID* )&pFactory
            );

            if( SUCCEEDED( hr ) )
            {
                hr = pFactory->CreateFormatConverter( &pConverter );
                if( SUCCEEDED( hr ) )
                {
                    hr = pConverter->Initialize(
                        pWICBitmapSource,
                        GUID_WICPixelFormat32bppBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.0f,
                        WICBitmapPaletteTypeCustom
                    );

                    if( SUCCEEDED( hr ) )
                    {
                        // 변환된 데이터를 QImage 버퍼로 직접 복사
                        hr = pConverter->CopyPixels(
                            nullptr,
                            image.bytesPerLine(),
                            image.bytesPerLine() * height,
                            static_cast< BYTE* >( image.bits() )
                        );
                    }
                }

                if( pConverter ) pConverter->Release();
                if( pFactory ) pFactory->Release();

                if( FAILED( hr ) )
                    return QPixmap();
            }
        }

        // 최적화: rgbSwapped()를 사용하면 추가 메모리 할당이 발생하므로,
        // BGRA -> RGBA 변환은 Qt의 Format_ARGB32로 해석하여 처리
        // (Qt에서는 ARGB32 포맷이지만 바이트 순서가 실제로는 BGRA와 일치함)

        // 최적화: 메모리 복사를 최소화하기 위해 QPixmap::fromImage()를 사용하여
        // 변환 과정에서 추가 복사 없이 직접 QPixmap 생성
        QPixmap pixmap = QPixmap::fromImage( std::move( image ) );

        return pixmap;
    }

    HRESULT CDXGICapture::Initialize()
    {
        AUTOLOCK();
        if( m_bInitialized )
        {
            return HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED ); // already initialized
        }

        HRESULT hr = S_OK;
        D3D_FEATURE_LEVEL lFeatureLevel;
        CComPtr<ID3D11Device> ipDevice;
        CComPtr<ID3D11DeviceContext> ipDeviceContext;

        //// required for monitor dpi problem (???)
        //SetProcessDpiAwareness( PROCESS_PER_MONITOR_DPI_AWARE );

        // Create device
        for( UINT i = 0; i < g_NumDriverTypes; ++i )
        {
            hr = D3D11CreateDevice( nullptr,
                                    g_DriverTypes[ i ],
                                    nullptr,
                                    /* D3D11_CREATE_DEVICE_BGRA_SUPPORT
                                    * This flag adds support for surfaces with a different
                                    * color channel ordering than the API default.
                                    * You need it for compatibility with Direct2D. */
                                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                    g_FeatureLevels,
                                    g_NumFeatureLevels,
                                    D3D11_SDK_VERSION,
                                    &ipDevice,
                                    &lFeatureLevel,
                                    &ipDeviceContext );

            if( SUCCEEDED( hr ) )
            {
                // Device creation success, no need to loop anymore
                break;
            }

            ipDevice = nullptr;
            ipDeviceContext = nullptr;
        }

        if( FAILED( hr ) )
        {
            return hr;
        }

        if( nullptr == ipDevice )
        {
            return E_UNEXPECTED;
        }

        // load all monitor informations
        hr = loadMonitorInfos( ipDevice );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // set common fields
        m_lD3DFeatureLevel = lFeatureLevel;
        m_ipD3D11Device = ipDevice;
        m_ipD3D11DeviceContext = ipDeviceContext;

        m_bInitialized = TRUE;

        return S_OK;
    }

    HRESULT CDXGICapture::Terminate()
    {
        AUTOLOCK();
        if( !m_bInitialized )
        {
            return S_FALSE; // already terminated
        }

        this->terminateDeviceResource();

        m_ipD3D11Device = nullptr;
        m_ipD3D11DeviceContext = nullptr;
        m_lD3DFeatureLevel = D3D_FEATURE_LEVEL_INVALID;

        freeMonitorInfos();

        m_bInitialized = FALSE;
        return S_OK;
    }

    HRESULT CDXGICapture::SetConfig( const tagScreenCaptureFilterConfig* pConfig )
    {
        AUTOLOCK();
        if( !m_bInitialized )
        {
            return D2DERR_NOT_INITIALIZED;
        }

        if( nullptr == pConfig )
        {
            return E_INVALIDARG;
        }

        // terminate old resources
        this->terminateDeviceResource();

        HRESULT hr = S_OK;
        const tagDublicatorMonitorInfo* pSelectedMonitorInfo = nullptr;

        pSelectedMonitorInfo = this->FindDublicatorMonitorInfo( pConfig->MonitorIdx );
        if( nullptr == pSelectedMonitorInfo )
        {
            return E_INVALIDARG;
        }

        hr = this->createDeviceResource( pConfig, pSelectedMonitorInfo );
        if( FAILED( hr ) )
        {
            return hr;
        }

        m_config = *pConfig;
        return hr;
    }

    HRESULT CDXGICapture::SetConfig( const tagScreenCaptureFilterConfig& config )
    {
        return this->SetConfig( &config );
    }

    BOOL CDXGICapture::IsInitialized() const
    {
        AUTOLOCK();
        return m_bInitialized;
    }

    D3D_FEATURE_LEVEL CDXGICapture::GetD3DFeatureLevel() const
    {
        AUTOLOCK();
        return m_lD3DFeatureLevel;
    }

    int CDXGICapture::GetDublicatorMonitorInfoCount() const
    {
        AUTOLOCK();
        return ( int )m_monitorInfos.size();
    }

    const tagDublicatorMonitorInfo* CDXGICapture::GetDublicatorMonitorInfo( int index ) const
    {
        AUTOLOCK();

        size_t nCount = m_monitorInfos.size();
        if( ( index < 0 ) || ( index >= ( int )nCount ) )
        {
            return nullptr;
        }

        return m_monitorInfos[ index ];
    } // GetDublicatorMonitorInfo

    const tagDublicatorMonitorInfo* CDXGICapture::FindDublicatorMonitorInfo( int monitorIdx ) const
    {
        AUTOLOCK();

        size_t nCount = m_monitorInfos.size();
        if( nCount == 0 )
        {
            return nullptr;
        }
        DublicatorMonitorInfoVec::const_iterator it = m_monitorInfos.begin();
        DublicatorMonitorInfoVec::const_iterator end = m_monitorInfos.end();
        for( size_t i = 0; ( i < nCount ) && ( it != end ); i++, it++ )
        {
            tagDublicatorMonitorInfo* pInfo = *it;
            if( monitorIdx == pInfo->Idx )
            {
                return pInfo;
            }
        }

        return nullptr;
    } // FindDublicatorMonitorInfo

    //
    // CaptureToFile
    //
    HRESULT CDXGICapture::CaptureToFile( _In_ LPCWSTR lpcwOutputFileName, _Out_opt_ BOOL* pRetIsTimeout /*= NULL*/, _Out_opt_ UINT* pRetRenderDuration /*= NULL*/ )
    {
        AUTOLOCK();

        if( nullptr != pRetIsTimeout )
        {
            *pRetIsTimeout = FALSE;
        }

        if( nullptr != pRetRenderDuration )
        {
            *pRetRenderDuration = 0xFFFFFFFF;
        }

        if( !m_bInitialized )
        {
            return D2DERR_NOT_INITIALIZED;
        }

        CHECK_POINTER_EX( m_ipDxgiOutputDuplication, E_INVALIDARG );
        CHECK_POINTER_EX( lpcwOutputFileName, E_INVALIDARG );

        HRESULT hr = S_OK;

        hr = DXGICaptureHelper::IsRendererInfoValid( &m_rendererInfo );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // is valid?
        hr = DXGICaptureHelper::GetContainerFormatByFileName( lpcwOutputFileName );
        if( FAILED( hr ) )
        {
            return hr;
        }

        DXGI_OUTDUPL_FRAME_INFO     FrameInfo;
        CComPtr<IDXGIResource>      ipDesktopResource;
        CComPtr<ID3D11Texture2D>    ipAcquiredDesktopImage;
        CComPtr<ID2D1Bitmap>        ipD2D1SourceBitmap;

        std::chrono::high_resolution_clock::time_point startTick;
        if( nullptr != pRetRenderDuration )
        {
            startTick = std::chrono::high_resolution_clock::now();
        }

        // Get new frame
        hr = m_ipDxgiOutputDuplication->AcquireNextFrame( 1000, &FrameInfo, &ipDesktopResource );
        if( hr == DXGI_ERROR_WAIT_TIMEOUT )
        {
            if( nullptr != pRetIsTimeout )
            {
                *pRetIsTimeout = TRUE;
            }
            return S_FALSE;
        }
        else if( FAILED( hr ) )
        {
            return hr;
        }

        // QI for ID3D11Texture2D
        hr = ipDesktopResource->QueryInterface( IID_PPV_ARGS( &ipAcquiredDesktopImage ) );
        ipDesktopResource = nullptr;
        CHECK_HR_RETURN( hr );

        if( nullptr == ipAcquiredDesktopImage )
        {
            // release frame
            m_ipDxgiOutputDuplication->ReleaseFrame();
            return E_OUTOFMEMORY;
        }

        // Copy needed full part of desktop image
        m_ipD3D11DeviceContext->CopyResource( m_ipCopyTexture2D, ipAcquiredDesktopImage );

        if( m_rendererInfo.ShowCursor )
        {
            hr = DXGICaptureHelper::GetMouse( m_ipDxgiOutputDuplication, &m_mouseInfo, &FrameInfo, ( UINT )m_rendererInfo.MonitorIdx, m_desktopOutputDesc.DesktopCoordinates.left, m_desktopOutputDesc.DesktopCoordinates.top );
            if( SUCCEEDED( hr ) && m_mouseInfo.Visible )
            {
                hr = DXGICaptureHelper::DrawMouse( &m_mouseInfo, &m_desktopOutputDesc, &m_tempMouseBuffer, m_ipCopyTexture2D );
            }

            if( FAILED( hr ) )
            {
                // release frame
                m_ipDxgiOutputDuplication->ReleaseFrame();
                return hr;
            }
        }

        // release frame
        hr = m_ipDxgiOutputDuplication->ReleaseFrame();
        CHECK_HR_RETURN( hr );

        // create D2D1 source bitmap
        hr = DXGICaptureHelper::CreateBitmap( m_ipD2D1RenderTarget, m_ipCopyTexture2D, &ipD2D1SourceBitmap );
        CHECK_HR_RETURN( hr );

        D2D1_RECT_F rcSource = D2D1::RectF( ( FLOAT )m_rendererInfo.SrcBounds.X,
                                            ( FLOAT )m_rendererInfo.SrcBounds.Y,
                                            ( FLOAT )( m_rendererInfo.SrcBounds.X + m_rendererInfo.SrcBounds.Width ),
                                            ( FLOAT )( m_rendererInfo.SrcBounds.Y + m_rendererInfo.SrcBounds.Height ) );
        D2D1_RECT_F rcTarget = D2D1::RectF( ( FLOAT )m_rendererInfo.DstBounds.X,
                                            ( FLOAT )m_rendererInfo.DstBounds.Y,
                                            ( FLOAT )( m_rendererInfo.DstBounds.X + m_rendererInfo.DstBounds.Width ),
                                            ( FLOAT )( m_rendererInfo.DstBounds.Y + m_rendererInfo.DstBounds.Height ) );
        D2D1_POINT_2F ptTransformCenter = D2D1::Point2F( m_rendererInfo.OutputSize.Width / 2.0f, m_rendererInfo.OutputSize.Height / 2.0f );

        // Apply the rotation transform to the render target.
        D2D1::Matrix3x2F rotate = D2D1::Matrix3x2F::Rotation(
            m_rendererInfo.RotationDegrees,
            ptTransformCenter
        );

        D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(
            D2D1::SizeF( m_rendererInfo.ScaleX, m_rendererInfo.ScaleY ),
            ptTransformCenter
        );

        // Priority: first rotate, after scale...
        m_ipD2D1RenderTarget->SetTransform( rotate * scale );

        m_ipD2D1RenderTarget->BeginDraw();
        // clear background color
        m_ipD2D1RenderTarget->Clear( D2D1::ColorF( D2D1::ColorF::Black, 1.0f ) );
        m_ipD2D1RenderTarget->DrawBitmap( ipD2D1SourceBitmap, rcTarget, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rcSource );
        // Reset transform
        //m_ipD2D1RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        // Logo draw sample
        //m_ipD2D1RenderTarget->DrawBitmap(ipBmpLogo, D2D1::RectF(0, 0, 2 * 200, 2 * 46));
        hr = m_ipD2D1RenderTarget->EndDraw();
        if( FAILED( hr ) )
        {
            return hr;
        }

        // calculate render time without save
        if( nullptr != pRetRenderDuration )
        {
            *pRetRenderDuration = ( UINT )( ( std::chrono::high_resolution_clock::now() - startTick ).count() / 10000 );
        }

        hr = DXGICaptureHelper::SaveImageToFile( m_ipWICImageFactory, m_ipWICOutputBitmap, lpcwOutputFileName );
        if( FAILED( hr ) )
        {
            return hr;
        }

        return S_OK;
    } // CaptureToFile

    QPixmap CDXGICapture::CaptureToPixmap( LPCWSTR lpcwOutputFileName, BOOL* pRetIsTimeout, UINT* pRetRenderDuration )
    {
        QPixmap Pixmap;

        do
        {
            HRESULT hRet = captureFrame( pRetIsTimeout, pRetRenderDuration );
            if( FAILED( hRet ) )
                break;

            Pixmap = convertWICBitmapToQPixmap( m_ipWICImageFactory, m_ipWICOutputBitmap );

        } while( false );

        return Pixmap;
    }
}
