#pragma once

#include <vector>
#include <memory>
// Includes for the Basler ToF camera class.
#include <ConsumerImplHelper/ToFCamera.h>

// Utility class that sets up n cameras for synchronous free run and
// continuously grabs depth and intensity data.
class SynchronousGrabber
{
public:
    SynchronousGrabber(size_t nCameras, unsigned int depthMin, unsigned int depthMax);
    int setupAndStart();
    int getNextImages(std::vector<std::shared_ptr<GenTLConsumerImplHelper::BufferParts> >& parts);

private:
    void setupCameras();
    void findMaster();
    void syncCameras();
    void setTriggerDelays();

    int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(int camIdx, double timeToMeasureSec, double timeDeltaSec);

private:
    // Vector holding shared pointers to the ToF cameras.
    std::vector<std::shared_ptr<GenTLConsumerImplHelper::CToFCamera>> m_Cameras;

    // Number of cameras to be used.
    int m_nCams;

    // The number of buffers used for image grabbing (per camera).
    const size_t c_nBuffers = 3;

    // Defines a region of interest for the z axis (in mm).
    // Points outside that range will be omitted.
    unsigned int m_DepthMin;
    unsigned int m_DepthMax;

    // Trigger rate in synchronous free run mode [fps] (will be calculated in the setTriggerDelays() method.)
    uint64_t m_SyncTriggerRate;

    // Trigger delay [ns] gets added to synchronous free run start time (will be calculated in the setTriggerDelays method.)
    int64_t m_TriggerDelay;

    // Constant base delay to be added for all cameras [ns].
    static const int64_t m_TriggerBaseDelay = 10000000;    // 10 ms

                                                           // Readout time. [ns]
                                                           // Though basically a constant inherent in the ToF camera, the exact value may still change in future firmware releases.
    static const int64_t c_ReadoutTime = 21000000;

    // The information whether a camera is master is stored here.
    std::vector<bool> m_IsMaster;
};
