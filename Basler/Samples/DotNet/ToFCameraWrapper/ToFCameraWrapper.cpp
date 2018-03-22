#include "stdafx.h"
#include "ToFCameraWrapper.h"
#include "Native.h"

using namespace System;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Runtime::InteropServices;
using namespace System::Drawing;
using namespace msclr::interop;


delegate bool ImageGrabbedNative( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts parts );

// Catches a native exception and translates it into a managed exception.
#define CATCH_TRANSLATE() \
    catch ( GenICam::GenericException& e ) { throw gcnew ToFCameraWrapper::CameraException(e); } \
    catch ( std::exception& e) { throw gcnew ToFCameraWrapper::CameraException(e); } \
    catch ( ... ) { throw gcnew ToFCameraWrapper::CameraException("Unhandled exception occurred"); } \

namespace ToFCameraWrapperImpl
{
    private ref class CameraInfoImpl : public ToFCameraWrapper::CameraInfo
    {
    public:
        CameraInfoImpl(const GenTLConsumerImplHelper::CameraInfo& info)
        {
            m_DeviceID = gcnew System::String(info.strDeviceID.c_str());
            m_DisplayName = gcnew System::String(info.strDisplayName.c_str());
            m_SerialNumber = gcnew System::String(info.strSerialNumber.c_str());
            m_ModelName = gcnew System::String(info.strModelName.c_str());
            m_UserDefinedName = gcnew System::String(info.strUserDefinedName.c_str());
            m_IpAddr = gcnew System::String("N/A");
        }

        static GenTLConsumerImplHelper::CameraInfo ToUnmanaged( ToFCameraWrapper::CameraInfo% cameraInfo)
        {
            msclr::interop::marshal_context^ mctxt = gcnew msclr::interop::marshal_context();
            GenTLConsumerImplHelper::CameraInfo unmanagedCameraInfo;
            unmanagedCameraInfo.strDeviceID = mctxt->marshal_as<const char*>(cameraInfo.DeviceID);
            unmanagedCameraInfo.strDisplayName = mctxt->marshal_as<const char*>(cameraInfo.DisplayName);
            unmanagedCameraInfo.strModelName = mctxt->marshal_as<const char*>(cameraInfo.ModelName);
            unmanagedCameraInfo.strSerialNumber = mctxt->marshal_as<const char*>(cameraInfo.SerialNumber);
            unmanagedCameraInfo.strUserDefinedName = mctxt->marshal_as<const char*>(cameraInfo.UserDefinedName);
            unmanagedCameraInfo.strIpAddress = mctxt->marshal_as<const char*>(cameraInfo.IpAddress);
            return unmanagedCameraInfo;
        }

    private:
        msclr::interop::marshal_context^ m_ctxt;

    };
}

ToFCameraWrapper::ToFCamera::ToFCamera()
    : m_isGrabbing(false)
    , m_GrabDelegate(new gcroot<ImageGrabbedNative^>(gcnew ImageGrabbedNative( this, &ToFCamera::onImageGrabbed )))

{
    try
    {
        int nCameras = System::Threading::Interlocked::Increment(s_nCameras);
        // When creating the first camera, the producer must be initialized first
        if (nCameras == 1)
        {
            GenTLConsumerImplHelper::CToFCamera::InitProducer();
        }
        m_pCamera = new Native::CToFCameraEx();
    }
    CATCH_TRANSLATE();
}

ToFCameraWrapper::ToFCamera::~ToFCamera()
{
    this->!ToFCamera();
}

ToFCameraWrapper::ToFCamera::!ToFCamera()
{
    try
     {
        if (IsGrabbing())
        {
            try
            {
                StopGrabbing();
            }
            catch (...)
            {
                // NOOP
            }
        }
        if ( m_pCamera )
        {
            delete m_pCamera;
        }
        delete m_GrabDelegate;

        int nCameras = System::Threading::Interlocked::Decrement(s_nCameras);
        if (0 == nCameras)
        {
            // There are no ohter cameras left, the producer can be terminated.
            GenTLConsumerImplHelper::CToFCamera::TerminateProducer();
        }
    }
    CATCH_TRANSLATE();
}

ToFCameraWrapper::CameraList^ ToFCameraWrapper::ToFCamera::EnumerateCameras()
{
    try
    {
        GenTLConsumerImplHelper::CameraList cameras = GenTLConsumerImplHelper::CToFCamera::EnumerateCameras();
        CameraList^ returnList = gcnew CameraList((int) cameras.size());
        for ( auto element : cameras )
        {
            returnList->Add(gcnew ToFCameraWrapperImpl::CameraInfoImpl(element));
        }
        return returnList;
    }
    CATCH_TRANSLATE();
}

