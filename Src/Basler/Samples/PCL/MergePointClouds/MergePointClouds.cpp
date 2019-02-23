/*
This sample code demonstrates how to apply a static coordinate transform to a given number of point clouds using
the Point Cloud Library (PCL). The aim of this is to generate a single point cloud from multiple cameras at different (fixed) positions.

For this purpose, the grab results of the ToF camera are converted into pcl::PointCloud<pcl::PointXYZRGB> objects, which are
the data structures provided by the PCL for representing point cloud data. After that, the "Eigen" library is used to generate
a transformation matrix that represents an affine transformation (https://en.wikipedia.org/wiki/Transformation_matrix#Affine_transformations).
This matrix is then used to rotate and translate every point in a point cloud.

The ToF cameras are configured to use the PTP/synchronous free run feature to avoid mutual disturbances between the devices. Refer to
the SyncFreerun sample contained in the Basler ToF Driver package for more details about how to synchronize multiple cameras.

In a last step, the PCL point cloud viewer class is used to render the resulting point cloud.
*/
#include "stdafx.h"
#include <iostream>
#include <algorithm> 


//
// Each camera has a right-handed local coordinate system. The origin of each coordinate system is located on the 
// camera's front surface, in the center of the lens. The z axis points away from the camera, the y axis points 
// downwards and the x axis points to the right. Each camera will create an individual point cloud, the coordinates 
// of the points are relative to the cameras' local coordinate systems.
//
// In order to merge multiple point clouds, all point clouds have to be transformed to one common coordinate
// system. We will use the coordinate system of cam 0 as the common reference coordinate system. This means that for the 
// first camera no coordinate transformation is required.
// All other coordinate systems of the cameras have to be rotated and translated relative to the first camera. 
// This means that for each point cloud, except the one from the first camera, a coordinate transformation needs to be applied. 
// The coordinate transformations are defined by a rotation about the y axis (because we assume that all the cameras
// are located in one common XZ-plane and therefore only a rotation about the y axis is required) and a translation.
//
// In this sample we assume that two cameras observe one object from the same distance. The viewing directions of
// the two cameras are perpendicular to each other. The distance of both cameras to the center of the object is 2000 mm.
//
// The point cloud of the second camera must be rotated around the y axis by 90 degrees (PI/2) and then translated along the
// x and z axes by 2000 mm. The rotation angle and the translation values are defined in the ROTATION_ANGLES, SHIFT_X, 
// SHIFT_Y, and SHIFT_Z arrays below.



/*


                     Object  <--------- cam 0  
                        ^
                        |
                        |                          
                        |                             ^ x
                        |                             |
                      cam 1                           | 
                                                      | 
                                             <---------   
                                             z
                                             The common coordinate system 
                                             (the one from cam 0)
*/



// Defines the number of cameras to be used. When increasing the number, the ROTATION_ANGLES and SHIFT_* arrays
// below must be extended and set up accordingly.
const size_t NUM_CAMERAS = 2;

// Defines the angles for the rotation of the point clouds. This will be used in the 'mergePointClouds()' function.
const float ROTATION_ANGLES[NUM_CAMERAS] = { 0.f, (float) -M_PI_2 }; 

// Defines the X,Y,Z values which will be used to shift the individual point clouds in the 'mergePointClouds()' function.
// Adjusts the shift values according to your camera setup.
const float SHIFT_X[NUM_CAMERAS] = { 0.f, 2000.f };
const float SHIFT_Y[NUM_CAMERAS] = { 0.f, 0.f };
const float SHIFT_Z[NUM_CAMERAS] = { 0.f, -2000.f };

// Defines a region of interest for the z axis (in mm).
// Points outside that range will be omitted. This is useful for suppressing
// the background of the scene. Adjust these values according to your setup.
const unsigned int DepthMin = 0;
unsigned int DepthMax = 2000;

// Includes for the Basler ToF camera class.
#include <ConsumerImplHelper/ToFCamera.h>

// Additional includes for the PCL.
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/common/transforms.h>

// Utility class that sets up n cameras for synchronous free run and
// continuously grabs depth and intensity data.
#include "SynchronousGrabber.h"

// Some namespaces that are used.
using namespace GenTLConsumerImplHelper;
//using namespace GenApi;
using namespace pcl;
using namespace std;

// Typedefs for the PCL types used.
typedef PointXYZRGB Point_t;
typedef PointCloud<Point_t> PointCloud_t;
typedef PointCloud_t::Ptr PointCloudPtr;





/*
Convert the Basler point cloud to a PCL point cloud.
*/
PointCloudPtr convertGrabResultToPointCloud(const BufferParts& imgParts)
{
    // An organized point cloud is used, i.e., for each camera pixel there is an entry
    // in the data structure indicating the 3D coordinate calculated from that pixel.
    // If the camera couldn't create depth information for a pixel, the x,y, and z coordinates
    // are set to NaN. These NaNs will be retained in the PCL point cloud.

    // Allocate PCL point cloud.
    const size_t width = imgParts[0].width;
    const size_t height = imgParts[0].height;
    PointCloudPtr ptrPointCloud(new PointCloud_t);
    ptrPointCloud->width = width;
    ptrPointCloud->height = height;
    ptrPointCloud->points.resize(width * height);
    ptrPointCloud->is_dense = false; // Indicates an organized point cloud that may contain NaN values

    // Create a pointer to the 3D coordinates of the first point.
    // imgParts[0] always refers to the point cloud data.
    CToFCamera::Coord3D* pSrcPoint = (CToFCamera::Coord3D*) imgParts[0].pData;

    // Create a pointer to the intensity information, stored in the second buffer part.
    uint16_t* pIntensity = (uint16_t*)imgParts[1].pData;

    // Set the points.
    for (size_t i = 0; i < height * width; ++i, ++pSrcPoint, ++pIntensity)
    {
        // Set the X/Y/Z cordinates.
        Point_t& dstPoint = ptrPointCloud->points[i];

        dstPoint.x = pSrcPoint->x;
        dstPoint.y = pSrcPoint->y;
        dstPoint.z = pSrcPoint->z;

        // Use the intensity value of the pixel for coloring the point.
        dstPoint.r = dstPoint.g = dstPoint.b = (uint8_t)(*pIntensity >> 8);
    }

    return ptrPointCloud;
}

