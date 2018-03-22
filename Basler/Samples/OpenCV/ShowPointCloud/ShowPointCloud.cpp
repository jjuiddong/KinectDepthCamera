/* This sample illustrates how to process and display images acquired with a ToF camera
 * in OpenCV.
 * Three image windows are displayed:
 * 	- The intensity images
 * 	- Point cloud, points colored with intensity value
 * 	- Point cloud, points colored according to distance
 *
 * The point cloud is projected to a 2D image using a virtual pinhole camera.
 * The position and angle of the camera can be controlled with the keyboard.
 * 
 * Adjust the Makefile or Visual Studio project settings to the version 
 * of the OpenCV library installed on your system.
 * 
 * The sample has been tested using OpenCV 2.4.13 (Windows) and 2.4.9 (Linux) respectively.
 *
 */

/*  
    Information for Visual Studio users
    ====================================

    The Basler ToF Driver installer installs all header files, import libraries, and 
    DLLs of the OpenCV library that are required to build and run the OpenCV samples.
    The OpenCV files are located in the Samples/OpenCV/include and Samples/OpenCV/x86 folders.
    Import libraries and DLLs for Visual Studio 2012 and Visual Studio 2013 are provided.

    The project settings are configured to use the libraries and DLLs targeting Visual Studio 2012.

    Since OpenCV always requires that the OpenCV libraries and DLLs match the Visual Studio version used
    for building an application, you manually have to adjust the project settings in order to use the
    correct version of the OpenCV libraries and DLLs.
    In particular, you will have to adjust the settings for the "Output Directory" and the "Additional 
    Library Directories".

    In case you are going to use Visual Studio 2015 or newer, you cannot use the OpenCV files that have 
    been installed together with the samples. Instead, you will have to provide an OpenCV installation 
    that supports the Visual Studio version you want to use. 
    For building the OpenCV solution with Visual Studio 2015 or newer, the "Additional Dependencies" 
    settings have to be adjusted as well as the settings mentioned above.
*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <vector>
#include <limits>

#include <opencv2/opencv.hpp>

using namespace GenTLConsumerImplHelper;
using namespace std;
using namespace cv;

class Sample
{
public:
    Sample();
    int run();
    bool onImageGrabbed( GrabResult grabResult, BufferParts );

private:
    CToFCamera      m_Camera;   // the ToF camera
    double          m_inc;      // displacement of translation vector per keystroke
    Point3d         m_t;        // translation vector of camera
    Mat             m_R;        // rotation matrix
    double          m_phiX;     // rotation of camera around x-axis
    double          m_phiY;     // rotation of camera around y-axis
    double          m_deltaPhi; // increment of rotation angle
    double          m_deltaF;   // increment of the focal length when zooming in and out
    double          m_f;        // pinhole camera's focal length

    void Reset();
};

Sample::Sample()
{
    Reset();
};

void Sample::Reset()
{
    m_inc = 50;
    m_deltaPhi = 0.05;
    m_phiX = 0.4;
    m_phiY = 0.4;
    m_t = Point3d( -1800, 1300, -100 );  // initial position of the camera (chosen at random)
    m_f = 10;
    m_deltaF = 0.5;
}

int Sample::run()
{
    try
    {
        m_Camera.OpenFirstCamera();
        cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;
        cout << "Keyboard navigation: "  << endl;
        cout << "  Arrow keys left/right:\tmove camera horizontally." << endl;
        cout << "  Arrow keys up/down:\t\tmove camera vertically." << endl;
        cout << "  Numpad 4/6:\t\t\trotate camera around y axis." << endl;
        cout << "  Numpad 8/2:\t\t\trotate camera around x axis." << endl;
        cout << "  Numpad 5:\t\t\treset to initial camera position." << endl;
        cout << "  Numpad +/-:\t\t\tzoom in and out." << endl;
        cout << "  q:\t\t\t\tquit program" << endl;
        cout << endl << "For keyboard navigation to work, one of the image windows must have the focus." << endl;

        // Enable 3D (point cloud) data and intensity data 
        GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
        GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
        GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");
        ptrComponentSelector->FromString("Range");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Coord3D_ABC32f" );
        ptrComponentSelector->FromString("Intensity");
        ptrComponentEnable->SetValue(true);

        namedWindow("Intensity", CV_WINDOW_AUTOSIZE );
        namedWindow("Point Cloud (intensity)", CV_WINDOW_AUTOSIZE );
        namedWindow("Point Cloud (distance)", CV_WINDOW_AUTOSIZE );

        // Acquire images until the call-back onImageGrabbed indicates to stop acquisition. 
        m_Camera.GrabContinuous( 2, 1000, this, &Sample::onImageGrabbed );

        // Clean-up
        m_Camera.Close();
    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool Sample::onImageGrabbed( GrabResult grabResult, BufferParts parts )
{
    int key = 0;
    if ( grabResult.status == GrabResult::Timeout )
    {
        cerr << "Timeout occurred. Acquisition stopped." << endl;
        return false; // Indicate to stop acquisition
    }
    if ( grabResult.status != GrabResult::Ok )
    {
        cerr << "Image was not grabbed." << endl;
    }
    else
    {
        try
        {
            // Create OpenCV images from grabbed buffer
            const int width = (int) parts[0].width;
            const int height = (int) parts[0].height;
            const int count = width * height;
            Mat cloud( count, 3, CV_32FC1, parts[0].pData ); 
            Mat intensity = Mat(height, width, CV_16UC1, parts[1].pData);  
            // Scale the intensity image since it often looks quite dark
            double  max;
            minMaxLoc( intensity, NULL, &max);
            intensity /= ( max / std::numeric_limits<uint16_t>::max() );

            // Display the intensity image
            imshow("Intensity", intensity);

            // Calculate parameters for projecting 3D data to 2D image
            // using a pinhole camera model.
            Mat A=Mat::eye(3,3,CV_64F);  // Intrinsic camera parameters
            A.at<double>(0,0) = m_f;
            A.at<double>(1,1) = m_f;
            A.at<double>(0,2) = parts[0].width / 2.0;
            A.at<double>(1,2) = parts[0].height / 2.0;
            Mat F=Mat::eye(3,3,CV_64F);  // Used for flipping the image
            F.at<double>(0,0) = -1;
            F.at<double>(1,1) = -1;
            F.at<double>(0,2) = (double) (width - 1);
            F.at<double>(1,2) = (double) (height -1);
            A = F*A;
            // m_R and m_t are the extrinsic parameters
            const double cosphix = cos(m_phiX);
            const double sinphix = sin(m_phiX);
            const double cosphiy = cos(m_phiY);
            const double sinphiy = sin(m_phiY);
            double r[3][3] = {
                { cosphiy, 0, sinphiy },
                { sinphix*sinphiy, cosphix, -sinphix*cosphiy},
                { 0, 0, 0} };
                m_R = Mat( 3, 3, CV_64F, r);

                Mat distCoeffs = Mat::zeros(Size(5,1),CV_32F); // No lens distortion
                // Perform the projection
                std::vector<Point2f> points2D;
                projectPoints( cloud, m_R, Mat(m_t), A, distCoeffs, points2D );

                // We create two images. For the first one, we assign each point its corresponding intensity value.
                // For the second image, we assign each point the distance to the pinhole camera.
                Mat intensityMap = Mat::zeros(height, width, CV_16UC1 );
                Mat distanceMap(height, width, CV_64FC1 );
                distanceMap.setTo( std::numeric_limits<double>::infinity() );

                for ( int row = 0; row < height; ++row )
                {
                    int idx = row * width;
                    for ( int col = 0; col < width; idx++, col++ )
                    {
                        double x = cloud.at<float>(idx, 0);
                        if ( x < 1e6 )
                        {
                            double y = cloud.at<float>(idx, 1);
                            double z = cloud.at<float>(idx, 2);

                            double dx = m_t.x - x;
                            double dy = m_t.y - y;
                            double dz = m_t.z - z;
                            double distance = sqrt(dx*dx+dy*dy+dz*dz);
                            int u=(int)(points2D[idx].x+0.5);
                            int v=(int)(points2D[idx].y+0.5);
                            if(u > 0 && u< width && v > 0 && v< height && distanceMap.at<double>(v,u) > distance  )
                            {
                                // (u,v) are the coordinates to which the 3D point is projected, (col, row) are the 
                                // coordinates of the pixel in the original sensor image.
                                // By comparing the current distance with the distance already stored we ensure that, if multiple 3D points 
                                // are projected to the same destination pixel, the nearest point wins.
                                intensityMap.at<uint16_t>( v, u ) = intensity.at<uint16_t>( row, col );
                                distanceMap.at<double>( v, u  ) =  distance;
                            }
                        }
                    }
                }

                Mat mask;  // Used for masking points without depth information
                compare( distanceMap, std::numeric_limits<double>::infinity(), mask, CMP_GE );
                distanceMap.setTo( 0, mask );
                Mat grey;
                minMaxLoc( distanceMap, NULL, &max );
                distanceMap.convertTo( grey, CV_8UC1, 255.0/max);
                Mat rainbow;
                applyColorMap( grey, rainbow, COLORMAP_RAINBOW);
                rainbow.setTo( 0, mask );
                imshow( "Point Cloud (intensity)", intensityMap );
                imshow( "Point Cloud (distance)", rainbow );
        }
        catch (const Exception&  e)
        {
            cerr << e.what() << endl;
        }
    }
    key = waitKey(1);
    if ( key != -1 )
    {
        switch ( key )
        {
        case 0x260000:  // Windows arrow up
        case 0x10ff52:  // Linux arrow up
             m_t.y += m_inc; break;  // Move camera up
        
        case 0x280000: // Windows arrow down
        case 0x10ff54: // Linux arrow down
            m_t.y -= m_inc; break;  // Move camera down

        case 0x250000: // Windows arrow left
        case 0x10ff51: // Linux arrow left
             m_t.x += m_inc; break;

        case 0x270000: // Windows arrow right
        case 0x10ff53: // Linux arrow right
             m_t.x -= m_inc; break;

        case '+' :     // Windows numpad +
        case 0x10ffab: // Linux numpad +
          m_f += m_deltaF; break;

        case '-' :     // Windows numpad -
        case 0x10ffad: // Linux numpad -
          m_t.z -= m_deltaF; break;

        case '4' :     // Windows numpad 4
        case 0x10ffb4: // Linux numpad 4
          m_phiY -= m_deltaPhi; break;

        case '6' :     // Windows numpad 6
        case 0x10ffb6: // Linux numpad 6
          m_phiY += m_deltaPhi; break;
        
        case '2' :     // Windows numpad 2
        case 0x10ffb2: // Linux numpad 2
          m_phiX -= m_deltaPhi; break;

        case '8':      // Windows numpad 8
        case 0x10ffb8: // Linux numpad 8
          m_phiX += m_deltaPhi; break;

        case '5':      // Windows numpad 5
        case 0x10ffb5: // Linux numpad 5
          Reset(); break;

        }
        cout << "t=" << m_t << " phiX =" << m_phiX << " phiy= " << m_phiY << " f= " << m_f << endl;
    }
    return 'q' != (char) key;
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

    return exitCode;
}
