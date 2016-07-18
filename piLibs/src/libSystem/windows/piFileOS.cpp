#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <errno.h>

#include "../piTypes.h"
#include "../piFile.h"

namespace piLibs {

bool piFile::Open(const wchar_t *name, const wchar_t *mode)
{
    FILE *fp = nullptr;
    if( _wfopen_s(&fp, name, mode) != 0 )
        return false;

    mInternal = (void*)fp;
    return true;
}

bool piFile::Seek(uint64 pos, SeekMode mode)
{
    int cmode = 0;
    if (mode == CURRENT) cmode = SEEK_CUR;
    if (mode == END) cmode = SEEK_END;
    if (mode == SET) cmode = SEEK_SET;

	return ( _fseeki64( (FILE*)mInternal, pos, cmode ) == 0 );
}

uint64 piFile::Tell(void)
{
    return _ftelli64((FILE*)mInternal);
}

void piFile::Close(void)
{
    fclose((FILE*)mInternal);
}

bool piFile::Exists(const wchar_t *name)
{
    FILE  *fp = _wfopen(name, L"rb");
    if (!fp)
        return false;
    fclose(fp);
    return true;
}

bool piFile::DirectoryExists(const wchar_t *dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


bool piFile::HaveWriteAccess(const wchar_t *name)
{
    if (_waccess(name, 6) == -1)
    {
        return errno == ENOENT;
    }

    return true;
}

bool piFile::Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists)
{
    if( CopyFile( src, dst, failIfexists)==0 )
    {
        int res = GetLastError();
        return false;
    }
    return true;
}

uint64 piFile::GetLength(void)
{
    uint64 p = ftell( (FILE*)mInternal );
    _fseeki64((FILE*)mInternal, 0, SEEK_END);
    uint64 l = ftell((FILE*)mInternal);
    _fseeki64((FILE*)mInternal, p, SEEK_SET);
    return l;
}


bool piFile::DirectoryCreate( const wchar_t *name, bool failOnExists )
{
    if( ::CreateDirectory( name, NULL )==0 )
    {
        int res = GetLastError();
        if (res == ERROR_ALREADY_EXISTS )
            return !failOnExists;
        if (res == ERROR_PATH_NOT_FOUND )
            return false;
        return false;
    }
    return true;
}

}