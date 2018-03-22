#include "stdafx.h"
#include "Native.h"

void Native::CToFCameraEx::GrabContinuous(DelegateFuncPtr cb)
{
    m_StopRequest = false;
    m_Delegate = cb;
    GenTLConsumerImplHelper::CToFCamera::GrabContinuous( 5, 1000, this,  &Native::CToFCameraEx::onImageGrabbed );
}

void Native::CToFCameraEx::StopGrab()
{
    m_StopRequest = true;
    GenTLConsumerImplHelper::CToFCamera::StopGrab();
}

bool Native::CToFCameraEx::onImageGrabbed( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts parts )
{
    bool cont = false;
    if ( m_Delegate != NULL )
    {
        cont  = m_Delegate( grabResult, parts );
    }
    return ! m_StopRequest && cont;  // Returning false indicates to stop acquisition
}


