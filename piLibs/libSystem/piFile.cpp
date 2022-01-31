#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include "piTypes.h"
#include "piFile.h"

namespace piLibs {

piFile::piFile() {
  mInternal = nullptr;
}

piFile::~piFile() {
}


//-------------------------------------------------------------------------------------------


char *piFile::ReadString(char *buffer, int num) {
  return fgets(buffer, num);
}

void piFile::Prints(const wchar_t *str) {
  fwprintf((FILE*)mInternal, L"%s", str);
}

void piFile::Printf(const wchar_t *format, ...) {
  va_list arglist;
  va_start(arglist, format);
  vfwprintf((FILE*)mInternal, format, arglist);
  //int maxLen = pivscwprintf(format, arglist) + 1;
  //wchar_t *tmpstr = (wchar_t*)_malloca(maxLen*sizeof(wchar_t));
  //if (!tmpstr) return;

  //pivwsprintf(tmpstr, maxLen, format, arglist);

  va_end(arglist);

  //fwprintf((FILE*)mInternal, str);
}

//-------------------------------------------------------------------------------------------


/*
   int  FILE_Copy(char *dst, char *ori)
   {
   FILE    *fo;
   FILE    *fd;
   long    l;
   char    *tmp;

   fo = FILE_Open(ori, "rb");
   if (!fo)
   return(0);

   fd = FILE_Open(dst, "wb");
   if (!fd)
   {
   fclose(fo);
   return(0);
   }

   tmp = (char*)malloc(65536);
   if (!tmp)
   {
   fclose(fo);
   fclose(fd);
   return(0);
   }

   l = FILE_GetLength(fo);
   long i, n;
   n = l >> 16;

   for (i=0; i<n; i++)
   {
   Read(tmp, 65536, fo);
   Write(tmp, 1, 65536, fd);
   }

   n = l & 65535;
   Read( tmp, n, fo);
   Write(tmp, 1, n, fd);


   fclose(fo);
   fclose(fd);

   free(tmp);
   return(1);

   }*/
}
