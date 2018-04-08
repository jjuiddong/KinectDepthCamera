/*

Synchronous Free Run Sample
===========================

This sample illustrates how to configure two or more ToF cameras to synchronously 
acquire images utilizing the synchronous free run feature. 

Synchronous Free Run
--------------------
A camera may be operated in triggered mode or free running. Triggered means that the start of each 
image exposure is triggered either by an internally or an externally generated signal.
Free running, on the other hand, means, that the sensor of the camera acquires images at 
its own internal speed.
In synchronous free run, image exposure is triggered by an internal timer. The timer operates 
based on the intrinsic precision clock of the camera.
The clocks of multiple cameras are synchronized by the precision time protocol (PTP).

(In that sense, the notion of 'free run' is slightly misleading. It rather refers to 
the fact that the trigger signals of the cameras are not generated by a common external trigger 
source but internally by the cameras themselves. In that sense, in PTP mode, each single camera
may be considered free-running.) 

PTP
---
PTP is a method to synchronize clocks of devices that are connected via Ethernet. 
It follows a simplified description of the PTP protocol.
(For details, please refer i.e. to [1] or [2] below.)
Amongst devices synchronizing via PTP, one of the devices takes the role of the master clock, 
while the others are slaves.
So prior to synchronisation, the devices need to negotiate the master role. This is done by 
means of the Best Master Clock algorithm.
After the master device has been determined, it (resp. the clock instance within the master) 
begins to send synchronization messages to the slaves so they can synchronize their clocks. 
In return, the slaves are sending delay request messages, which the master responds to with 
delay respond messages. From this, the slaves are able to calculate the network delay which has 
to be added to the timestamps from the master. 
Since network delay for example is not constant and not equal in both directions, the PTP algorithm 
can only estimate the correct time by means of a step-by-step approximation. 
Thus, clock synchronization can't be achieved immediately, but improves over time. 

Using Synchronous Free Run with Time of Flight cameras
------------------------------------------------------
Using PTP, camera clocks can be synchronized with an accuracy of 1 µs or even better.
It is therefore possible to operate cameras in two ways:
    a) cameras acquire images exactly at the same point of time
    b) cameras acquire images synchronously but with an intentional delay added between exposures
Scenario a) is useful when operating normal "2D" cameras. When ToF cameras are running exactly in 
sync, however, they can disturb each other's measurement accuracy because of the light each 
camera sends out. Therefore, scenario b) is preferable. Here, cameras can acquire images using a 
round-robin approach:

Synchronous Free Run in Standard (No HDR) Mode
-----------------------------------------------
Let's assume that 4 ToF cameras have been set up to operate synchronously.
Processing mode has been set to 'Standard' (no HDR) and exposure time is set to 20 ms.  
Camera 1 starts exposure. After 20 ms it finishes exposure (and also disables its infrared LEDs).
Camera 1 starts readout, which will take another 21 ms. However, since the LEDs of camera 1 are already 
disabled, we can set camera 2 to start exposure after a delay of 21 ms. Camera 3 starts after 42 ms, 
camera 4 after 63 ms. After 84 ms camera 4 has finished exposure. Note that we have an additional 1 ms 
in order to avoid disturbance between cameras, even taking into account clock jitter or other
inaccuracies in the trigger timing.
The trigger rate of all cameras can safely be set to 10 fps, which results in a break of 20 ms, before, 
after 100 ms, camera 1 once again starts exposure. 
In this way, the four cameras can be operated synchronously while at the same time avoiding mutual
disturbance by delaying exposures.

Synchronous Free Run in HDR Mode
--------------------------------
HDR is achieved by means of two separate exposures. Currently, the default values of the Basler ToF 
Camera are 4 ms for the first exposure time and 20 ms for the second one. 
After first and second exposure there are readout times of 21 ms each. 
Lets again assume that 4 cameras are to be set up in synchronous free run mode.
Camera 1 finishes the first exposure after 4 ms. Then follows the first readout time, which is 21 ms.
The second exposure starts after 25 ms and finishes after 45 ms. 
Since now camera 1 has disabled its LEDs, camera 2 can already start exposure after 46 ms.
(1 ms safety margin added.)
Thus, the start time for camera n can be calculated as follows:

    Tn = (n - 1) x (Texp1 + Texp2 + Treadout)

The maximum synchronous trigger rate can then be calculated as follows:

    Fmax = 1 / (n x (Texp1 + Texp2 + Treadout))

Having 4 cameras, the maximum synchronous trigger rate would be:

    Fmax = 1 / (4 x (4 ms + 20 ms + 21 ms) = 1 / 180 ms = 5.6 Hz

Therefore, the synchronous trigger rate can safely be set to 5 Hz for 4 cameras in HDR mode.


Implemented Functionality by Method
-----------------------------------
For clarity, functionality of the sample code has been divided into five methods:

Method              Function
-------------------------------------------------------------------------------------------------
setupCameras()      Find all ToF cameras, open and parametrize them.
findMaster()        Let cameras negotiate which one should be the master.
syncCameras()       Wait until all camera clocks have been synchronized.
setTriggerDelays()  Set trigger delay and sychronous free run trigger rate for each camera.
grabImages()        Grab a pre-defined number of images.
processData()       Print image IDs and timestamps. Called from grab loop inside of grabImages().
getStatistics()     Print statistic data after image acquisition has finished. 

See method headers for details.

Usage
-----
Connect two to four Basler ToF cameras to your PC (via switch or router). Optionally, connect
PTP clock (to same subnet).
Start sample application.
While the application works through the five methods, it will print the following information to the console:

    - number of cameras and serial numbers
    - which camera has master role
    - repeatedly the clock offsets from master clock
    - start time and trigger delays for all cameras
    - frame IDs and timestamp for every 10th image
    - number of timeout errors during image retrieval (if any)
    - statistic data (after finishing image acquisition)

Trigger delay and synchronous free run trigger rate will be calculated and set to proper values 
prior to image acquisition.

[1]        http://www.ni.com/newsletter/50130/en/
[2]        https://www.eecis.udel.edu/~mills/ptp.html


*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>           // cout ..
#include <algorithm>          // std::max ..
#include <iomanip> 
#include "StopWatch.h"


#ifdef CIH_LINUX_BUILD        // make sure proper headers are used when compiling under Linux ... 
#include <unistd.h>
#endif
#ifdef CIH_WIN_BUILD          // ... or under win
#include <windows.h>
#endif                        // since linux / win implementations of sleep functions differ 
                            


void mSleep(int sleepMs)      // Use a wrapper  
{
#ifdef CIH_LINUX_BUILD
    usleep(sleepMs * 1000);   // usleep takes sleep time in us
#endif
#ifdef CIH_WIN_BUILD
    Sleep(sleepMs);           // Sleep expects time in ms
#endif
}

#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif

using namespace GenTLConsumerImplHelper;
using namespace GenApi;
using namespace std;

// Maximum number of ToF cameras that can be handled.
const static int MAX_CAMS = 10;

/**
 * \brief Class that allocates memory buffers.
 *
 * Allocator class used by the CToFCamera class for allocating memory buffers
 * used for grabbing. This custom allocator allocates buffers on the C++ heap.
 * If the application doesn't provide an allocator, a default one is used
 * that allocates memory on the heap. 
*/
class CustomAllocator : public BufferAllocator
{
public:
    virtual void* AllocateBuffer(size_t size_by ) { return new char[size_by]; } 
    virtual void FreeBuffer( void* pBuffer ) { delete[] static_cast<char*>(pBuffer); }
    virtual void Delete() { delete this; }
};


