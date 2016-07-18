#include <windows.h>
#include <shlobj.h>

#include "../piSystemFolders.h"

namespace piLibs {

wchar_t *piSystemFolders::GetPictures(void)
{
    wchar_t *res;
    HRESULT h = ::SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &res);
    return res;
}

void piSystemFolders::Free(wchar_t *str)
{
    CoTaskMemFree(str);
}

}

