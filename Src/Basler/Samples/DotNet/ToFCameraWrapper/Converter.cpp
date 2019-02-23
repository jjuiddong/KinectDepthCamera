#include "stdafx.h"
#include "Converter.h"

using namespace ToFCameraWrapper;
using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Imaging;

namespace ToFUtil
{

    Bitmap^ Converter::PartToBitmap( ToFCameraWrapper::BufferPart^ part)
    {
        if ( part->dataFormat == PixelFormats::Confidence16 || part->dataFormat == PixelFormats::Coord3D_C16 || part->dataFormat == PixelFormats::Mono16 )
        {
            Bitmap^ bitmap = gcnew Bitmap( part->width, part->height, PixelFormat::Format8bppIndexed);
            ColorPalette^ palette = bitmap->Palette;
            if ( greyScalePaletteEntries == nullptr  )
            {
                greyScalePaletteEntries = gcnew array<Color>(256);
                for ( int i = 0; i < 255; ++i )
                {
                    greyScalePaletteEntries[i] = Color::FromArgb(i, i, i);
                }
            }
            greyScalePaletteEntries->CopyTo(palette->Entries, 0);
            bitmap->Palette = palette; // assignment is required to "activate" pallette

            // Pin Bitmap  part->data and source  part->data
            BitmapData^ bitmapData = bitmap->LockBits( Drawing::Rectangle(0, 0, bitmap->Width, bitmap->Height), ImageLockMode::WriteOnly, bitmap->PixelFormat); 
            pin_ptr<uint16_t> src = &(dynamic_cast<array<uint16_t>^>(part->data))[0];
            for ( int y = 0; y < (int) part->height; ++y )
            {
                uint8_t* pDst = (uint8_t*) (void*) bitmapData->Scan0 + y * bitmapData->Stride;
                uint16_t* pSrc = (uint16_t*) & src[y * part->width];
                for ( int x = 0; x < (int) part->width; ++x, pSrc++, pDst++ )
                {
                    *pDst = *pSrc >> 8;
                }
            }
            bitmap->UnlockBits(bitmapData);
            return bitmap;
        }
        else if ( part->dataFormat == PixelFormats::RGB8 )
        {
            Bitmap^ bitmap = gcnew Bitmap(part->width, part->height, PixelFormat::Format32bppRgb);
            BitmapData^ bitmapData = bitmap->LockBits( Drawing::Rectangle(0, 0, bitmap->Width, bitmap->Height), ImageLockMode::WriteOnly, bitmap->PixelFormat); 
            pin_ptr<RGB8> src = &(dynamic_cast<array<RGB8>^>( part->data))[0];
            for ( int y = 0; y < (int) part->height; ++y )
            {
                uint8_t* pDst = (uint8_t*) (void*) bitmapData->Scan0 + y * bitmapData->Stride;
                RGB8* pSrc = (RGB8*) &src[y * part->width];
                for ( int x = 0; x < (int) part->width; ++x, pSrc++, pDst += 4 )
                {
                    pDst[0] = pSrc->b;
                    pDst[1] = pSrc->g;
                    pDst[2] = pSrc->r;
                    pDst[3] = 255;
                }
            }
            bitmap->UnlockBits(bitmapData);
            return bitmap;
        }
        else
        {
            throw gcnew Exception(String::Format("Bitmap conversion doesn't support pixel format {0}", part->dataFormat));
        }
    }

}