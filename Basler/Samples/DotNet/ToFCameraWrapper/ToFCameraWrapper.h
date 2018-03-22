#pragma once

#include <msclr\marshal.h> 
#include "Native.h"

namespace ToFCameraWrapper {

    public enum class PixelFormats
    {
        Mono8=PFNC_Mono8,
        Mono16=PFNC_Mono16,
        RGB8=PFNC_RGB8,
        Coord3D_ABC32f=PFNC_Coord3D_ABC32f,
        Coord3D_C16=PFNC_Coord3D_C16,
        Confidence16=PFNC_Confidence16
    };

    public enum class BufferPartType
    {
        Range = GenTLConsumerImplHelper::Range,
        Intensity = GenTLConsumerImplHelper::Intensity,
        Confidence = GenTLConsumerImplHelper::Confidence,
        Undefined = GenTLConsumerImplHelper::Undefined
    };

    [System::Runtime::InteropServices::StructLayout(System::Runtime::InteropServices::LayoutKind::Sequential, Pack = 1)]
    public value struct Coord3D
    {
        float x;
        float y;
        float z;
    };

    [System::Runtime::InteropServices::StructLayout(System::Runtime::InteropServices::LayoutKind::Sequential, Pack = 1)]
    public value struct RGB8
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    public ref class CameraException : public System::Exception
    {
    public:
        CameraException( const GenICam::GenericException& e)
            : System::Exception( gcnew System::String(e.GetDescription()))
        {
            Source = System::String::Format("{0}:{1}", gcnew System::String(e.GetSourceFileName()), e.GetSourceLine());
        }

        CameraException( const std::exception& e)
            : System::Exception( gcnew System::String(e.what()))
        {
        }

        CameraException( const char* msg )
            : System::Exception( gcnew System::String(msg))
        {
        }

    };

    // Describes a single part of a multi-part image buffer
    public ref class BufferPart
    {
    public:
        Object^         data;          // Image data. Depending on the data format type data is an array of 16bit unsigned integer values, 
                                       // triples of float, or an RGB trible.
        uint32_t        width;         // Width of the image in pixels
        uint32_t        height;        // Height of the image in pixels
        BufferPartType  partType;      // The kind of data this part represents.
        PixelFormats    dataFormat;    // Format of the data, e.g., RGB, Mono16, or Coord3D_ABC32f

    };



    // Indicates the result of grabbing a multi-part image
    public enum class GrabResultStatus
    {
        Ok,
        Failed,
        Timeout
    };

    // The event arguments of the ImageGrabbedEvent 
    public ref class ImageGrabbedEventArgs : System::EventArgs
    {
        GrabResultStatus _status;
        System::Collections::Generic::List<BufferPart^> ^_parts;
    public:
        ImageGrabbedEventArgs(GrabResultStatus status, System::Collections::Generic::List<BufferPart^> ^parts)
        {
            this->stop = false;
            this->status = status;
            this->parts = parts;
        }

        property GrabResultStatus status
        {
            GrabResultStatus get() { return _status; }
        private:
            void set(GrabResultStatus status) {_status=status; }
        }
    
        property System::Collections::Generic::List<BufferPart^>^ parts
        {
            System::Collections::Generic::List<BufferPart^>^ get() { return _parts; }
        private:
            void set( System::Collections::Generic::List<BufferPart^>^ parts) { _parts = parts; }
        }

        property bool stop;   // If set to true, grab will be stopped.
    };

    public ref class CameraInfo
    {
    public:
        property System::String^ DeviceID
        {
            System::String^ get() { return m_DeviceID; };
        }

        property System::String^ ModelName
        {
            System::String^ get() { return m_ModelName; }
        }

        property System::String^ DisplayName
        {
            System::String^ get() { return m_ModelName; }
        }

        property System::String^ SerialNumber
        {
            System::String^ get() { return m_SerialNumber; }
        }

        property System::String^ UserDefinedName
        {
            System::String^ get() { return m_UserDefinedName; }
        }

        property System::String^ IpAddress
        {
            System::String^ get() { return m_IpAddr; }
        }

    protected:
        System::String^ m_DeviceID;
        System::String^ m_ModelName;
        System::String^ m_DisplayName;
        System::String^ m_SerialNumber;
        System::String^ m_UserDefinedName;
        System::String^ m_IpAddr;
    };

    public ref class CameraList : public System::Collections::Generic::List<CameraInfo^>
    {
    internal:
        CameraList(int nElements) : System::Collections::Generic::List<CameraInfo^>(nElements)
        {
        }
    };

    delegate bool ImageGrabbedNative( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts parts );

    // A managed wrapper for the native CToFCamera class.
    // This class serves as an example and can be easily extended to wrap more
    // functionality of the native CToFCamera class.
    public ref class ToFCamera
    {
    public:
        event System::EventHandler<ImageGrabbedEventArgs^>^ ImageGrabbed;  // Will be raised when a multi-part image was acquired.
        
        ToFCamera();
        ~ToFCamera();

        static CameraList^ EnumerateCameras();
        void Open(CameraInfo^ cameraInfo);
        void OpenFirstCamera();
        bool IsOpen();
        bool IsConnected();
        bool IsGrabbing();
        void Close();

        // Access the camera parameters.
        System::String^ GetParameterValue( System::String^  name );
        void SetParameterValue( System::String^ name, System::String^ value );

        // Access the parameters provided by the producer's device module.
        System::String^ GetDeviceModuleParameterValue(System::String^  name);
        void SetDeviceModuleParameterValue(System::String^ name, System::String^ value);

        // Access the parameters provided by the producer's stream module.
        System::String^ GetDataStreamParameterValue(System::String^  name);
        void SetDataStreamParameterValue(System::String^ name, System::String^ value);

        // Acquire images continuously. The acquisition is performed by a thread (m_GrabThread).
        void StartGrabbing();

        // Request to stop image acquisition.
        void StopGrabbing();

    private:
        void GrabLoop();  // The thread procedure

        bool onImageGrabbed( GenTLConsumerImplHelper::GrabResult grabResult, GenTLConsumerImplHelper::BufferParts parts );
        
    protected:
        // Finalizer
        !ToFCamera();

    private:
        Native::CToFCameraEx                            *m_pCamera;
        msclr::interop::marshal_context                 ^m_marshal_context;
        System::Threading::Tasks::Task                  ^m_GrabThread;
        bool                                             m_isGrabbing;
        static int                                       s_nCameras = 0;
        gcroot<ImageGrabbedNative^>*                     m_GrabDelegate;
    };



}
