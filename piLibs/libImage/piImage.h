#pragma once

namespace piLibs {

class piImage
{
public:
    typedef enum
    {
        TYPE_1D = 1,
        TYPE_2D = 2,
        TYPE_3D = 3,
        TYPE_CUBE = 4
    }Type;

    typedef enum
    {
        FORMAT_I_INDEXED = 1,
        FORMAT_I_GREY    = 2,
        FORMAT_I_15BIT   = 3,
        FORMAT_I_16BIT   = 4,
        FORMAT_I_RG      = 5,
        FORMAT_I_RGB     = 6,
        FORMAT_I_RGBA    = 7,
        FORMAT_F_GREY    = 8,
        FORMAT_F_RG      = 9,
        FORMAT_F_RGB     = 10,
        FORMAT_F_RGBA    = 11
    }Format;

    void Init( void );
    bool Make(Type type, int xres, int yres, int zres, Format format);
    void Free( void );
    void Create(Type type, int xres, int yres, int zres, Format format, void *buffer);
    bool Load(const wchar_t *name);
    bool Save(const wchar_t *name) const;
    int  Clone( const piImage *b );
    int  GetBpp( void ) const;
    void Replace(piImage *img);

    const Format GetFormat( void ) const;
    const Type   GetType( void ) const;
    const int    GetXRes( void ) const;
    const int    GetYRes( void ) const;
    const int    GetZRes( void ) const;
    void  *      GetData( void );
    void  *      GetData(void) const;
    unsigned char * GetPallette(void);
    unsigned char * GetPallette(void) const;

    // move this to processing
    int  Index2Color(  unsigned char idx );
    bool Convert(  Format newformat, bool swapRB);
    void SwapRB(void);
    int  RGB2YUV( void );
    bool ExtractComponent( piImage *dst,  int comp ); // comp: 2=red 1=green 0=blue
    bool Compatibles( const piImage *a );

    void SampleBilinear( float x, float y, float *data ) const;




private:
    void	        *mData;
    unsigned char   *mPallette;
    unsigned int	mXres, mYres, mZres;
    Format   mFormat;
    Type     mType;
};

} // namespace piLibs