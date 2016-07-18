#pragma once

#include "piTypes.h"

namespace piLibs {

class piFile
{
public:
    piFile();
    ~piFile();

    static bool  DirectoryCreate(const wchar_t *name, bool failOnExists);
    static bool  DirectoryExists(const wchar_t *dirName_in);
    static bool  Exists(const wchar_t *name);
    static bool  HaveWriteAccess( const wchar_t *name );
    static bool  Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists);

    typedef enum
    {
        CURRENT = 0,
        END = 1,
        SET = 2
    }SeekMode;

    bool         Open(const wchar_t *name, const wchar_t *mode);
    bool         Seek(uint64 pos, SeekMode mode);
    uint64       Tell(void);
    void         Close( void );
    uint64       GetLength( void );
    
    //----------------------

    char *       ReadString(char *buffer, int num);
    void         Prints(const wchar_t *str);
    void         Printf(const wchar_t *format, ...);

    //-----------------------

    void         Read(void *dst, uint64 amount);
    uint8        ReadUInt8( void );
    uint16       ReadUInt16( void );
    uint32       ReadUInt32( void );
    uint64       ReadUInt64( void );
    float        ReadFloat( void );
    void         ReadFloatarray(float *dst, int num, int size, uint64 amount);
    void         ReadFloatarray2(float *dst, uint64 amount);
    void         ReadDoublearray2(double *dst, uint64 amount);
    void         ReadUInt64array(uint64 *dst, uint64 amount);
    void         ReadUInt32array(uint32 *dst, uint64 amount);
    void         ReadUInt32array2(uint32 *dst, uint64 amount, int size, int stride);
    void	     ReadUInt16array(uint16 *dst, uint64 amount);
    void	     ReadUInt8array(uint8  *dst, uint64 amount);

    int          Write(const void *dst, uint64 amount);
    void		 WriteUInt8(uint8 x);
    void		 WriteUInt16(uint16 x);
    void		 WriteUInt32(uint32 x);
    void		 WriteUInt64(uint64 x);
    void		 WriteFloat(float x);
    void		 WriteUInt8array(uint8 *dst, uint64 amount);
    void		 WriteUInt16array(uint16 *dst, uint64 amount);
    void		 WriteUInt32array(uint32 *dst, uint64 amount);
    void		 WriteUInt64array(uint64 *dst, uint64 amount);
    void		 WriteFloatarray2(float *ori, uint64 amout);
    void		 WriteDoublearray2(double *ori, uint64 amout);

private:
    void *mInternal;

};

} // namespace piLibs