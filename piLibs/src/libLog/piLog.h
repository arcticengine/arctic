#pragma once


namespace piLibs {

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

#define __WFILE__     WIDEN(__FILE__)

#ifdef WINDOWS
#define __WFUNCTION__ WIDEN(__FUNCTION__)
#else
#define __WFUNCTION__ L"unkown"
#endif


#define LT_ERROR    __WFILE__, __WFUNCTION__, __LINE__, 1
#define LT_WARNING  __WFILE__, __WFUNCTION__, __LINE__, 2
#define LT_ASSUME   __WFILE__, __WFUNCTION__, __LINE__, 3
#define LT_MESSAGE  __WFILE__, __WFUNCTION__, __LINE__, 4
#define LT_DEBUG    __WFILE__, __WFUNCTION__, __LINE__, 5

#define PILOG_TXT 1
#define PILOG_CNS 2



typedef struct
{   
    unsigned int  free_memory_MB;
    unsigned int  total_memory_MB;
    int     number_cpus;
    wchar_t processor[512];
    int     mhz;
    wchar_t os[512];
    wchar_t date[512];
    wchar_t gpuVendor[64];
    wchar_t gpuModel[512];
    unsigned int  total_videomemory_MB;
    int     mScreenResolution[2];
	int     mIntegratedMultitouch;
    int     mNumMonitors;
}piLogStartInfo;


class piLogger
{
public:
	virtual bool Init( const wchar_t *path, const piLogStartInfo *info ) = 0;
    virtual void End( void ) = 0;
	virtual void Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str ) = 0;
};

class piLog
{
public:
	piLog();
	~piLog();

    bool Init( const wchar_t *path, int loggers );
    void End( void );

    void Printf( const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *format, ... );

private:
	int       mNumLoggers;
	piLogger *mLoggers[8];
	int       mMessageCounter;
};


} // namespace piLibs