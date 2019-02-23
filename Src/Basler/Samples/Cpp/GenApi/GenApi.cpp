/*
    This sample illustrates how to configure the ToF camera using the GenICam GenApi.

    Access to ToF cameras is provided by a GenICam-compliant GenTL Producer. A GenTL 
    Producer is a dynamic library implementing a standardized software interface for 
    accessing the camera.

    The software interacting with the GenTL Producer is called a GenTL Consumer. Using this terminology,
    this sample is a GenTL Consumer, too.

    For camera configuration and access to other parameters, a GenTL consumer
    uses the technologies defined by the GenICam standard hosted by the
    European Machine Vision Association (EMVA). The GenICam specification
    (http://www.emva.org/standards-technology/genicam/) defines a format for camera description files.
    These files describe the configuration interface of GenICam-compliant cameras.
    The description files are written in XML and describe camera registers, their
    interdependencies, and all other information needed to access high-level features 
    such as Gain, Exposure Time, or Image Format by means of low-level register read and
    write operations.

    The elements of a camera description file are represented as software
    objects called "nodes". For example, a node can represent a single camera
    register, a camera parameter such as Gain, a set of available parameter
    values, etc. Each node implements the GenApi::INode interface.

    The nodes are linked by different relationships as explained in the
    GenICam standard document available at www.GenICam.org. The complete set of
    nodes is stored in a data structure called a "node map".
    At runtime, a node map is instantiated from an XML description.

    The names and types of the parameter nodes can be found by using the pylon Viewer tool.

    The GenICam GenApi standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_Standard_v2_0.pdf
    The GenTL standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf

*/

#include "stdafx.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <string>
#include <iostream>

using namespace GenTLConsumerImplHelper;
using namespace GenApi;
using namespace std;

// Adjust a value to make it comply with range and increment constraints.
//
// The parameter's minimum and maximum are always considered valid values.
// If the increment is larger than one, the returned value will be: min + (n * inc).
// If the value doesn't meet these criteria, it will be rounded down to ensure compliance.
int64_t Adjust(int64_t val, int64_t minimum, int64_t maximum, int64_t inc)
{
    // Check the lower bound.
    if (val < minimum)
    {
        return minimum;
    }

    // Check the upper bound.
    if (val > maximum)
    {
        return maximum;
    }

    // Check the increment.
    if (inc == 1)
    {
        // Special case: all values are valid.
        return val;
    }
    else
    {
        // The value must be min + (n * inc).
        // Due to the integer division, the value will be rounded down.
        return minimum + ( ((val - minimum) / inc) * inc );
    }
}