void ToFCameraWrapper::ToFCamera::Open(ToFCameraWrapper::CameraInfo^ cameraInfo)
{
    try
    {
        if ( cameraInfo == nullptr )
        {
            throw RUNTIME_EXCEPTION("Specified camera not found or invalid camera info.");
        }
        GenTLConsumerImplHelper::CameraInfo unmanagedCameraInfo = ToFCameraWrapperImpl::CameraInfoImpl::ToUnmanaged( *cameraInfo );
        m_pCamera->Open( unmanagedCameraInfo);
    }
    CATCH_TRANSLATE();
}

void ToFCameraWrapper::ToFCamera::OpenFirstCamera()
{
    try 
    {
        m_pCamera->OpenFirstCamera();
    }
    CATCH_TRANSLATE();
}

bool ToFCameraWrapper::ToFCamera::IsOpen()
{
     return m_pCamera->IsOpen(); 
}

bool ToFCameraWrapper::ToFCamera::IsConnected()
{
    return m_pCamera->IsConnected();
}

bool ToFCameraWrapper::ToFCamera::IsGrabbing()
{
    return m_isGrabbing;
}

void ToFCameraWrapper::ToFCamera::Close()
{
    try 
    {
        m_pCamera->Close();
    }
    CATCH_TRANSLATE();
}

System::String^ ToFCameraWrapper::ToFCamera::GetParameterValue( System::String^ name )
{
    System::String^ result;
    try 
    {
        if ( m_marshal_context == nullptr )
            m_marshal_context= gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetParameter( m_marshal_context->marshal_as<const char*>(name) ) );
        result = marshal_as<String^>(ptrValue->ToString().c_str() );
    }
    CATCH_TRANSLATE();
    return result;
}

void ToFCameraWrapper::ToFCamera::SetParameterValue( System::String^ name, System::String^ value )
{
    try 
    {
        if ( m_marshal_context == nullptr )
            m_marshal_context= gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetParameter( m_marshal_context->marshal_as<const char*>(name) ));
        ptrValue->FromString( m_marshal_context->marshal_as<const char*>(value) );
    }
    CATCH_TRANSLATE();
}

System::String^ ToFCameraWrapper::ToFCamera::GetDeviceModuleParameterValue(System::String^ name)
{
    System::String^ result;
    try
    {
        if (m_marshal_context == nullptr)
            m_marshal_context = gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetDeviceModuleParameter(m_marshal_context->marshal_as<const char*>(name)));
        result = marshal_as<String^>(ptrValue->ToString().c_str());
    }
    CATCH_TRANSLATE();
    return result;
}

void ToFCameraWrapper::ToFCamera::SetDeviceModuleParameterValue(System::String^ name, System::String^ value)
{
    try
    {
        if (m_marshal_context == nullptr)
            m_marshal_context = gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetDeviceModuleParameter(m_marshal_context->marshal_as<const char*>(name)));
        ptrValue->FromString(m_marshal_context->marshal_as<const char*>(value));
    }
    CATCH_TRANSLATE();
}

System::String^ ToFCameraWrapper::ToFCamera::GetDataStreamParameterValue(System::String^ name)
{
    System::String^ result;
    try
    {
        if (m_marshal_context == nullptr)
            m_marshal_context = gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetDataStreamParameter(m_marshal_context->marshal_as<const char*>(name)));
        result = marshal_as<String^>(ptrValue->ToString().c_str());
    }
    CATCH_TRANSLATE();
    return result;
}

void ToFCameraWrapper::ToFCamera::SetDataStreamParameterValue(System::String^ name, System::String^ value)
{
    try
    {
        if (m_marshal_context == nullptr)
            m_marshal_context = gcnew msclr::interop::marshal_context();

        GenApi::CValuePtr ptrValue(m_pCamera->GetDataStreamParameter(m_marshal_context->marshal_as<const char*>(name)));
        ptrValue->FromString(m_marshal_context->marshal_as<const char*>(value));
    }
    CATCH_TRANSLATE();
}

void ToFCameraWrapper::ToFCamera::StartGrabbing()
{
    if ( IsGrabbing() )
    {
        throw gcnew CameraException("Grabbing already has been started.");
    }
    m_isGrabbing = true;
    m_GrabThread = Task::Run( gcnew Action( this, &ToFCamera::GrabLoop ) );
}

