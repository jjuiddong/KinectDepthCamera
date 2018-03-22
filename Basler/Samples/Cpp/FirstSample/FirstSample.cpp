/* 
Access to ToF cameras is provided by a GenICam-compliant GenTL Producer. A GenTL Producer is a dynamic library
implementing a standardized software interface for accessing the camera.

The software interacting with the GentL Producer is called a GenTL Consumer. Using this terminology,
this sample is a GenTL Consumer, too.

As part of the suite of sample programs, Basler provides the ConsumerImplHelper C++ template library that serves as 
an implementation helper for GenTL consumers.

The GenICam GenApi library is used to access the camera parameters.

For more details about GenICam, GenTL, and GenApi, refer to the http://www.emva.org/standards-technology/genicam/ website.
The GenICam GenApi standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_Standard_v2_0.pdf
The GenTL standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf

This sample illustrates how to grab images from a ToF camera and how to access the image and depth data.

The GenApi sample, which is part of the Basler ToF samples, illustrates in more detail how to configure a camera.
*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip> 

#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif


using namespace GenTLConsumerImplHelper;
using namespace std;

class Sample
{
public:
    int run();

    bool onImageGrabbed( GrabResult grabResult, BufferParts );

private:
    CToFCamera  m_Camera;
    int         m_nBuffersGrabbed;
};


int Sample::run()
{
    m_nBuffersGrabbed = 0;

    try
    {
        // Open the first camera found, i.e., establish a connection to the camera device. 
        m_Camera.OpenFirstCamera();

        /*  CToFCamera::OpenFirstCamera() is a shortcut for the following sequence:
            CameraList cameras = CToFCamera::EnumerateCameras();
            CameraInfo camInfo = *cameras.begin();
            m_Camera.Open(camInfo);

            If there are multiple cameras connected and you want to open a specific one, use
            the CToFCamera::Open( CameraInfoKey, string ) method.
                
            Example: Open a camera using its IP address
            CToFCamera::Open( IpAddress, "192.168.0.2" );

            Instead of the IP address, any other property of the CameraInfo struct can be used, 
            e.g., the serial number or the user-defined name:

            CToFCamera::Open( SerialNumber, "23167572" );
            CToFCamera::Open( UserDefinedName, "Left" );
        */

        cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;

        // Enable 3D (point cloud) data, intensity data, and confidence data. 
        GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
        GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
        GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");

        // Enable range data.
        ptrComponentSelector->FromString("Range");
        ptrComponentEnable->SetValue(true);
        // Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
        // Note: To change the format of an image component, the Component Selector must first be set to the component
        // you want to configure (see above).
        // To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
        ptrPixelFormat->FromString("Coord3D_ABC32f" );

        ptrComponentSelector->FromString("Intensity");
        ptrComponentEnable->SetValue(true);

        ptrComponentSelector->FromString("Confidence");
        ptrComponentEnable->SetValue(true);

        // Acquire images until the call-back onImageGrabbed indicates to stop acquisition. 
        // 5 buffers are used (round-robin).
        m_Camera.GrabContinuous( 5, 1000, this, &Sample::onImageGrabbed );

        // Clean-up
        m_Camera.Close();
    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        // After successfully opening the camera, the IsConnected method can be used 
        // to check if the device is still connected.
        if ( m_Camera.IsOpen() && ! m_Camera.IsConnected() )
        {
            cerr << "Camera has been removed." << endl;
        }

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool Sample::onImageGrabbed( GrabResult grabResult, BufferParts parts )
{
    if ( grabResult.status == GrabResult::Timeout )
    {
        cerr << "Timeout occurred. Acquisition stopped." << endl;
        // The timeout might be caused by a removal of the camera. Check if the camera
        // is still connected.
        if ( ! m_Camera.IsConnected() )
        {
            cerr << "Camera has been removed." << endl;
        }

        return false; // Indicate to stop acquisition
    }
    m_nBuffersGrabbed++;
    if ( grabResult.status != GrabResult::Ok )
    {
        cerr << "Image " << m_nBuffersGrabbed << "was not grabbed." << endl;
    }
    else
    {
        // Retrieve the values for the center pixel
        const int width = (int) parts[0].width;
        const int height = (int) parts[0].height;
        const int x = (int) (0.5 * width);
        const int y = (int) (0.5 * height);
        CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData + y * width + x;
        uint16_t *pIntensity = (uint16_t*) parts[1].pData + y * width + x;
        uint16_t *pConfidence = (uint16_t*) parts[2].pData + y * width + x;

        cout << "Center pixel of image " << setw(2) << m_nBuffersGrabbed << ": ";
        cout.setf( ios_base::fixed);
        cout.precision(1);
        if ( p3DCoordinate->IsValid() )
            cout << "x=" << setw(6) <<  p3DCoordinate->x << " y=" << setw(6) << p3DCoordinate->y << " z=" << setw(6) << p3DCoordinate->z;
        else
            cout << "x=   n/a y=   n/a z=   n/a";
        cout << " intensity="<< setw(5) << *pIntensity << " confidence=" << setw(5) << *pConfidence << endl;
    }
    return m_nBuffersGrabbed < 10; // Indicate to stop acquisition when 10 buffers are grabbed
}

int main(int argc, char* argv[])
{
    int exitCode = EXIT_SUCCESS;
    
    try
    {
        CToFCamera::InitProducer();

        Sample processing;


        exitCode = processing.run();
    }
    catch ( GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
        exitCode = EXIT_FAILURE;
    }

    // Release the GenTL producer and all of its resources. 
    // Note: Don't call TerminateProducer() until the destructor of the CToFCamera
    // class has been called. The destructor may require resources which may not
    // be available anymore after TerminateProducer() has been called.
    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions

    cout << endl << "Press Enter to exit." << endl;
    while (cin.get() != '\n');

    return exitCode;
}
