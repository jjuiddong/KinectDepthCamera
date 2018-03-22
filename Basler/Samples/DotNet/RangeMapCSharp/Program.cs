/*
 * This sample illustrates how to grab data from a ToF camera and how to visualize range maps, confidence maps, and 
 * intensity images using C#.
 *
 * Access to ToF cameras is provided by a GenICam-compliant GenTL Producer. A GenTL Producer is a dynamic library
 * implementing a standardized C interface for accessing the camera.
 * 
 * The software interacting with the GentL Producer is called a GenTL Consumer. Using this terminology,
 * this sample is a GenTL Consumer, too.
 * As part of the suite of sample programs, Basler provides the ConsumerImplHelper C++ template library that serves as 
 * an implementation helper for GenTL consumers.
 * 
 * The ToFCameraWrapper assembly partially wraps this native ConsumerImplHelper library to allow .NET languages to access the 
 * GenTL Producer.
 *
 * For more details about GenICam, GenTL, and GenApi, refer to the http://www.emva.org/standards-technology/genicam/ website.
 * The GenICam GenApi standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_Standard_v2_0.pdf
 * The GenTL standard document can be downloaded from http://www.emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf
 *
 * 
*/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RangeMapCSharp
{
    static class Program
    {
        /// The main entry point for the application.
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}
