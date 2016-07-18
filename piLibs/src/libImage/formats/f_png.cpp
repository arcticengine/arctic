#include "../../libSystem/piFile.h"
#include "../../libSystem/piTypes.h"
#include "../piImage.h"
#include "tinyPngOut/TinyPngOut.h"

namespace piLibs {


    bool PNG_Save( const piImage *bmp, const wchar_t *name )
{
    const int xres = bmp->GetXRes();
    const int yres = bmp->GetYRes();
    void *data = bmp->GetData();

    const piImage::Format fmt = bmp->GetFormat();

    if( fmt==piImage::FORMAT_I_RGB )
    {
        piFile fo;
        if (!fo.Open(name, L"wb"))
            return false;

        struct TinyPngOut pngout;
        if( TinyPngOut_init(&pngout, &fo, xres, yres) != TINYPNGOUT_OK)
            return false;

        if (TinyPngOut_write(&pngout, (unsigned char*)data, xres * yres) != TINYPNGOUT_OK)
            return false;

        if (TinyPngOut_write(&pngout, nullptr, 0) != TINYPNGOUT_DONE)
            return false;

        fo.Close();

        return true;
    }
    else if (fmt == piImage::FORMAT_I_RGBA)
    {
        piFile fo;
        if (!fo.Open(name, L"wb"))
            return false;

        struct TinyPngOut pngout;
        if (TinyPngOut_init(&pngout, &fo, xres, yres) != TINYPNGOUT_OK)
            return false;

        const int num = xres * yres;
        for( int i=0; i<num; i++ )
        {
            if (TinyPngOut_write(&pngout, ((unsigned char*)data) + 4*i, 1) != TINYPNGOUT_OK)
                return false;
        }

        if (TinyPngOut_write(&pngout, nullptr, 0) != TINYPNGOUT_DONE)
            return false;

        fo.Close();

        return true;
    }

    if (bmp->GetFormat() == piImage::FORMAT_I_RGBA)
        return 0;//BMP_Salva32To24( name, (unsigned char*)data, xres, yres );

	return false;
}

}