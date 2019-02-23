
'  This sample illustrates how to grab images from a ToF camera and how to access the image and depth data 
'  using Visual Basic .NET.
' 
'  Access to ToF cameras is provided by a GenICam-compliant GenTL Producer. A GenTL Producer is a dynamic library
'  implementing a standardized C interface for accessing the camera.
'  
'  The software interacting with the GentL Producer is called a GenTL Consumer. Using this terminology,
'  this sample is a GenTL Consumer, too.
'  As part of the suite of sample programs, Basler provides the ConsumerImplHelper C++ template library that serves as 
'  an implementation helper for GenTL consumers.
'  
'  The ToFCameraWrapper assembly partially wraps this native ConsumerImplHelper library to allow .NET languages to access the 
'  GenTL Producer.
' 
'  For more details about GenICam, GenTL, and GenApi, refer to the http://www.emva.org/standards-technology/genicam/ website.
'  The GenICam GenApi standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_Standard_v2_0.pdf
'  The GenTL standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf
' 

Imports ToFCameraWrapper

Module FirstSample
    ' Handler for the GrabImage event. Demonstrates how to extract the data of the different 
    ' image parts.
    ' !!! WARNING !!!
    ' A thread is used to raise the ImageGrabbed events. This means that if a GUI application 
    ' is used, the handler will NOT be called within the context of the UI thread.
    ' Refer to the RangeMapVB sample for a demonstration of how to marshal operations to the UI thread. 
    Sub ImageGrabbedHandler(ByVal sender As Object, ByVal e As ImageGrabbedEventArgs)
        If GrabResultStatus.Timeout = e.status Then
            Console.WriteLine("Timeout occurred. Acquisition stopped.")
            e.stop = True ' Request to stop image acquisition

            ' The timeout might be caused by the removal of the camera. Check if the camera
            ' is still connected.
            If Not DirectCast(sender, ToFCamera).IsConnected() Then
                Console.WriteLine("Camera has been removed.")
                Return
            End If
        ElseIf GrabResultStatus.Ok <> e.status Then
            Console.WriteLine("Image was not grabbed successfully.")
        Else
            Dim width = e.parts(0).width
            Dim height = e.parts(0).height
            Dim RangeData = DirectCast(e.parts(0).data, Coord3D()) ' Array of coordinate triples.
            Dim IntensityData = DirectCast(e.parts(1).data, UInt16()) ' Array of 16-bit grey values.
            Dim ConfidenceData = DirectCast(e.parts(2).data, UInt16()) ' Array of 16-bit values.

            ' Retrieve the data for the center pixel.
            Dim x As UInteger = 0.5 * width
            Dim y As UInteger = 0.5 * height
            Dim Coord = RangeData(y * width + x)
            Dim Intensity = IntensityData(y * width + x)
            Dim Confidence = ConfidenceData(y * width + x)
            Console.WriteLine("x={0}, y={1}, z={2}, intensity={3}, confidence = {4}", Coord.x, Coord.y, Coord.z, Intensity, Confidence)
        End If
    End Sub


    Sub Main()

        Using camera As New ToFCamera
            Try
                ' Open a camera, i.e., establish a connection to a camera. 
                camera.OpenFirstCamera()

                ' ToFCamera.OpenFirstCamera() is a shortcut for the following sequence:
                '    Dim cameras As CameraList = ToFCamera.EnumerateCameras()
                '    Dim camInfo As CameraInfo = cameras(0)
                '    camera.Open(camInfo)
                '
                ' How to open a specific camera, e.g., a camera with a certain IP address:
                '
                '   camera.Open(ToFCamera.EnumerateCameras().Find(Function(camInfo) camInfo.IpAddress.Equals("192.168.1.2")))
                '
                '
                ' You can use any property of the CameraInfo class to select a camera, e.g., the serial number or the user-defined name.
                '
                ' Examples:
                '   camera.Open(ToFCamera.EnumerateCameras().Find(Function(camInfo) camInfo.SerialNumber.Equals("21959736")))
                '   camera.Open(ToFCamera.EnumerateCameras().Find(Function(camInfo) camInfo.UserDefinedName.Equals("Left")))


                '   Enable 3D (point cloud) data, intensity data, and confidence data. 
                camera.SetParameterValue("ComponentSelector", "Range")
                camera.SetParameterValue("ComponentEnable", "true")


                ' Range information can be sent either as a 16-bit gray value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
                ' Note: To change the format of an image component, the Component Selector must first be set to the component
                ' you want to configure (see above).
                ' To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
                camera.SetParameterValue("PixelFormat", "Coord3D_ABC32f")

                camera.SetParameterValue("ComponentSelector", "Intensity")
                camera.SetParameterValue("ComponentEnable", "true")

                camera.SetParameterValue("ComponentSelector", "Confidence")
                camera.SetParameterValue("ComponentEnable", "true")

                ' Subscribe to the ImageEvent that will be raised for each image grabbed.
                AddHandler camera.ImageGrabbed, AddressOf ImageGrabbedHandler

                ' Let the camera grab images continuously until either we call StopGrabbing or
                ' the GrabImageEvent handler signals to stop image acquisition.
                camera.StartGrabbing()

                ' In this sample, we want the camera to grab for 10 seconds.
                System.Threading.Thread.Sleep(10000)

                camera.StopGrabbing()

                camera.Close()

            Catch ex As CameraException
                Console.WriteLine("Camera exception occurred: {0}", ex)
                Console.WriteLine("Source: {0}", ex.Source)
                ' After successfully opening the camera, the IsConnected function may be used 
                ' to check if the device is still connected.
                If camera.IsOpen And Not camera.IsConnected Then
                    Console.WriteLine("Camera has been removed.")
                End If
            Catch ex As Exception
                Console.WriteLine("Exception occurred: {0}", ex)

            Finally
                Console.WriteLine("Press any key to exit program.")
                Console.ReadKey()
            End Try

        End Using

    End Sub

End Module
