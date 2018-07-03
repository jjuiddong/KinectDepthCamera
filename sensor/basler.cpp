
#include "stdafx.h"
#include "basler.h"
//#include "3dview.h"
//#include "depthframe.h"


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



cBaslerCamera::cBaslerCamera(const bool isThreadMode //= false
)
	: m_Camera(NULL)
	, m_isSetupSuccess(false)
{
}

cBaslerCamera::~cBaslerCamera()
{
	Clear();
}


bool cBaslerCamera::Init()
{
	m_isSetupSuccess = false;

	try
	{
		CToFCamera::InitProducer();
		m_Camera = new CToFCamera();
		if (EXIT_SUCCESS == BaslerCameraSetup())
		{
			//m_baslerSetupSuccess = true;
		}
	}
	catch (GenICam::GenericException& e)
	{
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();
		::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);
		//cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
		//exitCode = EXIT_FAILURE;
		return false;
	}

	m_isSetupSuccess = true;
	return true;
}


int cBaslerCamera::BaslerCameraSetup()
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
		m_Camera->SetBufferAllocator(new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

																   // Allocate the memory buffers and prepare image acquisition.
		m_Camera->PrepareAcquisition(nBuffers);

		// Enqueue all buffers to be filled with image data.
		for (size_t i = 0; i < nBuffers; ++i)
		{
			m_Camera->QueueBuffer(i);
		}

		// Start the acquisition engine.
		m_Camera->StartAcquisition();

		// Now, the acquisition can be started on the camera.
		m_Camera->IssueAcquisitionStartCommand(); // The camera continuously sends data now.

	}
	catch (const GenICam::GenericException& e)
	{
		//cerr << "Exception occurred: " << e.GetDescription() << endl;
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();

		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		if (m_Camera->IsOpen() && !m_Camera->IsConnected())
		{
			//cerr << "Camera has been removed." << endl;
			msg += "Camera has been removed.";
		}

		::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



void cBaslerCamera::setupCamera()
{
	m_Camera->OpenFirstCamera();
	//cout << "Connected to camera " << m_Camera->GetCameraInfo().strDisplayName << endl;

	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_Camera->GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_Camera->GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_Camera->GetParameter("PixelFormat");

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



bool cBaslerCamera::Capture(graphic::cRenderer &renderer)
{
	RETV(!m_isSetupSuccess, false);

	try
	{
		GrabResult grabResult;
		// Wait up to 1000 ms for the next grabbed buffer available in the 
		// acquisition engine's output queue.
		m_Camera->GetGrabResult(grabResult, 100);

		// Check whether a buffer has been grabbed successfully.
		if (grabResult.status == GrabResult::Timeout)
		{
			//cerr << "Timeout occurred." << endl;
			// The timeout might be caused by a removal of the camera. Check if the camera
			// is still connected.
			if (!m_Camera->IsConnected())
			{
				//cerr << "Camera has been removed." << endl;
			}
			//break; // exit loop
			return false;
		}
		if (grabResult.status != GrabResult::Ok)
		{
			//cerr << "Failed to grab image." << endl;
			//break; // exit loop
			return false;
		}

		//nImagesGrabbed++;
		// We can process the buffer now. The buffer will not be overwritten with new data until
		// it is explicitly placed in the acquisition engine's input queue again.
		processData(grabResult, renderer);

		// We finished processing the data, put the buffer back into the acquisition 
		// engine's input queue to be filled with new image data.
		m_Camera->QueueBuffer(grabResult.hBuffer);
	}
	catch (const GenICam::GenericException& e)
	{
		//cerr << "Exception occurred: " << e.GetDescription() << endl;
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();

		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		if (m_Camera->IsOpen() && !m_Camera->IsConnected())
		{
			//cerr << "Camera has been removed." << endl;
			msg += "Camera has been removed.";
		}

		::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);
		return false;
	}

	return true;
}


void cBaslerCamera::processData(const GrabResult& grabResult, graphic::cRenderer &renderer)
{
	BufferParts parts;
	m_Camera->GetBufferParts(grabResult, parts);

	// Retrieve the values for the center pixel
	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
	uint16_t *pIntensity = (uint16_t*)parts[1].pData;
	uint16_t *pConfidence = (uint16_t*)parts[2].pData;
	const size_t nPixel = parts[0].width * parts[0].height;

	cDatReader reader;
	memcpy(&reader.m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);
	memcpy(&reader.m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
	memcpy(&reader.m_confidence[0], pConfidence, sizeof(unsigned short) * 640 * 480);

	//g_root.m_sensorBuff[0].ReadDatFile(renderer, reader);

	// save *.pcd file
	if (g_root.m_isAutoSaveCapture)
	{
		using namespace std;
		string fileName = string("../media/depth/") + common::GetCurrentDateTime() + ".pcd";
		ofstream o(fileName, ios::binary);
		if (o)
		{
			o.write((char*)p3DCoordinate, sizeof(float)*nPixel * 3);
			o.write((char*)pIntensity, sizeof(uint16_t)*nPixel);
			o.write((char*)pConfidence, sizeof(uint16_t)*nPixel);
		}
	}
}


void cBaslerCamera::Clear()
{
	if (m_isSetupSuccess && m_Camera)
	{
		// Stop the camera
		m_Camera->IssueAcquisitionStopCommand();
		// Stop the acquisition engine and release memory buffers and other resources used for grabbing.
		m_Camera->FinishAcquisition();
		// Close the connection to the camera
		m_Camera->Close();
	}

	if (m_Camera)
	{
		SAFE_DELETE(m_Camera);

		if (CToFCamera::IsProducerInitialized())
			CToFCamera::TerminateProducer();  // Won't throw any exceptions
	}
}
