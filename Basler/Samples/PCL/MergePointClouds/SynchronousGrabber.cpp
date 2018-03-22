#include "stdafx.h"
#include "SynchronousGrabber.h"
// Utility class for time measurements.
#include "StopWatch.h"

#ifdef CIH_LINUX_BUILD        // Make sure proper headers are used when compiling under Linux ...
#include <unistd.h>
#endif
#ifdef CIH_WIN_BUILD          // ... or under Windows.
#include <windows.h>
#endif

#include <iostream>

using namespace std;
using namespace GenTLConsumerImplHelper;
using namespace GenApi;


namespace
{
    // Sleep function for Windows and Linux
    void mSleep(int sleepMs)
    {
#ifdef CIH_LINUX_BUILD
        usleep(sleepMs * 1000);   // usleep takes sleep time in us
#endif
#ifdef CIH_WIN_BUILD
        Sleep(sleepMs);           // Sleep expects time in ms
#endif
    }
}

SynchronousGrabber::SynchronousGrabber(size_t nCameras, unsigned int DepthMin, unsigned int DepthMax)
    : m_nCams(nCameras)
    , m_DepthMin(DepthMin)
    , m_DepthMax(DepthMax)
    , m_IsMaster(nCameras)
{
}

void SynchronousGrabber::setupCameras()
{
    cout << "Searching for cameras ... ";

    CameraList cameras = CToFCamera::EnumerateCameras();
    cout << "found " << cameras.size() << " ToF cameras " << endl << endl;

    if (m_nCams != cameras.size())
    {
        std::stringstream errmsg;
        errmsg << "The number of cameras found (" << cameras.size() << ") is not equal to the number of expected cameras (" << m_nCams << ").";
        throw std::runtime_error(errmsg.str());
    }

    int64_t camIdx = 0;

    // Initialize array with master/slave info.
    for (size_t i = 0; i < m_nCams; i++)
    {
        m_IsMaster[i] = false;
    }


    // Iterate over list of cameras.
    for (auto &cInfo : cameras)
    {
        cout << "Configuring Camera " << camIdx << " : " << cInfo.strDisplayName << "." << endl;

        // Create shared pointer to ToF camera.
        shared_ptr<CToFCamera> cam(new CToFCamera());

        // Store shared pointer for later use.
        m_Cameras.push_back(cam);

        // Open camera with camera info.
        cam->Open(cInfo);

        //
        // Configure camera for synchronous free run.
        // Do not yet configure trigger delays.
        //

        // Enable IEEE1588.
        CBooleanPtr ptrIEEE1588Enable = cam->GetParameter("GevIEEE1588");
        ptrIEEE1588Enable->SetValue(true);

        // Enable trigger.
        GenApi::CEnumerationPtr ptrTriggerMode = cam->GetParameter("TriggerMode");

        // Set trigger mode to "on".
        ptrTriggerMode->FromString("On");

        // Configure the sync timer as trigger source.
        GenApi::CEnumerationPtr ptrTriggerSource = cam->GetParameter("TriggerSource");
        ptrTriggerSource->FromString("SyncTimer");

        // Configure the cameras to send 3D coordinates and intensity values. We will use
        // the intensity values as colors for the points in the point cloud.
        CEnumerationPtr ptrComponentSelector = cam->GetParameter("ComponentSelector");
        CBooleanPtr ptrComponentEnable = cam->GetParameter("ComponentEnable");
        CEnumerationPtr ptrPixelFormat = cam->GetParameter("PixelFormat");

        ptrComponentSelector->FromString("Range");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Coord3D_ABC32f");

        ptrComponentSelector->FromString("Intensity");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Mono16");

        // Set the desired depth range.
        CIntegerPtr ptrDepthMin = cam->GetParameter("DepthMin");
        ptrDepthMin->SetValue(m_DepthMin);
        CIntegerPtr ptrDepthMax = cam->GetParameter("DepthMax");
        ptrDepthMax->SetValue(m_DepthMax);

        // Allocate the memory buffers and prepare image acquisition.
        cam->PrepareAcquisition(c_nBuffers);

        // Enqueue all buffers to be filled with image data.
        for (size_t i = 0; i < c_nBuffers; ++i)
        {
            cam->QueueBuffer(i);
        }

        // Start acquisition engine.
        cam->StartAcquisition();

        // Proceed to next camera.
        camIdx++;
    }
}