class Sample
{
public:
    int run();

private:

   /**
    * \brief Find all ToF cameras, open and parametrize them.
    *
    * The method starts searching for ToF cameras. 
    * For each camera found, an instance of CTofCamera is created and a shared pointer to 
    * the camera is stored in the m_Cameras array.
    * Various parameters are configured via the camera's GenICam interface.
    */
    void setupCameras();

   /**
    * \brief Let cameras negotiate which one should be the master.
    *
    * The value of the GevIEEE1588Status GenICam node is checked for each camera.
    * The algorithm waits until the value changes from "listening" to either "master" or "slave".
    * After it has been decided which camera is master (if any) and which cameras are slaves, 
    * the master role is stored in the m_master[] array for use in the following methods. 
    *
    * Note that if a PTP master clock is present in the subnet, all ToF cameras
    * should ultimately assume the slave role.
    */
    void findMaster();

   /**
    * 
    * \brief Make sure that all slave clocks are in sync with the master clock.
    * 
    * For each camera with slave role: Check how much the slave clocks deviate from the master clock.
    * Wait until deviation is lower than a preset threshold.
    *
    */
    void syncCameras();

   /**
    * \brief Set trigger delay for each camera.
    *
    * A trigger delay is set that is equal to or longer than the exposure time of the camera. 
    * A timestamp is read from the first camera.
    * Calculation of synchronous free run timestamps is based on this timestamp.
    * Calculate synchronous free run timestamp by adding trigger delay.
    * First camera starts after triggerBaseDelay, nth camera is triggered after a delay of 
    * triggerBaseDelay +  n * ( triggerDelay + safety margin).
    * For an explanation of the calculation of trigger delays and synchronous free run trigger rate, 
    * please have a look at the documentation block at the top of this file!
    */
    void setTriggerDelays();

