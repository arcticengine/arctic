// The MIT License(MIT)
//
// Copyright 2015 - 2017 Inigo Quilez
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "engine/easy_sprite_instance.h"

#include <memory>

#include "engine/arctic_platform.h"
#include "engine/rgb.h"
#include "engine/rgba.h"

namespace arctic {
namespace easy {

SpriteInstance::SpriteInstance(Si32 width, Si32 height)
  : width_(width)
    , height_(height)
    , data_(width * height * sizeof(Rgba)) {
    }

}  // namespace easy

#pragma pack(1)
typedef struct {
  Ui8 IDFieldLength;
  Ui8 ColorMapType;
  Ui8 ImageType;
  Ui16 ColorMapOrigin;
  Ui16 ColorMapLength;
  Ui8 ColorMapEntrySize;
  Ui16 ImageXOrigin;
  Ui16 ImageYOrigin;
  Ui16 xres;
  Ui16 yres;
  Ui8 bpp;
  Ui8 ImageDescriptor;
}TGAHEADER;
#pragma pack()

/*
   static int U_Load(piImage *bmp, FILE * fp) {
   const int bpp = bmp->GetBpp();
   const int l = bmp->GetXRes()*bmp->GetYRes()*bpp;

   if (!fread(bmp->GetData(), 1, l, fp))
   return(0);

   return(1);
   }

   static int C_Load(piImage * bmp, FILE * fp) {
   unsigned char   aux[4];
   unsigned char   chunkheader;
   int             i, counter;

   const int bpp = bmp->GetBpp();

   int pixelcount = bmp->GetXRes()*bmp->GetYRes();
   int currentpixel = 0;
   unsigned char *buffer = (unsigned char*)bmp->GetData();


   while (currentpixel<pixelcount) {
   if (!fread(&chunkheader, 1, 1, fp))
   return(0);

   counter = 1 + (chunkheader & 0x7F);

   if (chunkheader<128) {
   for (i = 0; i<counter; i++) {
   if (!fread(buffer, 1, bpp, fp))
   return(0);
   buffer += bpp;
   }
   } else {
   if (!fread(aux, 1, bpp, fp))
   return(0);

   for (i = 0; i<counter; i++) {
   buffer[0] = aux[0];
   if (bpp>1) buffer[1] = aux[1];
   if (bpp>2) buffer[2] = aux[2];
   if (bpp>3) buffer[3] = aux[3];
   buffer += bpp;
   }
   }

   currentpixel += counter;
   }

   return(1);
   }*/

std::shared_ptr<easy::SpriteInstance> LoadTga(const Ui8 *data,
    const Si64 size) {
  std::shared_ptr<easy::SpriteInstance> sprite;
  Check(size >= sizeof(TGAHEADER), "Error in LoadTga, size is too small.");
  const TGAHEADER *tga = static_cast<const TGAHEADER*>(
      static_cast<const void*>(data));
#ifdef BIGENDIAN
  tga.xres = ((tga.xres & 255) << 8) | (tga.xres >> 8);
  tga.yres = ((tga.yres & 255) << 8) | (tga.yres >> 8);
#endif  // BIGENDIAN
  Check(tga->xres >= 2, "Error in LoadTga, tga.xres is too small.");
  Check(tga->yres >= 2, "Error in LoadTga, tga.yres is too small.");
  Check((tga->bpp == 24) || (tga->bpp == 32) || (tga->bpp == 16)
      || (tga->bpp == 8), "Error in LoadTga, unsupported bpp.");
  const Ui8 *p = data + sizeof(TGAHEADER) + tga->IDFieldLength;
  bool is_origin_upper_left = !!(tga->ImageDescriptor & (1 << 5));
  switch (tga->ImageType) {
    case 2:  // uncommpressed rgb
      if (tga->bpp == 24) {
        sprite.reset(new easy::SpriteInstance(tga->xres, tga->yres));
        Si64 l = sprite->width() * sprite->height() * sizeof(Rgb);
        Check(p + l <= data + size,
            "Error in LoadTga, unexpected end of file.");
        Ui8 *to_line = sprite->RawData();
        Si64 from_line_size = sprite->width() * sizeof(Rgb);
        const Ui8 *from_line = p +
          (is_origin_upper_left ? tga->yres - 1 : 0) * from_line_size;
        const Si64 from_line_step =
          (is_origin_upper_left ? -from_line_size : from_line_size);
        for (Si64 y = 0; y < tga->yres; ++y) {
          const Ui8 *from = from_line + y * from_line_step;
          for (Si64 x = 0; x < tga->xres; ++x) {
            *(to_line + 0) = *(from + 2);
            *(to_line + 1) = *(from + 1);
            *(to_line + 2) = *(from + 0);
            *(to_line + 3) = 255;
            from += sizeof(Rgb);
            to_line += sizeof(Rgba);
          }
        }
        return sprite;
      } else if (tga->bpp == 32) {
        sprite.reset(new easy::SpriteInstance(tga->xres, tga->yres));
        Si64 l = sprite->width() * sprite->height() * sizeof(Rgba);
        Check(p + l <= data + size,
            "Error in LoadTga, unexpected end of file.");
        Ui8 *to_line = sprite->RawData();
        Ui64 from_line_size = sprite->width() * sizeof(Rgba);
        const Ui8 *from_line = p +
          (is_origin_upper_left ? tga->yres - 1 : 0) * from_line_size;
        const Si64 from_line_step =
          (is_origin_upper_left ? -from_line_size : from_line_size);
        for (Si64 y = 0; y < tga->yres; ++y) {
          const Ui8 *from = from_line + y * from_line_step;
          for (Si64 x = 0; x < tga->xres; ++x) {
            *to_line = *from;
            *(to_line + 0) = *(from + 2);
            *(to_line + 1) = *(from + 1);
            *(to_line + 2) = *(from + 0);
            *(to_line + 3) = *(from + 3);
            from += sizeof(Rgba);
            to_line += sizeof(Rgba);
          }
        }
        return sprite;
      }
      Fatal("Error in LoadTga, unexpected bpp.");
      break;

      /*
         case 10:  // compressed rgb
         if (tga.bpp == 24)
         format = piImage::FORMAT_I_RGB;
         else if (tga.bpp == 32)
         format = piImage::FORMAT_I_RGBA;
         else
         break;

         if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, format))
         break;
         if (!C_Load(bmp, fp))
         break;
         if (!piImage_Flip(bmp))
         break;
         res = 1;
         break;

         case 3:  // uncompressed grey
         if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, piImage::FORMAT_I_GREY))
         break;
         if (!U_Load(bmp, fp))
         break;
         if (!piImage_Flip(bmp))
         break;
         res = 1;
         break;

         case 11:  // compressed grey
         if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, piImage::FORMAT_I_GREY))
         break;
         if (!C_Load(bmp, fp))
         break;
         if (!piImage_Flip(bmp))
         break;
         res = 1;
         break;
         */
    case 1:  // uncompressed pallete
      break;

    case 9:  // compressed pallete
      break;
  }
  return sprite;
}


}  // namespace arctic