void SynchronousGrabber::findMaster()
{
    unsigned int nMaster;

    cout << endl << "Waiting for cameras to negotiate master role ..." << endl << endl;

    do
    {
        // Number of devices that currently claim to be a master clock.
        nMaster = 0;
        //
        // Wait until a master camera (if any) and the slave cameras have been chosen.
        // Note that if a PTP master clock is present in the subnet, all ToF cameras
        // ultimately assume the slave role.
        //
        for (size_t i = 0; i < m_nCams; i++)
        {
            // Latch IEEE1588 status.
            CCommandPtr ptrGevIEEE1588DataSetLatch = m_Cameras.at(i)->GetParameter("GevIEEE1588DataSetLatch");
            ptrGevIEEE1588DataSetLatch->Execute();

            // Read out latched status.
            GenApi::CEnumerationPtr ptrGevIEEE1588StatusLatched = m_Cameras.at(i)->GetParameter("GevIEEE1588StatusLatched");
            // The smart pointer holds the node, not the value.
            // The node value is always up to date. 
            // Therefore, there is no need to repeatedly retrieve the pointer here.
            while (ptrGevIEEE1588StatusLatched->ToString() == "Listening")
            {
                // Latch GevIEEE1588 status.
                ptrGevIEEE1588DataSetLatch->Execute();
                cout << "." << std::flush;
                mSleep(1000);
            }

            //
            // Store the information whether the camera is master or slave.
            //
            if (ptrGevIEEE1588StatusLatched->ToString() == "Master")
            {
                m_IsMaster[i] = true;
                nMaster++;
            }
            else
            {
                m_IsMaster[i] = false;
            }
        }
    } while (nMaster > 1);    // Repeat until there is at most one master left.

    // Use this variable to check whether there is an external master clock.
    bool externalMasterClock = true;

    for (size_t camIdx = 0; camIdx < m_nCams; camIdx++)
    {
        if (true == m_IsMaster[camIdx])
        {
            cout << "   camera " << camIdx << " is master" << endl << endl;
            externalMasterClock = false;
        }
    }

    if (true == externalMasterClock)
    {
        cout << "External master clock present in subnet: All cameras are slaves." << endl << endl;
    }
}

void SynchronousGrabber::syncCameras()
{
    // Maximum allowed offset from master clock. 
    const uint64_t tsOffsetMax = 10000;

    cout << "Wait until offsets from master clock have settled below " << tsOffsetMax << " ns " << endl << endl;

    for (size_t camIdx = 0; camIdx < m_nCams; camIdx++)
    {
        // Check all slaves for deviations from master clock.
        if (false == m_IsMaster[camIdx])
        {
            uint64_t tsOffset;
            do
            {
                tsOffset = GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(camIdx, 1.0, 0.1);
                cout << "max offset of cam " << camIdx << " = " << tsOffset << " ns" << endl;
            } while (tsOffset >= tsOffsetMax);
        }
    }
}

