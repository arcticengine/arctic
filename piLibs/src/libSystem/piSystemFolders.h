#pragma once

#include "piTypes.h"

namespace piLibs {

class piSystemFolders
{
public:
    static wchar_t *GetPictures(void);
    static void     Free( wchar_t *str );
};

} // namespace piLibs