/*
Merge multiple point clouds into one single cloud by rotating and shifting the individual clouds
according to the fixed relative position of the ToF cameras.
*/
PointCloudPtr mergePointClouds(vector<PointCloudPtr>& clouds)
{
    using namespace Eigen;

    // We expect NUM_CAMERAS point clouds as input.
    if (!(clouds.size() == NUM_CAMERAS)) return nullptr;

    // Create a point cloud to store the result.
    PointCloudPtr ptrMerged(new PointCloud_t);

    // We will rotate every point cloud about the y axis here.
    Vector3f rotationAxis = Vector3f::UnitY();

    // Iterate over the individual point clouds.
    for (int i = 0; i < NUM_CAMERAS; i++) {
        // Create a vector to represent the shift (translation) of the current point cloud.
        Vector3f shiftVector(SHIFT_X[i], SHIFT_Y[i], SHIFT_Z[i]);

        // We will use an affine transformation matrix in order to rotate and translate the point cloud in just one step.
        Affine3f transform = Affine3f::Identity();
        transform.rotate(AngleAxisf(ROTATION_ANGLES[i], rotationAxis));
        transform.translate(shiftVector);

        // Apply the transformation.
        transformPointCloud(*clouds.at(i), *clouds.at(i), transform);

        // Add the transformed point cloud to the result.
        *ptrMerged += *clouds.at(i);
    }

    return ptrMerged;
}

/*
Initialize the PCL CloudViewer. This function only gets called once.
See http://pointclouds.org/documentation/tutorials/cloud_viewer.php for further information.
*/
void initCloudViewer(visualization::PCLVisualizer& viewer)
{
    // Set some properties of the visualizer.
    viewer.setBackgroundColor(0.1, 0.1, 0.1);
#if PCL_VERSION_COMPARE(>=,1,8,0)
    viewer.removeAllCoordinateSystems();
#endif
    // Set up the visualizer's camera.

    // The camera position.
    const double pos_x = 0; const double pos_y = 0; const double pos_z = -4000;

    // The point the camera is looking at.
    const double view_x = 0; const double view_y = 0; const double view_z = 3000;

    // Defines the view-up direction of the camera ( up_y == -1 --> y axis of the ToF camera points down ).
    const double up_x = 0; const double up_y = -1; const double up_z = 0;
    viewer.setCameraPosition(pos_x, pos_y, pos_z, view_x, view_y, view_z, up_x, up_y, up_z);

    // Set the size of the rendered points.
    viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 3);
}

int main()
{
    try {
        // Initialize the producer and open the camera.
        CToFCamera::InitProducer();
        SynchronousGrabber grabber(NUM_CAMERAS, DepthMin, DepthMax);

        int setupRet = grabber.setupAndStart();
        if (setupRet == EXIT_SUCCESS) {
            // Smart pointer for accessing the CloudViewer.
            boost::shared_ptr<pcl::visualization::CloudViewer> viewer;
            bool isViewerInitalized = false;

            cout << "Press 'q' to exit." << endl;

            // Create a vector to store the buffer parts from every camera.
            vector<shared_ptr<BufferParts> > bufParts;

            // Grab images and process point clouds until the CloudViewer is closed.
            do {

                // Grab the next images.
                int grabRetID = grabber.getNextImages(bufParts);
                if (grabRetID == EXIT_SUCCESS)
                {
                    vector<PointCloudPtr> cloudPointers;

                    for (int i = 0; i < NUM_CAMERAS; i++) {
                        // Convert the grab results to an organized PCL point cloud.
                        cloudPointers.push_back(convertGrabResultToPointCloud(*bufParts.at(i)));
                    }

                    //
                    // Further filters etc. could be applied to the point clouds here.
                    //

                    // Merge the individual point clouds.
                    PointCloudPtr ptrMergedCloud = mergePointClouds(cloudPointers);

                    // Create the CloudViewer.
                    if (viewer == 0)
                    {
                        viewer.reset(new pcl::visualization::CloudViewer("Merged Point Clouds"));
                    }

                    // Show the resulting point cloud.
                    if (ptrMergedCloud)
                    {
                        viewer->showCloud(ptrMergedCloud);
                    }

                    // After showing a point cloud for the first time, it is safe
                    // to adjust the settings for rendering the point cloud.
                    if (!isViewerInitalized)
                    {
                        isViewerInitalized = true;
                        viewer->runOnVisualizationThreadOnce(initCloudViewer);
                    }
                }
            } while (!viewer || !viewer->wasStopped());
        }
    }
    catch (GenICam::GenericException& ex) {
        cerr << "An exception occurred:" << endl << ex.what() << endl;
    }

    // Close the camera and clean up resources.
    if (CToFCamera::IsProducerInitialized()) CToFCamera::TerminateProducer();

    return 0;
}