/*
\param camIndex         Index in smart pointer array of camera which is in Slave state.
\param timeToMeasureSec Amount of time in seconds for computing the maximum absolute offset from master.
\param timeDeltaSec     Amount of time in seconds between latching the offset from master.
\return                 Maximum absolute offset from master in nanoseconds.
*/
int64_t SynchronousGrabber::GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(int camIdx, double timeToMeasureSec, double timeDeltaSec)
{
    CCommandPtr ptrGevIEEE1588DataSetLatch = m_Cameras.at(camIdx)->GetParameter("GevIEEE1588DataSetLatch");
    ptrGevIEEE1588DataSetLatch->Execute();
    CIntegerPtr ptrGevIEEE1588OffsetFromMaster = m_Cameras.at(camIdx)->GetParameter("GevIEEE1588OffsetFromMaster");
    StopWatch m_StopWatch;
    m_StopWatch.reset();
    // Maximum of offsets from master
    int64_t maxOffset = 0;
    // Number of samples
    uint32_t n(0);
    // Current time
    double currTime(0);
    do
    {
        // Update the current time
        currTime = m_StopWatch.get(false);
        if (currTime >= n * timeDeltaSec)
        {
            // Time for next sample has elapsed.
            // Latch IEEE1588 data set to get offset from master.
            ptrGevIEEE1588DataSetLatch->Execute();
            // Maximum of offsets from master.
            maxOffset = max(maxOffset, std::abs(ptrGevIEEE1588OffsetFromMaster->GetValue()));
            // Increase number of samples.
            n++;
        }
        mSleep(1);
    } while (currTime <= timeToMeasureSec);
    // Return maximum of offsets from master for given time interval.
    return maxOffset;
}

void SynchronousGrabber::setTriggerDelays()
{
    // Current timestamp
    uint64_t timestamp, syncStartTimestamp;

    // The low and high part of the timestamp
    uint64_t tsLow, tsHigh;

    // Initialize trigger delay.
    m_TriggerDelay = 0;

    cout << endl << "configuring start time and trigger delays ..." << endl << endl;

    //
    // Cycle through cameras and set trigger delay.
    //
    for (size_t camIdx = 0; camIdx < m_Cameras.size(); camIdx++)
    {

        cout << "Camera " << camIdx << " : " << endl;

        //
        // Read timestamp and exposure time.
        // Calculation of synchronous free run timestamps will all be based 
        // on timestamp and exposure time(s) of first camera.
        //
        if (camIdx == 0)
        {
            // Latch timestamp registers.
            CCommandPtr ptrTimestampLatch = m_Cameras.at(camIdx)->GetParameter("TimestampLatch");
            ptrTimestampLatch->Execute();

            // Read the two 32-bit halves of the 64-bit timestamp. 
            CIntegerPtr ptrTimeStampLow(m_Cameras.at(camIdx)->GetParameter("TimestampLow"));
            CIntegerPtr ptrTimeStampHigh(m_Cameras.at(camIdx)->GetParameter("TimestampHigh"));
            tsLow = ptrTimeStampLow->GetValue();
            tsHigh = ptrTimeStampHigh->GetValue();

            // Assemble 64-bit timestamp and keep it.
            timestamp = tsLow + (tsHigh << 32);
            cout << "Reading time stamp from first camera. \ntimestamp = " << timestamp << endl << endl;

            cout << "Reading exposure times from first camera:" << endl;

            // Get exposure time count (in case of HDR there will be 2, otherwise 1).
            CIntegerPtr ptrExposureTimeSelector = m_Cameras.at(camIdx)->GetParameter("ExposureTimeSelector");
            size_t n_expTimes = 1 + (size_t)ptrExposureTimeSelector->GetMax();

            // Sum up exposure times.
            CFloatPtr ptrExposureTime = m_Cameras.at(camIdx)->GetParameter("ExposureTime");
            for (size_t l = 0; l < n_expTimes; l++)
            {
                ptrExposureTimeSelector->SetValue(l);
                cout << "exposure time " << l << " = " << ptrExposureTime->GetValue() << endl << endl;
                m_TriggerDelay += (int64_t)(1000 * ptrExposureTime->GetValue());   // Convert from us -> ns
            }

            cout << "Calculating trigger delay." << endl;

            // Add readout time.
            m_TriggerDelay += (n_expTimes - 1) * c_ReadoutTime;

            // Add safety margin for clock jitter.
            m_TriggerDelay += 1000000;

            // Calculate synchronous trigger rate.
            cout << "Calculating maximum synchronous trigger rate ... " << endl;
            m_SyncTriggerRate = 1000000000 / (m_nCams * m_TriggerDelay);

            // If the calculated value is greater than the maximum supported rate, 
            // adjust it. 
            CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate"));
            if (m_SyncTriggerRate > ptrSyncRate->GetMax())
            {
                m_SyncTriggerRate = (uint64_t)ptrSyncRate->GetMax();
            }

            // Print trigger delay and synchronous trigger rate.
            cout << "Trigger delay = " << m_TriggerDelay / 1000000 << " ms" << endl;
            cout << "Setting synchronous trigger rate to " << m_SyncTriggerRate << " fps" << endl << endl;
        }

        // Set synchronization rate.
        CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate"));
        ptrSyncRate->SetValue((double)m_SyncTriggerRate);

        // Calculate new timestamp by adding trigger delay.
        // First camera starts after triggerBaseDelay, nth camera is triggered 
        // after a delay of triggerBaseDelay +  n * triggerDelay.
        syncStartTimestamp = timestamp + m_TriggerBaseDelay + camIdx * m_TriggerDelay;

        // Disassemble 64-bit timestamp.
        tsHigh = syncStartTimestamp >> 32;
        tsLow = syncStartTimestamp - (tsHigh << 32);

        // Get pointers to the two 32-bit registers, which together hold the 64-bit 
        // synchronous free run start time.
        CIntegerPtr ptrSyncStartLow(m_Cameras.at(camIdx)->GetParameter("SyncStartLow"));
        CIntegerPtr ptrSyncStartHigh(m_Cameras.at(camIdx)->GetParameter("SyncStartHigh"));

        ptrSyncStartLow->SetValue(tsLow);
        ptrSyncStartHigh->SetValue(tsHigh);

        // Latch synchronization start time & synchronization rate registers.
        // Until the values have been latched, they won't have any effect.
        CCommandPtr ptrSyncUpdate = m_Cameras.at(camIdx)->GetParameter("SyncUpdate");
        ptrSyncUpdate->Execute();

        // Show new synchronous free run start time.
        cout << "Setting Sync Start time stamp" << endl;
        cout << "SyncStartLow = " << ptrSyncStartLow->GetValue() << endl;
        cout << "SyncStartHigh = " << ptrSyncStartHigh->GetValue() << endl << endl;
    }
}

