/*
    This sample illustrates how to save the acquired 3D data to a simple text file.

    One single image is acquired. The 3D and intensity data are written to a text file.

    The Point Cloud Library data format (.pcd) is used: http://pointclouds.org/documentation/tutorials/pcd_file_format.php.

    You can view .pcd in the free CloudCompare software: http://www.danielgm.net/cc/
    
*/
#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <GenTL/PFNC.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace GenTLConsumerImplHelper;
using namespace GenApi;
using namespace std;

void WritePcdHeader( ostream& o, size_t width, size_t height, bool saveIntensity )
{
    o << "# .PCD v0.7 - Point Cloud Data file format" << endl;
    o << "VERSION 0.7" << endl;
    o << "FIELDS x y z rgb" << endl;
    o << "SIZE 4 4 4";
    if ( saveIntensity )
        o << " 4";
    o << endl;

    o << "TYPE F F F";
    if ( saveIntensity )
        o << " F";
    o << endl;

    o << "COUNT 1 1 1";
    if ( saveIntensity )
        o << " 1";
    o << endl;

    o << "WIDTH " << width << endl;
    o << "HEIGHT " << height << endl;
    o << "VIEWPOINT 0 0 0 1 0 0 0" << endl;
    o << "POINTS " << width * height << endl;
    o << "DATA ASCII" << endl;
}

bool SavePointCloud( const BufferParts& parts, const char* fileName )
{
    if ( parts.empty() )
    {
        cerr << "No valid image data." << endl;
        return false;
    }

    // If the point cloud is enabled, the first part always contains the point cloud data.
    if ( parts[0].dataFormat != PFNC_Coord3D_ABC32f )
    {
        cerr << "Unexpected data format for the first image part. Coord3D_ABC32f is expected." << endl;
        return false;
    }

    const bool saveIntensity = parts.size() > 1;
    if ( saveIntensity && parts[1].dataFormat != PFNC_Mono16 )
    {
        cerr << "Unexpected data format for the second image part. Mono 16 is expected." << endl;
        return false;
    }

    ofstream o( fileName );
    if ( ! o )
    {
        cerr << "Error:\tFailed to create file "<< fileName << endl;
        return false;
    }

    cout << "Writing point cloud to file " << fileName << "...";
    CToFCamera::Coord3D *pPoint = (CToFCamera::Coord3D*) parts[0].pData;
    uint16_t *pIntensity = saveIntensity ? (uint16_t*) parts[1].pData : NULL;
    const size_t nPixel = parts[0].width * parts[0].height;

    WritePcdHeader( o, parts[0].width, parts[0].height, saveIntensity );

    for ( size_t i = 0; i < nPixel; ++i )
    {
        // Check if there are valid 3D coordinates for that pixel.
        if ( pPoint->IsValid() )   
        {
            o.precision( 0 );  // Coordinates will be written as whole numbers.
 
            // Write the coordinates of the next point. Note: Since the coordinate system
            // used by the CloudCompare tool is different from the one used by the ToF camera, 
            // we apply a 180-degree rotation around the x-axis by writing the negative 
            // values of the y and z coordinates.
            o << std::fixed << pPoint->x << ' ' << -pPoint->y << ' ' << -pPoint->z;
            
            if ( saveIntensity )
            {
                // Save the intensity as an RGB value.
                uint8_t gray = *pIntensity >> 8;
                uint32_t rgb = (uint32_t) gray << 16 | (uint32_t) gray << 8 | (uint32_t) gray;
                // The point cloud library data format represents RGB values as floats. 
                float fRgb = * (float*) &rgb;
                o.unsetf(ios_base::floatfield); // Switch to default float formatting
                o.precision(9); // Intensity information will be written with highest precision.
                o << ' ' << fRgb << endl;
            }
        }
        else
        {
            o << "nan nan nan 0" << endl;
        }
        pPoint++;
        pIntensity++;
    }
    o.close();
    cout << "done." << endl;
    return true;
}


int main(int argc, char* argv[])
{
    int exitCode = EXIT_FAILURE;
    const char* fileName = argc > 1 ? argv[1] : "points.pcd";

    CToFCamera camera;
    try
    {
        CToFCamera::InitProducer();
        camera.OpenFirstCamera();

        CEnumerationPtr ptrComponentSelector = camera.GetParameter("ComponentSelector");
        CBooleanPtr ptrComponentEnable = camera.GetParameter("ComponentEnable");
        CEnumerationPtr ptrPixelFormat = camera.GetParameter("PixelFormat");

        // Parameterize the camera to send 3D coordinates and intensity data
        ptrComponentSelector->FromString("Range");
        ptrComponentEnable->SetValue( true );
        ptrPixelFormat->FromString("Coord3D_ABC32f");

        ptrComponentSelector->FromString("Intensity");
        ptrComponentEnable->SetValue(true);
        ptrPixelFormat->FromString("Mono16");

        // Acquire one single image
        BufferParts parts;
        GrabResultPtr ptrGrabResult = camera.GrabSingleImage( 1000, &parts );

        // Save 3D data
        if ( ptrGrabResult->status == GrabResult::Ok )
        {
            SavePointCloud( parts, fileName );
        }
        else
        {
            cerr << "Failed to grab an image." << endl;
        }

        camera.Close();
        exitCode = EXIT_SUCCESS;
    }
    catch ( GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
        // After successfully opening the camera, the IsConnected method can be used 
        // to check if the device is still connected.
        if ( camera.IsOpen() && ! camera.IsConnected() )
        {
            cerr << "Camera has been removed." << endl;
        }

    }

    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions

    cout << endl << "Press Enter to exit." << endl;
    while (cin.get() != '\n');

    return exitCode;
}

