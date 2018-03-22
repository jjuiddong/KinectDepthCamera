#pragma once
#pragma managed ( push, off )
#define _CRT_SECURE_NO_WARNINGS
#include <ConsumerImplHelper/ToFCamera.h>
#pragma managed( pop )
namespace Native 
{
    // Delegates are basically function pointers to functions with stdcall calling convention.
    typedef bool (__stdcall *DelegateFuncPtr)( GenTLConsumerImplHelper::GrabResult , GenTLConsumerImplHelper::BufferParts  );


    /* This extension of the CToFCamera provides a native callback for the native CToFCamera::GrabContinuous function.
       The native callback calls the managed delegate provided by the managed ToFCamera class. */
   class CToFCameraEx : public GenTLConsumerImplHelper::CToFCamera
    {
    public:
        CToFCameraEx()
            : m_Delegate( NULL )
        {
        }

        void GrabContinuous( DelegateFuncPtr cb );  // Grabs until either the callback requests to stop or the StopGrab() method is called.
        void StopGrab();

        // The callback called by the native CToFCamera class
        bool onImageGrabbed( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts parts );

    private:
        bool                m_StopRequest;
        DelegateFuncPtr     m_Delegate;   
    };


}

