The sample code provided demonstrates how the Point Cloud Library (PCL) can be 
used to filter, transform, merge, or manipulate the point clouds grabbed with  
Basler ToF cameras.

For this purpose, the grab results of the ToF camera are converted into 
pcl::PointCloud<pcl::PointXYZRGB> objects which are the data structures 
provided by the PCL for representing point cloud data.

The PCL point cloud viewer class is used to render the resulting
point clouds.

Windows 
========

The samples have been built and tested against PCL 1.8.0 built with Microsoft
Visual Studio 2015.

You can either build PCL 1.8.0 using Microsoft Visual Studio or install it using
the all-in-one installer provided by http://unanancyowen.com/?p=2009&lang=en

For the project settings of the samples it has been assumed that the 
all-on-one installer of PCL 1.8.0 has installed the PCL and their dependencies
at the default location, the Program Files resp. Program Files (x86) folders.

The PCL visualization library requires the OpenNI2.dll file. The the OpenNI2 SDK
is part of the PCL all-in-one installer. After installing the OpenNI2 SDK, 
OpenNI2.dll is located in the %ProgramFiles%\OpenNI2\Redist folder. Either 
adjust your PATH variable accordingly or copy the DLL to a location where the
linker will find it.

Please make sure you always use a PCL version that has been built with
the same compiler version as the samples resp. your application.

Linux
======

The samples have been tested on Ubuntu 15.04 and Ubuntu 16.04 using PCL 1.7.2
and 1.8.0. Basler recommends not to use the pre-built PCL 1.7.2 from the Ubuntu
package repositories. Instead, you should use PCL 1.8.0 or newer built from
sources that can be downloaded from 
https://github.com/PointCloudLibrary/pcl/releases. See the troubleshooting
section below for more details.

To build the samples, navigate to the individual folders and run cmake first to
generate a makefile suitable for your system. When cmake has successfully
created a makefile, build the sample by running make.

Example:
  cd ShowPointCloud
  cmake .
  make
  


Troubleshooting
---------------

Symptom: The ConvertAndFilter sample crashes with a segfault.
Reason: On Ubuntu 15.04, using the PCL 1.7.2 from the Ubuntu package repository,
an application crashes immediately once the VoxelGridFilter.h header is
included. 
Solution: Download the sources of PCL 1.8.0 or newer and build and install the
PCL. Delete the CMakeCache.txt file in the sample folder and rerun cmake in
order to use the PCL built from sources.

Symptom: On Ubuntu 16.04, the PCL samples fail to build due to a missing
libvtkproj4.so library.
Reason: The PCL 1.7.2 from the Ubuntu 16.04 package repository has a bug. The
cmake find-module for the PCL creates an incorrect dependency to libvtkproj4.so,
which is not present on the system. 
Solution: Either build and install the PCL 1.8.0 or newer from sources as
described above or provide a symbolic link to the missing library by entering
the following command in a terminal window:
    sudo ln /usr/lib/libvtkproj4.so.5.10 /usr/lib/libvtkproj4.so