   /**
    * \brief Grab one image from each camera round robin and call processData()  
    */
    void grabImages();

   /**
    * \brief Read out statistic data from one camera.
    *
    * The statistic data is acquired after image acqusisition has terminated.
    * It is useful to check whether fault conditions like timeouts, buffer 
    * underruns or packet delivery problems, etc. have occurred. 
    */
    void getStatistics(size_t camIdx); 

   /**
    * \brief Process the data retrieved from one camera in the grab loop.
    *
    * In this example, frame IDs and timestamps are printed for every 10th image.
    * In an actual application scenario, this is the place to do further image processing
    * or to display images.
    */
    void processData( size_t camIdx, const GrabResult& grabResult );


    /*
    \param camIndex         Index in smart pointer array of camera which is in Slave state.
    \param timeToMeasureSec Amount of time in seconds for computing the maximum absolute offset from master.
    \param timeDeltaSec     Amount of time in seconds between latching the offset from master.
    \return                 Maximum absolute offset from master in nanoseconds.
    */
    int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(size_t camIdx, double timeToMeasureSec, double timeDeltaSec);

private:

    
   /** 
    * Vector holding shared pointers to the ToF cameras
    *
    * Note: Using an stl shared pointer 
    * If the compiler does not support this version of stl, boost shared pointers may be used instead.
    */
    vector<shared_ptr<CToFCamera> > m_Cameras;    

    // List of all ToF cameras available in subnet
    CameraList m_CameraList;

    // Number of cameras, determined from size of camera list at runtime
    size_t m_NumCams;    

    // Trigger rate in synchronous free run mode [fps] (Will be calculated in method setTriggerDelays().)
    uint64_t m_SyncTriggerRate;

    // Trigger delay [ns] gets added to synchronous free run start time  (Will be calculated in method setTriggerDelays.)
    int64_t m_TriggerDelay;

    // Readout time. [ns]
    // Though basically a constant inherent to the ToF camera, the exact value may still change in future firmware releases.
    static const int64_t m_ReadoutTime = 21000000;

    // Constant base delay to be added for all cameras [ns]
    static const int64_t m_TriggerBaseDelay = 250000000;    // 250 ms

    // Information whether a camera is master is stored here
    bool m_IsMaster[MAX_CAMS];

};

void Sample::setupCameras()
{
    cout << "Searching for cameras ... " << endl << endl;

    m_CameraList = CToFCamera::EnumerateCameras();

    cout << "found " << m_CameraList.size() << " ToF cameras " << endl << endl;

    // Store number of cameras.
    m_NumCams = (int)m_CameraList.size();
    size_t camIdx = 0;

    // Initialize array with master/slave info.
    for (size_t i = 0; i < MAX_CAMS; i++)
    {
        m_IsMaster[i] = false;
    }
    
    CameraList::const_iterator iterator;

    // Iterate over list of cameras.
    for (iterator = m_CameraList.begin(); iterator != m_CameraList.end(); ++iterator) 
	{
        CameraInfo cInfo = *iterator;
        cout << "Configuring Camera " << camIdx << " : "<< cInfo.strDisplayName << "." << endl;

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

        // Proceed to next camera.
        camIdx++;
    }

}