int main(int argc, char* argv[])
{
    int exitCode = EXIT_FAILURE;

    CToFCamera camera;
    try
    {
        CToFCamera::InitProducer();
        camera.OpenFirstCamera();

        // 
        // Common properties of all parameter types
        //

        // We use GenApi's smart pointer like classes for a convenient way to get access 
        // to parameters. GenApi supports different parameter types, for each of them 
        // there is a corresponding parameter class: CIntgegerPtr, CFloatPtr, CEnumerationPtr, 
        // CBooleanPtr, etc.
        CIntegerPtr ptrFancyParameter = camera.GetParameter( "FancyParameter" );

        // Although the camera doesn't provide this fancy parameter, the camera class
        // returns a smart pointer. Accessing the smart pointer will throw an exception 
        // since there is no corresponding GenApi node object the pointer could provide 
        // access to. Trying to access the value or the properties of ptrFancyParameter will result
        // in an exception. 
        // Check to see whether GetParameter returned a valid pointer to a parameter:
        if ( ! ptrFancyParameter.IsValid() )
        {
            cout << "Camera doesn't provide the parameter 'FancyParameter'" << endl;
        }

        // Parameters might be read-only. When accessing read-only parameters, we will get
        // an exception. The GenApi::IsWritable() function can be used to check whether
        // a value can be set.
        // GenApi provides some further convenience functions for checking the accessibility
        // of nodes: GenApi::IsReadable(), GenApi::IsImplemended().
        // These functions apply to all parameter types.

        // Of course, a non-existent parameter is not readable...
        if ( ! IsReadable(ptrFancyParameter) )
        {
            cout << "'FancyParameter' is not readable."  << endl;
        }

        CFloatPtr ptrDeviceTemperature = camera.GetParameter ( "DeviceTemperature" );
        if ( ! IsWritable(ptrDeviceTemperature) )
        {
            cout << "Device temperature is not writable." << endl;
        }
        // All parameter types can be converted to a string using the ToString() method.
        if ( IsReadable(ptrDeviceTemperature) )
        {
            cout << "Current temperature is " << ptrDeviceTemperature->ToString() << endl;
        }

        //
        // Usage of integer parameters
        //

        // Get the integer nodes describing the camera's area of interest (AOI).
        CIntegerPtr ptrOffsetX( camera.GetParameter("OffsetX"));
        CIntegerPtr ptrOffsetY( camera.GetParameter("OffsetY"));
        CIntegerPtr ptrWidth( camera.GetParameter("Width"));
        CIntegerPtr ptrHeight( camera.GetParameter("Height"));

        // Set the AOIs offset (i.e. upper left corner of the AOI) to the allowed minimum.
        ptrOffsetX->SetValue( ptrOffsetX->GetMin() );
        ptrOffsetY->SetValue( ptrOffsetY->GetMin() );

        // Some properties have restrictions, i.e., the values must be a multiple of a certain
        // increment. Use GetInc/GetMin/GetMax to make sure you set a valid value, as shown
        // in the Adjust() function above.
        int64_t newWidth = 202;
        newWidth = Adjust(newWidth, ptrWidth->GetMin(), ptrWidth->GetMax(), ptrWidth->GetInc());

        int64_t newHeight = 101;
        newHeight = Adjust(newHeight, ptrHeight->GetMin(), ptrHeight->GetMax(), ptrHeight->GetInc());

        ptrWidth->SetValue(newWidth);
        ptrHeight->SetValue(newHeight);

        cout << "OffsetX          : " << ptrOffsetX->GetValue() << endl;
        cout << "OffsetY          : " << ptrOffsetY->GetValue() << endl;
        cout << "Width            : " << ptrWidth->GetValue() << endl;
        cout << "Height           : " << ptrHeight->GetValue() << endl;

        // Reset the AOI to its full size.
        ptrWidth->SetValue(ptrWidth->GetMax() );
        ptrHeight->SetValue(ptrHeight->GetMax());

        // 
        // Usage of enumeration parameters, selectors, and Boolean parameters
        //
        // Enumeration parameters, as the PixelFormat, represent a defined set of named values.
        // Selectors are enumeration parameters influencing the state of the parameters
        // that are "selected by" the selector.
        // The ComponentSelector feature, as an example, is a selector. The feature controls 
        // the number of parts an acquired image consists of. Each part can be individually 
        // enabled by using the ComponentEnable parameter.
        // For some parts there are multiple options for the part's pixel format.
        // Thus, the ComponentSelector "selects" the PixelFormat and the Component parameters.
        //
        // All parameter types provide a corresponding FromString() method allowing to set
        // a parameter by using a string. The FromString() method is the only way to set the
        // value of an enumeration parameter.
        // 
        // Example: Iterate over all parts and enable those parts that support RGB data.

        CEnumerationPtr ptrComponentSelector = camera.GetParameter("ComponentSelector");
        CBooleanPtr ptrComponentEnable = camera.GetParameter("ComponentEnable");
        CEnumerationPtr ptrPixelFormat( camera.GetParameter("PixelFormat") );
        NodeList_t entries;
        ptrComponentSelector->GetEntries( entries );
        for ( NodeList_t::const_iterator entry = entries.begin(); entry != entries.end(); ++entry)
        {
            CEnumEntryPtr ptrEntry( *entry );
            cout << "Camera supports a " << ptrEntry->GetSymbolic() << " part: " << (IsAvailable( ptrEntry ) ? "yes" :  "no") << endl;
            if ( IsAvailable( ptrEntry ) )
            {
                // First, select the part to configure...
                ptrComponentSelector->FromString( ptrEntry->GetSymbolic() );
                // ... then access the parameters for the part.
                if ( IsAvailable( ptrPixelFormat->GetEntryByName( "RGB8")))
                {
                    ptrPixelFormat->FromString( "RGB8");
                    ptrComponentEnable->SetValue(true);
                }
                cout << "Pixel format of part " << ptrComponentSelector->ToString() << ": " << ptrPixelFormat->ToString() << endl;
                cout << "Part is " << (ptrComponentEnable->GetValue() ? "enabled" : "disabled") << endl;
            }
        }

        //
        // Usage of floating point parameters
        //
        CFloatPtr ptrAcquisitionFrameRate( camera.GetParameter("AcquisitionFrameRate") );
        cout << "Current acquisition frame rate: " << ptrAcquisitionFrameRate->GetValue();
        // Set to maximum
        ptrAcquisitionFrameRate->SetValue( ptrAcquisitionFrameRate->GetMax() );
        cout << ". New value: " << ptrAcquisitionFrameRate->GetValue() << endl;

        // 
        // Configuring the camera's exposure time
        //
        //
        // By default, the exposure time is controlled automatically. Switch off the 
        // automatic control first.
        CEnumerationPtr( camera.GetParameter("ExposureAuto"))->FromString("Off");
        // Now the value can be controlled manually.
        CFloatPtr ptrExposureTime( camera.GetParameter("ExposureTime") );
        ptrExposureTime->SetValue( ptrExposureTime->GetMin() * 3 );
        cout << "Set exposure time to " << ptrExposureTime->GetValue() << endl;

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

    // Release the GenTL producer and all of its resources. 
    // Note: Don't call TerminateProducer() until the destructor of the CToFCamera
    // class has been called. The destructor may require resources which may not
    // be available anymore after TerminateProducer() has been called.
    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions

    cout << endl << "Press Enter to exit." << endl;
    while (cin.get() != '\n');

    return exitCode;
}

