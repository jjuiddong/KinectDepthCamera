#pragma once
#include "ToFCameraWrapper.h"


namespace ToFUtil
{
    public ref class Converter
    {
    public:
        static System::Drawing::Bitmap^ PartToBitmap( ToFCameraWrapper::BufferPart^ part);
    private:
        static array<System::Drawing::Color>^ greyScalePaletteEntries = nullptr;
    };
}