void Sample::findMaster()
{

    // Number of masters found ( != 1 ) 
    unsigned int nMaster;

    cout << endl << "waiting for cameras to negotiate master role ..." << endl << endl;

    do
    {

        nMaster = 0;
        //
        // Wait until a master camera (if any) and the slave cameras have been chosen.
        // Note that if a PTP master clock is present in the subnet, all TOF cameras
        // ultimately assume the slave role.
        //
        for (size_t i = 0; i < m_NumCams; i++)
        {
            // Latch IEEE1588 status.
            CCommandPtr ptrGevIEEE1588DataSetLatch = m_Cameras.at(i)->GetParameter("GevIEEE1588DataSetLatch");
            ptrGevIEEE1588DataSetLatch->Execute();

            // Read back latched status.
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

    for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
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


void Sample::syncCameras() 
{

    // Maximum allowed offset from master clock. 
    const uint64_t tsOffsetMax = 10000;

    cout << "Wait until offsets from master clock have settled below " << tsOffsetMax << " ns " << endl << endl;

    for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
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


int64_t Sample::GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(size_t camIdx, double timeToMeasureSec, double timeDeltaSec)
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
        // Update current time.
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


void Sample::setTriggerDelays() {

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
            size_t n_expTimes = 1 + (size_t) ptrExposureTimeSelector->GetMax();
        
            // Sum up exposure times.
            CFloatPtr ptrExposureTime = m_Cameras.at(camIdx)->GetParameter("ExposureTime");
            for (size_t l = 0; l < n_expTimes; l++)
            {   
                ptrExposureTimeSelector->SetValue(l);
                cout << "exposure time " << l << " = " << ptrExposureTime->GetValue() << endl << endl;
                m_TriggerDelay += (int64_t) (1000 * ptrExposureTime->GetValue());   // Convert from us -> ns
            }

            cout << "Calculating trigger delay." << endl;  

            // Add readout time.
            m_TriggerDelay += (n_expTimes -1) * m_ReadoutTime;

            // Add safety margin for clock jitter.
            m_TriggerDelay += 1000000;

            // Calculate synchronous trigger rate.
            cout << "Calculating maximum synchronous trigger rate ... " << endl;
            m_SyncTriggerRate = 1000000000/(m_NumCams * m_TriggerDelay);

            // If the calculated value is greater than the maximum supported rate, 
            // adjust it. 
            CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate")); 
            if ( m_SyncTriggerRate > ptrSyncRate->GetMax() )
            {
                m_SyncTriggerRate = (uint64_t) ptrSyncRate->GetMax();
            }

            // Print trigger delay and synchronous trigger rate.
            cout << "Trigger delay = " << m_TriggerDelay/1000000 << " ms" << endl;
            cout << "Setting synchronous trigger rate to " << m_SyncTriggerRate << " fps"<< endl << endl;
        }

        // Set synchronization rate.
        CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate")); 
        ptrSyncRate->SetValue((double) m_SyncTriggerRate);

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


void Sample::grabImages ()
{
    const size_t nBuffers = 3;                // Number of buffers to be used for grabbing.
    size_t nImagesToGrab = 100 * m_NumCams;    // Grab 10 images per camera.
    size_t nImagesGrabbed = 0;

    try
    {
        // Prepare cameras and buffers for image exposure.
        for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
        {
            // Let the camera class use our allocator. 
            // When the application doesn't provide an allocator, a default one that allocates memory buffers
            // on the heap will be used automatically.
            m_Cameras.at(camIdx)->SetBufferAllocator( new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

            // Allocate the memory buffers and prepare image exposure.
            m_Cameras.at(camIdx)->PrepareAcquisition( nBuffers );

            // Enqueue all buffers to be filled with image data.
            for ( size_t j = 0; j < nBuffers; ++j )
            {
               m_Cameras.at(camIdx)->QueueBuffer( j );
            }

            // Start the acquisition engine.
            m_Cameras.at(camIdx)->StartAcquisition();

            // Now, the acquisition can be started on the camera.
            m_Cameras.at(camIdx)->IssueAcquisitionStartCommand(); // The camera continuously sends data now.
        }

        bool TimoutOccurred = false;    // As long as a valid buffer is returned, i.e., no timeout has occurred, the grab loop can continue.

        // Enter the grab loop.
        do 
        {
            for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
            {
                GrabResult grabResult;
                // Wait up to 1000 ms for the next grabbed buffer available in the 
                // acquisition engine's output queue.
                m_Cameras.at(camIdx)->GetGrabResult( grabResult, 1000 );

                // Check whether a buffer has been grabbed successfully.
                if ( grabResult.status == GrabResult::Timeout )
                {
                    cerr << "Timeout occurred." << endl;
                    TimoutOccurred = true;
                    // In case of a timeout, no buffer has been grabbed, i.e., the grab result doesn't hold a valid buffer.
                    // Do not try to access the buffer in case of a timeout.

                    // The timeout might be caused by a removal of the camera. Check if the camera
                    // is still connected.
                    if ( ! m_Cameras.at(camIdx)->IsConnected() )
                    {
                        cerr << "Camera has been removed." << endl;
                    }

                }
                else
                {
                    nImagesGrabbed++;
                }

                if ( grabResult.status == GrabResult::Failed )
                {
                    cerr << "Got a buffer, but it hasn't been successfully grabbed." << endl;
                }
            
                // A successfully grabbed buffer can be processed now. The buffer will not be overwritten with new data until
                // it is explicitly placed in the acquisition engine's input queue again.
                if ( grabResult.status == GrabResult::Ok )
                {
                    processData( camIdx, grabResult );
                }
            
                // Data processing has finished. Put the buffer back into the acquisition 
                // engine's input queue to be filled with new image data.
                if ( grabResult.status != GrabResult::Timeout)
                {
                    m_Cameras.at(camIdx)->QueueBuffer( grabResult.hBuffer );
                }
            
            }
        } while ( (nImagesGrabbed < nImagesToGrab) && ! TimoutOccurred );

        for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
        {
            // Stop the camera.
            m_Cameras.at(camIdx)->IssueAcquisitionStopCommand();

            // Stop the acquisition engine and release memory buffers and other resources used for grabbing.
            m_Cameras.at(camIdx)->FinishAcquisition();

            // Get the statistics gathered during acquisition.
            getStatistics(camIdx);

            // Close connection to camera.
            m_Cameras.at(camIdx)->Close();
        }
    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
    }

}

void Sample::processData(size_t camIdx,  const GrabResult& grabResult )
{
    // Do not try to access data members of the grab result when a timeout has occurred
    // because the grab result doesn't represent a valid buffer in that case.
    // Also, in case of a failed grab, the buffer and the grab result members may
    // not be valid.
    if ( grabResult.status != GrabResult::Timeout )
    {
        uint64_t timeStamp = grabResult.timeStamp;
        uint64_t frameID = grabResult.frameID;
        if ( 0 == (frameID % 10) ) 
        {
            // Display timestamp in milliseconds.
            long double timeStampMillis = timeStamp / 1000000.0L;
            cout << fixed << setprecision(6) << "camera " << camIdx << "\tframeID = " 
                << frameID << "\ttimestamp = " << timeStampMillis << " ms" << endl;
        }
    }
   
}

void Sample::getStatistics(size_t camIdx) {
    
    cout << endl;

    cout << "Statistics of Camera " << camIdx << endl;
    cout << "--------------------------" << endl;
        
    // Get pointers to statistic nodes.
    CIntegerPtr ptrTotalBufferCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Total_Buffer_Count");
    CIntegerPtr ptrFailedBufferCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Failed_Buffer_Count");
    CIntegerPtr ptrBufferUnderrunCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Buffer_Underrun_Count");
    CIntegerPtr ptrTotalPacketCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Total_Packet_Count");
    CIntegerPtr ptrFailedPacketCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Failed_Packet_Count");
    CIntegerPtr ptrResendRequestCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Resend_Request_Count");
    CIntegerPtr ptrResendPacketCount = m_Cameras.at(camIdx)->GetParameter("Statistic_Resend_Packet_Count");

    // Print statistics.
    cout << "Total number of buffers retrieved : " << ptrTotalBufferCount->GetValue() << endl;
    cout << "Number of failed buffers : " << ptrFailedBufferCount->GetValue() << endl;
    cout << "Number of buffers underruns : " << ptrBufferUnderrunCount->GetValue() << endl;
    cout << "Total number of packets : " << ptrTotalPacketCount->GetValue() << endl;
    cout << "Number of failed packets : " << ptrFailedPacketCount->GetValue() << endl;
    cout << "Number resend requests occurred : " << ptrResendRequestCount->GetValue() << endl;
    cout << "Number of packets resent : " << ptrResendPacketCount->GetValue() << endl;
        
}


int Sample::run()
{
    try
    {
        //
        // Find all ToF cameras, open and parametrize them.
        //
        setupCameras();

        //
        // Let cameras negotiate which one should be the master.
        // 
        findMaster();

        //
        // Wait until all camera clocks have been synchronized.
        //
        syncCameras();

        //
        // Set trigger delay and synchronous free run trigger rate for each camera.
        //
        setTriggerDelays();

        //
        // Grab a pre-defined number of images.
        //
        grabImages();

    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    
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
        CToFCamera::TerminateProducer();  // Won't throw any exceptions.

    cout << endl << "Press Enter to exit." << endl;
    while (cin.get() != '\n');

    return exitCode;
}