/*
Grab the next image from every camera and store the buffer parts in the output vector.
*/
int SynchronousGrabber::getNextImages(vector<shared_ptr<BufferParts> >& parts)
{
    // Clear the output vector and reserve the required memory.
    parts.clear();
    parts.resize(m_nCams);

    // Grab one image from every camera.
    for (int i = 0; i < m_nCams; i++)
    {
        GrabResult grabResult;
        shared_ptr<BufferParts> ptrParts(new BufferParts);

        m_Cameras.at(i)->GetGrabResult(grabResult, 100);
        if (grabResult.status == GrabResult::Ok) {
            // Read the buffer parts and store them in the output vector.
            m_Cameras.at(i)->GetBufferParts(grabResult, *ptrParts);
            parts.at(i) = ptrParts;
        }
        else {
            cout << "There was an invalid grab result." << endl;
            return EXIT_FAILURE;
        }

        // We finished processing the data and put the buffer back into the acquisition
        // engine's input queue to be filled with new image data.
        m_Cameras.at(i)->QueueBuffer(grabResult.hBuffer);
    }

    return EXIT_SUCCESS;
}

/*
Set up the ToF cameras and synchronize them.
*/
int SynchronousGrabber::setupAndStart()
{
    try
    {
        // Find all ToF cameras, open and parameterize them.
        setupCameras();

        // Let cameras negotiate which one should be the master.
        findMaster();

        // Wait until all camera clocks have been synchronized.
        syncCameras();

        // Set trigger delay for each camera.
        setTriggerDelays();

        // Start continuous image acquisition.
        for (int i = 0; i < m_nCams; i++) {
            // The camera continuously sends data now.
            m_Cameras.at(i)->IssueAcquisitionStartCommand();
        }
    }
    catch (const std::exception& e)
    {
        cerr << "Exception occurred: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch (const GenICam::GenericException& e)
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
