/*
    This sample illustrates how to configure the camera from a pylon feature stream file (.pfs)
    generated with the pylon Viewer.
*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace GenTLConsumerImplHelper;
using namespace std;

void printUsage( char* argv[] )
{
    cerr << "Usage: " << argv[0] << " <pfs file>" << endl;
    cerr << "Example:" << argv[0] << " parameters.pfs" << endl;
}

int main(int argc, char* argv[])
{
    CToFCamera camera;

    int exitCode = EXIT_FAILURE;
    if ( argc != 2 )
    {
        printUsage( argv );
        goto exit;
    }

    try
    {
        CToFCamera::InitProducer();
        camera.OpenFirstCamera();
        camera.ParametrizeFromFile( argv[1] );
        
        // Work with camera...

        camera.Close();
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

    // Release the GenTL producer and all of its resources. 
    // Note: Don't call TerminateProducer() until the destructor of the CToFCamera
    // class has been called. The destructor may require resources which may not
    // be available anymore after TerminateProducer() has been called.
    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions
    exitCode = EXIT_SUCCESS;

exit:
    cout << endl << "Press Enter to exit." << endl;
    while (cin.get() != '\n');

    return exitCode;
}
