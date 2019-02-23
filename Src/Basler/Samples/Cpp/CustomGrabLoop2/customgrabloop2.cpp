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

This sample illustrates how to grab images from a ToF camera and how to access the image and depth data. Different from the
FirstSample sample, user-provided buffers and a user-provided grab loop are used here.

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

/* Allocator class used by the CToFCamera class for allocating memory buffers
used for grabbing. This custom allocator allocates buffers on the C++ heap.
If the application doesn't provide an allocator, a default one is used
that allocates memory on the heap. */
class CustomAllocator : public BufferAllocator
{
public:
	virtual void* AllocateBuffer(size_t size_by) { return new char[size_by]; }
	virtual void FreeBuffer(void* pBuffer) { delete[] static_cast<char*>(pBuffer); }
	virtual void Delete() { delete this; }
};


class Sample
{
public:
	int run();

private:
	void setupCamera();
	void processData(const GrabResult& grabResult);

private:

	CToFCamera  m_Camera;
};

void Sample::setupCamera()
{
	m_Camera.OpenFirstCamera();
	cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;

	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");

	// Enable range data
	ptrComponentSelector->FromString("Range");
	ptrComponentEnable->SetValue(true);
	// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
	// Note: To change the format of an image component, the Component Selector must first be set to the component
	// you want to configure (see above).
	// To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
	ptrPixelFormat->FromString("Coord3D_ABC32f");

	ptrComponentSelector->FromString("Intensity");
	ptrComponentEnable->SetValue(true);

	ptrComponentSelector->FromString("Confidence");
	ptrComponentEnable->SetValue(true);

}

int Sample::run()
{
	const size_t nBuffers = 3;  // Number of buffers to be used for grabbing.
	const size_t nImagesToGrab = 10; // Number of images to grab.
	size_t nImagesGrabbed = 0;

	try
	{
		//
		// Open and parameterize the camera.
		//

		setupCamera();

		//
		// Grab and process images
		//

		// Let the camera class use our allocator. 
		// When the application doesn't provide an allocator, a default one that allocates memory buffers
		// on the heap will be used automatically.
		m_Camera.SetBufferAllocator(new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

																  // Allocate the memory buffers and prepare image acquisition.
		m_Camera.PrepareAcquisition(nBuffers);

		// Enqueue all buffers to be filled with image data.
		for (size_t i = 0; i < nBuffers; ++i)
		{
			m_Camera.QueueBuffer(i);
		}

		// Start the acquisition engine.
		m_Camera.StartAcquisition();

		// Now, the acquisition can be started on the camera.
		m_Camera.IssueAcquisitionStartCommand(); // The camera continuously sends data now.

												 // Enter the grab loop
		do
		{
			GrabResult grabResult;
			// Wait up to 1000 ms for the next grabbed buffer available in the 
			// acquisition engine's output queue.
			m_Camera.GetGrabResult(grabResult, 1000);

			// Check whether a buffer has been grabbed successfully.
			if (grabResult.status == GrabResult::Timeout)
			{
				cerr << "Timeout occurred." << endl;
				// The timeout might be caused by a removal of the camera. Check if the camera
				// is still connected.
				if (!m_Camera.IsConnected())
				{
					cerr << "Camera has been removed." << endl;
				}
				break; // exit loop
			}
			if (grabResult.status != GrabResult::Ok)
			{
				cerr << "Failed to grab image." << endl;
				break; // exit loop
			}
			nImagesGrabbed++;
			// We can process the buffer now. The buffer will not be overwritten with new data until
			// it is explicitly placed in the acquisition engine's input queue again.
			processData(grabResult);

			// We finished processing the data, put the buffer back into the acquisition 
			// engine's input queue to be filled with new image data.
			m_Camera.QueueBuffer(grabResult.hBuffer);

		} while (nImagesGrabbed < nImagesToGrab);

		// Stop the camera
		m_Camera.IssueAcquisitionStopCommand();

		// Stop the acquisition engine and release memory buffers and other resources used for grabbing.
		m_Camera.FinishAcquisition();

		// Close the connection to the camera
		m_Camera.Close();
	}
	catch (const GenICam::GenericException& e)
	{
		cerr << "Exception occurred: " << e.GetDescription() << endl;
		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		if (m_Camera.IsOpen() && !m_Camera.IsConnected())
		{
			cerr << "Camera has been removed." << endl;
		}
		return EXIT_FAILURE;
	}

	return nImagesGrabbed == nImagesToGrab ? EXIT_SUCCESS : EXIT_FAILURE;
}

void Sample::processData(const GrabResult& grabResult)
{
	BufferParts parts;
	m_Camera.GetBufferParts(grabResult, parts);

	// Retrieve the values for the center pixel
	const int width = (int)parts[0].width;
	const int height = (int)parts[0].height;
	const int x = (int)(0.5 * width);
	const int y = (int)(0.5 * height);
	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData + y * width + x;
	uint16_t *pIntensity = (uint16_t*)parts[1].pData + y * width + x;
	uint16_t *pConfidence = (uint16_t*)parts[2].pData + y * width + x;

	cout.setf(ios_base::fixed);
	cout.precision(1);
	if (p3DCoordinate->IsValid())
		cout << "x=" << setw(6) << p3DCoordinate->x << " y=" << setw(6) << p3DCoordinate->y << " z=" << setw(6) << p3DCoordinate->z;
	else
		cout << "x=   n/a y=   n/a z=   n/a";
	cout << " intensity=" << setw(5) << *pIntensity << " confidence=" << setw(5) << *pConfidence << endl;
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
	catch (GenICam::GenericException& e)
	{
		cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
		exitCode = EXIT_FAILURE;
	}

	// Release the GenTL producer and all of its resources. 
	// Note: Don't call TerminateProducer() until the destructor of the CToFCamera
	// class has been called. The destructor may require resources which may not
	// be available anymore after TerminateProducer() has been called.
	if (CToFCamera::IsProducerInitialized())
		CToFCamera::TerminateProducer();  // Won't throw any exceptions

	cout << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}
