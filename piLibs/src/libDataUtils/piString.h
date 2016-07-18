#pragma once

namespace piLibs {

// 3/4
class piString
{
public:
    piString();
    ~piString();

    int         Init( unsigned long max);
    int         InitCopy( const piString *ori);
    int         InitCopyW(const wchar_t *ori);
    int         InitCopyS(const char *ori);
    int         InitCopyS(const char *ori, unsigned int len);
    int         InitConcatW(const wchar_t *ori1, const wchar_t *ori2);
    int         InitConcatW(const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3);
    int         WrapW(const wchar_t *ori);
    int         Copy(const piString *ori);
    int         CopyW(const wchar_t *ori);
    int         CopyS(const char *ori);
    bool        IsNull(void) const;
    void        SetNull(void);
    void        End( void );
    int         GetLength( void ) const;
    void        SetLength( unsigned int len);
    void        Reset( void );

    int         AppendWC( wchar_t obj);
    int         AppendAC( char obj);

    int			AppendCAt( wchar_t obj, unsigned int pos);
    int         AppendS( const piString *str);
    int         AppendSW( const wchar_t *str);
    int         AppendSA( const char *str);
    int         AppendPrintf( const wchar_t *format, ...);
    int		    RemoveCAt( unsigned int pos);

    int            Equal( const piString *str) const;
    int            EqualW( const wchar_t *str) const;
    bool           ContainsW( const wchar_t *str) const;
    const wchar_t  GetC(unsigned int n) const;
    const wchar_t *GetS(void) const;
    int            Printf( const wchar_t *format, ...);
    int            ToSInt(  int *res) const;
    int            ToUInt(  unsigned int *res) const;
    int            ToFloat(  float *res) const;

    bool           RemoveOccurrences(const wchar_t what);
    bool           ReplaceOccurrences(const wchar_t what, const wchar_t bywhat);

private:
    int          iReallocate(void);

    wchar_t      *mBuffer;		// 1/2
    unsigned long mMax;			// 1
    unsigned long mNum;		    // 1
};

} // namespace piLibs