void ToFCameraWrapper::ToFCamera::StopGrabbing()
{
    if ( IsGrabbing() )
    {
        try 
        {
            m_isGrabbing = false;
            m_pCamera->StopGrab();
        }
        CATCH_TRANSLATE();
        if ( m_GrabThread != nullptr )
        {
            try
            {
                 m_GrabThread->Wait();
            }
            catch ( AggregateException^ e)
            {
                throw e->Flatten()->InnerException;
            }
            finally
            {
                m_GrabThread = nullptr;
            }
        }
    }
}

void ToFCameraWrapper::ToFCamera::GrabLoop()
{
    try 
    {
        m_pCamera->GrabContinuous( static_cast<Native::DelegateFuncPtr>(Marshal::GetFunctionPointerForDelegate(*m_GrabDelegate).ToPointer() ));
    }
    CATCH_TRANSLATE();
}

namespace {
    template<typename T> Object^ AllocAndCopy(void* src, size_t nBytes )
    {
        array<T>^ arr = gcnew array<T>((int) nBytes / sizeof T );
        pin_ptr< T > ptrPinned = &arr[arr->GetLowerBound(0)];
        memcpy(ptrPinned, src, nBytes);
        return arr;
    }
}

// Called by the unmanaged code.
// Fill out the ImageEventArgs and fire the event.
bool ToFCameraWrapper::ToFCamera::onImageGrabbed( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts nativeParts )
{
    auto parts = gcnew System::Collections::Generic::List<ToFCameraWrapper::BufferPart^>();
    GrabResultStatus status = GrabResultStatus::Ok;
    switch ( grabResult.status )
    {
    case GenTLConsumerImplHelper::GrabResult::Failed:
        status = GrabResultStatus::Failed;
        break;
    case GenTLConsumerImplHelper::GrabResult::Timeout: 
        status = GrabResultStatus::Timeout;
        break;
    case GenTLConsumerImplHelper::GrabResult::Ok:
        status = GrabResultStatus::Ok; break;
    default: 
        throw gcnew ToFCameraWrapper::CameraException("Cannot map unknown native GrabResult status value.");
    }

    try
    {
        for ( GenTLConsumerImplHelper::BufferParts::const_iterator nativePart = nativeParts.begin(); nativePart != nativeParts.end(); ++nativePart )
        {
            if ( nativePart->size == 0 )
            {
                // There are producers with a bug that may deliver a part with size 0 in case the components are 
                // enabled/disabled when grabbing. Ignore those parts.
                continue;
            }
            ToFCameraWrapper::BufferPart ^part = gcnew ToFCameraWrapper::BufferPart();
            part->dataFormat = (PixelFormats) nativePart->dataFormat;
            part->partType = (BufferPartType) nativePart->partType;
            part->height = (uint32_t) nativePart->height;
            part->width = (uint32_t) nativePart->width;
            
            // Copy data to prevent life time issues and to provide an array of the suited type.
            switch ( part->dataFormat )
            {
            case PixelFormats::Mono16:
            case PixelFormats::Coord3D_C16:
            case PixelFormats::Confidence16:
                // These formats are 16 bit grey values per pixel.
                part->data = AllocAndCopy<UInt16>( nativePart->pData, nativePart->size );
                break;

            case PixelFormats::Coord3D_ABC32f:
                // There is a coordinate triple for each pixel.
                part->data = AllocAndCopy<Coord3D>( nativePart->pData, nativePart->size );
                break;

            case PixelFormats::RGB8:
                // There is an RGB triple for each pixel.
                part->data = AllocAndCopy<RGB8>( nativePart->pData, nativePart->size );
                break;

            default:
                continue;
            }
            parts->Add(part);
        }
    }
    CATCH_TRANSLATE();
    if ( parts->Count == 0 )
    {
       // There are producers with a bug that may deliver a part with size 0 in case the components are 
       // enabled/disabled when grabbing. These parts are ignored above, which can lead to an grab result
       // without any parts. Ignore these grab results and do not raise the event.
        return true; // indicate to continue grabbing
    }

    // Deliver the parts.
    ImageGrabbedEventArgs ^args = gcnew ImageGrabbedEventArgs( status, parts);
    ImageGrabbed( this, args );
    return ! args->stop;  // If a subscriber sets the stop flag, image acquisition will be stopped.
}
