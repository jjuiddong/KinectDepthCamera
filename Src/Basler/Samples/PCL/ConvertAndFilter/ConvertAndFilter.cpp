/*
This sample code demonstrates how the Point Cloud Library (PCL) can be used to filter, transform, or manipulate
the point clouds grabbed with a Basler ToF camera.

For this purpose, the grab results of the ToF camera are converted into pcl::PointCloud<pcl::PointXYZRGB> objects which are
the data structures provided by the PCL for representing point cloud data.


The PCL point cloud viewer class is used to render the resulting point cloud.

Refer to ReadMe.txt for further information.
*/
#include "stdafx.h"
#include <iostream>

// Includes for the Basler ToF camera
#include <ConsumerImplHelper/ToFCamera.h>


// Additional includes for the PCL
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>

// Some namespaces that are used
using namespace GenTLConsumerImplHelper;
using namespace GenApi;
using namespace pcl;
using namespace std;

// Typedefs for the PCL types used
typedef PointXYZRGB Point_t;
typedef PointCloud<Point_t> PointCloud_t;
typedef PointCloud_t::Ptr PointCloudPtr;

/*
Convert the Basler point cloud to a PCL point cloud.
*/
PointCloudPtr convertGrabResultToPointCloud(const BufferParts& imgParts)
{
    // An organized point cloud is used, i.e., for each camera pixel there is an entry 
    // in the data structure indicating the 3D coordinates calculated from that pixel.
    // If the camera wasn't able to create depth information for a pixel, the x-,y-, and z- coordinates 
    // are set to NaN. These NaNs will be retained in the PCL point cloud.

    // Allocate PCL point cloud.
    const size_t width = imgParts[0].width;
    const size_t height = imgParts[0].height;
    PointCloudPtr ptrPointCloud(new PointCloud_t);
    ptrPointCloud->width = width;
    ptrPointCloud->height = height;
    ptrPointCloud->points.resize(width * height);
    ptrPointCloud->is_dense= false; // Indicates an organized point cloud that may contain NaN values.

    // Create a pointer to the 3D coordinates of the first point.
    // imgParts[0] always refers to the point cloud data.
    CToFCamera::Coord3D* pSrcPoint = (CToFCamera::Coord3D*) imgParts[0].pData;
    
    // Create a pointer to the intensity information, stored in the second buffer part.
    uint16_t* pIntensity = (uint16_t*) imgParts[1].pData;

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
    
    // Defines the view-up direction of the camera ( up_y == -1 --> y axis of the ToF camera points down )
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
        CToFCamera cam;
        cam.OpenFirstCamera();

        // Configure the camera to send 3D coordinates and intensity values as we will
        // use them as colors for the points in the point cloud.

        CEnumerationPtr ptrComponentSelector = cam.GetParameter("ComponentSelector");
        CBooleanPtr ptrComponentEnable = cam.GetParameter("ComponentEnable");
        CEnumerationPtr ptrPixelFormat = cam.GetParameter("PixelFormat");

        ptrComponentSelector->FromString("Range");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Coord3D_ABC32f");

        ptrComponentSelector->FromString("Intensity");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Mono16");
        
        // Provide access to the grabbed depth and intensity data.
        GrabResultPtr ptrGrabResult;
        BufferParts imgParts;

        // Smart pointer for accessing the cloud viewer.
        boost::shared_ptr<pcl::visualization::CloudViewer> viewer;
        bool isViewerInitalized = false;

        cout << "Press 'q' to exit." << endl;

        // Grab images and process point clouds until the point cloud viewer is closed.
        do {
            
            // Grab the next image.
            ptrGrabResult = cam.GrabSingleImage(1000, &imgParts);

            if (ptrGrabResult->status == GrabResult::Ok)
            {
                // Convert the grab result to an organized PCL point cloud.
                PointCloudPtr ptrCloud = convertGrabResultToPointCloud(imgParts);

                // As an example, a simple VoxelGrid filter will be used for subsampling and removing noise.
                
                // Create the VoxelGrid filter and use the converted point cloud as input.
                VoxelGrid<PointXYZRGB> voxelGrid;
                voxelGrid.setInputCloud(ptrCloud);

                // Specify the size of the voxels.
                voxelGrid.setLeafSize(10.0f, 10.0f, 20.0f);

                // Apply the voxel grid filter.
                PointCloudPtr ptrFilteredCloud(new PointCloud_t);
                voxelGrid.filter(*ptrFilteredCloud);

                // Create the point cloud viewer.
                if (viewer == 0)
                {
                    viewer.reset(new pcl::visualization::CloudViewer("Cloud Viewer"));
                }

                // Show the resulting point cloud.
                viewer->showCloud(ptrFilteredCloud);

                // After showing a point cloud for the first time, it is safe 
                // to adjust the settings for rendering the point cloud.
                if (!isViewerInitalized)
                {
                    isViewerInitalized = true;
                    viewer->runOnVisualizationThreadOnce(initCloudViewer);
                }
            }
        } while (!viewer->wasStopped());
    }
    catch (GenICam::GenericException& ex) {
        cerr << "An exception occurred:" << endl << ex.what() << endl;
    }

    // Close the camera and clean up resources.
    if (CToFCamera::IsProducerInitialized()) CToFCamera::TerminateProducer();

    return 0;
}

