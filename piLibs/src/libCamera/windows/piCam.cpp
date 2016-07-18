#include <malloc.h>

#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")

#include "ds/baseclasses/streams.h"

#include "../piCam.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

typedef struct
{
    wchar_t            pName[512];
    int                pCaps[3*64];
    int                pNumCaps;
    IMoniker           *pMoniker;
}MyDeviceInfo;


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CDumpInputPin;
class CDump;
class CDumpFilter;

class CDumpFilter : public CBaseFilter
{
public:
    CDumpFilter(CDump *pDump, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr);
    CBasePin * GetPin(int n);
    int        GetPinCount();
private:
    CDump * const m_pDump;
};


//  Pin object
class CDumpInputPin : public CRenderedInputPin
{
public:
    CDumpInputPin(CDump *pDump, LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

    // Write detailed information about this sample to a file
    HRESULT WriteStringInfo(IMediaSample *pSample);

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

    // Break connection
    HRESULT BreakConnect();

    // Track NewSegment
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

private:
    CDump    * const m_pDump;           // Main renderer object
    CCritSec * const m_pReceiveLock;    // Sample critical section
    REFERENCE_TIME m_tLast;             // Last sample receive time
};


//  CDump object which has filter and pin members

class CDump : public CUnknown, public IFileSinkFilter
{

    friend class CDumpFilter;
    friend class CDumpInputPin;

public:

    DECLARE_IUNKNOWN

    CDump(LPUNKNOWN pUnk, HRESULT *phr, int xres, int yres, unsigned char *buffer );
    ~CDump();

    //static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Write raw data stream to a file
    HRESULT Write(PBYTE pbData, LONG lDataLength);

    CDumpFilter   *GetFilter();
    bool           IsNewFrame(void);
    void           SetConsumed( void );

    // Implements the IFileSinkFilter interface
    STDMETHODIMP SetFileName(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt);

private:

    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

private:

    CDumpFilter   *m_pFilter;       // Methods for filter interfaces
    CDumpInputPin *m_pPin;          // A simple rendered input pin
    CCritSec m_Lock;                // Main renderer critical section
    CCritSec m_ReceiveLock;         // Sublock for received samples
    CPosPassThru *m_pPosition;      // Renderer position controls
    unsigned char *mData;
    bool           mNewData;

};

static GUID CLSID_Dump = { 0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5 };


const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudDump =
{
    &CLSID_Dump,                // Filter CLSID
    L"Dump",                    // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};

CDumpFilter::CDumpFilter(CDump *pDump, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr) : CBaseFilter(NAME("CDumpFilter"), pUnk, pLock, CLSID_Dump),
    m_pDump(pDump)
{
}

CBasePin * CDumpFilter::GetPin(int n)
{
    if (n == 0) return m_pDump->m_pPin;
    return NULL;
}

int CDumpFilter::GetPinCount()
{
    return 1;
}

//  Definition of CDumpInputPin
CDumpInputPin::CDumpInputPin(CDump *pDump, LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr) :  CRenderedInputPin(NAME("CDumpInputPin"),
    pFilter,
    pLock,
    phr,
    L"Input"),
    m_pReceiveLock(pReceiveLock),
    m_pDump(pDump),
    m_tLast(0)
{
}


// Check if the pin can support this specific proposed type and format
HRESULT CDumpInputPin::CheckMediaType(const CMediaType *)
{
    return S_OK;
}

// Break a connection
HRESULT CDumpInputPin::BreakConnect()
{
    if (m_pDump->m_pPosition != NULL)
        m_pDump->m_pPosition->ForceRefresh();

    return CRenderedInputPin::BreakConnect();
}


// We don't hold up source threads on Receive
STDMETHODIMP CDumpInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

// Do something with this media sample
STDMETHODIMP CDumpInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample, E_POINTER);

    CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;

    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);


    m_tLast = tStart;

    // Copy the data to the file

    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr))
        return hr;

    int len = pSample->GetActualDataLength();

    return m_pDump->Write(pbData, len);
}

STDMETHODIMP CDumpInputPin::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

}

// Called when we are seeked
STDMETHODIMP CDumpInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    m_tLast = 0;
    return S_OK;
}

CDump::CDump(LPUNKNOWN pUnk, HRESULT *phr, int xres, int yres, unsigned char *buffer) : CUnknown(NAME("CDump"), pUnk),
        m_pFilter(NULL),
        m_pPin(NULL),
        m_pPosition(NULL)
{
    ASSERT(phr);

    m_pFilter = new CDumpFilter(this, GetOwner(), &m_Lock, phr);
    if (m_pFilter == NULL) 
    {
        if (phr) *phr = E_OUTOFMEMORY;
        return;
    }

    m_pPin = new CDumpInputPin(this, GetOwner(), m_pFilter, &m_Lock, &m_ReceiveLock, phr);
    if (m_pPin == NULL) 
    {
        if (phr) *phr = E_OUTOFMEMORY;
        return;
    }

    mData = buffer;
    mNewData = false;
}

bool CDump::IsNewFrame( void )
{
    return mNewData;
}
void CDump::SetConsumed(void)
{
    mNewData = false;
}


// Implemented for IFileSinkFilter support
STDMETHODIMP CDump::SetFileName(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
{
    return S_OK;
}

// Implemented for IFileSinkFilter support
STDMETHODIMP CDump::GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;

    const wchar_t *m_pFileName = L"piFrameGrabber";
    if (m_pFileName != NULL)
    {
        size_t len = 1 + lstrlenW(m_pFileName);
        *ppszFileName = (LPOLESTR) QzTaskMemAlloc(sizeof(WCHAR)* (len));

        if (*ppszFileName != NULL)
        {
            HRESULT hr = StringCchCopyW(*ppszFileName, len, m_pFileName);
        }
    }

    if (pmt)
    {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->majortype = MEDIATYPE_NULL;
        pmt->subtype = MEDIASUBTYPE_NULL;
    }

    return S_OK;

}


CDump::~CDump()
{
    delete m_pPin;
    delete m_pFilter;
    delete m_pPosition;
}

//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CDump::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv, E_POINTER);
    CAutoLock lock(&m_Lock);

    if (riid == IID_IFileSinkFilter)
        return GetInterface((IFileSinkFilter *) this, ppv);

    if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist)
        return m_pFilter->NonDelegatingQueryInterface(riid, ppv);

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) 
    {
        if (m_pPosition == NULL)
        {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("Dump Pass Through"),
                (IUnknown *)GetOwner(),
                (HRESULT *)&hr, m_pPin);
            if (m_pPosition == NULL)
                return E_OUTOFMEMORY;

            if (FAILED(hr))
            {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }

        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface


CDumpFilter *CDump::GetFilter()
{
    return m_pFilter;
}

// Write raw data to the file
//
HRESULT CDump::Write(PBYTE pbData, LONG lDataLength)
{
    mNewData = true;
    memcpy( mData, pbData, lDataLength );
    return S_OK;
}

//--------------------------------------------------------------------------------

class piCameraWin : public piCamera
{
public:
    bool Start( void );
    bool Stop( void );
    bool IsNewFrame( void );
    void *GetBuffer( void );

    friend class piCameraMgrWin;

private:
    piCameraWin();
    ~piCameraWin();

    bool Init( const MyDeviceInfo *dev, int capID, int *xres, int *yres, int *rate );
    void End( void );

private:
    IFilterGraph2         *m_pGraph;
    ICaptureGraphBuilder2 *m_pBuilder;
    IMediaControl * m_pMC;
    CDump          *mGrabber;
    int             mXres;
    int             mYres;
    unsigned char  *mData;
};

piCameraWin::piCameraWin():piCamera()
{
    mData = nullptr;
    mGrabber = nullptr;
    m_pGraph = nullptr;
    m_pBuilder = nullptr;
    m_pMC = nullptr;
}

piCameraWin::~piCameraWin()
{
}


bool piCameraWin::Init( const MyDeviceInfo *dev, int capID, int *xres, int *yres, int *rate )
{
    HRESULT hr;

    // create the capture graph builder
    hr = CoCreateInstance( CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&m_pBuilder);
    if (FAILED(hr))
        return false;

    // create the filter graph
    hr = CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&m_pGraph);
    if (FAILED(hr))
        return false;

    // attach the filter graph to the builder
    hr = m_pBuilder->SetFiltergraph( m_pGraph );
    if (FAILED(hr))
        return false;

    // get source filter (camera)
    IBaseFilter *pSrcF;
    hr = dev->pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrcF);
    if (FAILED(hr))
        return false;

    // Add Capture filter to our graph.
    hr = m_pGraph->AddFilter(pSrcF, L"Video Capture");
    if (FAILED(hr))
        return false;

    // configure capturer
    IAMStreamConfig* pVSC;
    hr = m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrcF, IID_IAMStreamConfig, (void **)&pVSC);
    if (FAILED(hr))
        return false;

    VIDEO_STREAM_CONFIG_CAPS scc;
    AM_MEDIA_TYPE            *pmt;
    pVSC->GetStreamCaps(capID, &pmt, (BYTE*)&scc);
    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;
    xres[0] = abs( pVih->bmiHeader.biWidth );
    yres[0] = abs( pVih->bmiHeader.biHeight );
    rate[0] = (int)( (__int64)10000000 / pVih->AvgTimePerFrame );
    mXres = xres[0];
    mYres = yres[0];
    pVSC->SetFormat(pmt);

    pVSC->Release();

    // allocate buffer
    mData = (unsigned char*)malloc( mXres*mYres*3 );
    if( !mData )
        return false;

    // create renderer (frame grabber)
    mGrabber = new CDump(NULL, &hr, mXres, mYres, mData);
    IBaseFilter *pRender = mGrabber->GetFilter();
    
    // add grabber to the graph
    m_pGraph->AddFilter(pRender, L"Sample Grabber");

    // connect it
    hr = m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrcF, NULL, pRender);
    if (FAILED(hr))
        return false;

    // create media controller
    hr = m_pGraph->QueryInterface(IID_IMediaControl, (LPVOID *)&m_pMC);
    if (FAILED(hr))
        return false;


    return true;
}

void piCameraWin::End( void )
{
    if (mData != nullptr) free(mData );

    if (mGrabber != nullptr) delete( mGrabber );
    if( m_pMC)              m_pMC->Release();
    if( m_pGraph)           m_pGraph->Release();
    if( m_pBuilder)         m_pBuilder->Release();

    CoUninitialize();
}

bool piCameraWin::Start(void)
{
    HRESULT hr = m_pMC->Run();
    if (FAILED(hr))
        return false;
    return true;
}
bool piCameraWin::Stop(void)
{
    m_pMC->Stop();
    return true;
}
bool piCameraWin::IsNewFrame(void)
{
    return mGrabber->IsNewFrame();
}

void * piCameraWin::GetBuffer(void)
{
    mGrabber->SetConsumed();
    return mData;
}



//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class piCameraMgrWin : public piCameraMgr
{
public:
    piCameraMgrWin();
    ~piCameraMgrWin();

    bool      Init( void );
    void      End( void );
    int       GetNumDevices( void );
    bool      GetDeviceInfo( int id, wchar_t **name, int *numcaps, int **caps );
    piCamera *CreateCamera( int deviceID, int capID, int *xres, int *yres, int *rate );
    void      DestroyCamera( piCamera *cam );

private:
    MyDeviceInfo mDevices[256];
    int          mNumCameras;
    bool         mEnabled;
};

piCameraMgrWin::piCameraMgrWin()
{
}

piCameraMgrWin::~piCameraMgrWin()
{
}

static bool iGetCaps( IMoniker *moniker, int *res, int maxres, int *resnumcaps )
{
    HRESULT         hr;
    IBaseFilter     *pSrc;
    IAMStreamConfig *pVSC;
    int             numcaps, piSize;

    // get source filter (camera)
    hr = moniker->BindToObject( 0,0,IID_IBaseFilter, (void**)&pSrc );
    if (FAILED(hr))
        return false;

    ICaptureGraphBuilder2 *pBuilder;
    hr = CoCreateInstance (CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pBuilder);
    if (FAILED(hr))
    {
        pSrc->Release();
        return false;
    }
    hr = pBuilder->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrc, IID_IAMStreamConfig, (void **)&pVSC);
    if (FAILED(hr))
    {
        pBuilder->Release();
        pSrc->Release();
        return false;
    }

    hr = pVSC->GetNumberOfCapabilities(&numcaps, &piSize);
    if (FAILED(hr))
    {
        pSrc->Release();
        return false;
    }

    if( numcaps>maxres ) numcaps = maxres;
    resnumcaps[0] = numcaps;
    for( int i = 0; i < numcaps; i++ )
    {
        VIDEO_STREAM_CONFIG_CAPS scc;
        AM_MEDIA_TYPE            *pmt = NULL;

        pVSC->GetStreamCaps( i, &pmt, reinterpret_cast<BYTE*>(&scc) );

        VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);

        res[3*i+0] = pVih->bmiHeader.biWidth;
        res[3*i+1] = pVih->bmiHeader.biHeight;
        res[3*i+2] = (int)( (__int64)10000000 / pVih->AvgTimePerFrame );
    }

    //--------
    pVSC->Release();
    pBuilder->Release();
    pSrc->Release();
    return true;
}


bool piCameraMgrWin::Init( void )
{
    mNumCameras = 0;

    if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        return false;

    //------------------------------

    HRESULT hr;
    ICreateDevEnum  *pDevEnum =NULL;

    hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **) &pDevEnum);
    if (FAILED(hr))
        return false;

    IEnumMoniker *pClassEnum = NULL;
    hr = pDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
    if (FAILED(hr))
        return false;

    if (pClassEnum == NULL)
        return true;

    while (pClassEnum->Next(1, &mDevices[mNumCameras].pMoniker, NULL) == S_OK)
    {
        IPropertyBag *pPropBag;
        hr = mDevices[mNumCameras].pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
        if (FAILED(hr))
            continue;

        VARIANT varName;
        VariantInit(&varName);
        hr = pPropBag->Read(L"Description", &varName, 0);
        if (FAILED(hr))
        hr = pPropBag->Read(L"FriendlyName", &varName, 0);

        if (FAILED(hr))
        {
            VariantClear(&varName); 
            pPropBag->Release();
            continue;
        }

        wcscpy_s( mDevices[mNumCameras].pName, varName.bstrVal );
        pPropBag->Release();

        iGetCaps( mDevices[mNumCameras].pMoniker, mDevices[mNumCameras].pCaps, 64, &mDevices[mNumCameras].pNumCaps );

        mNumCameras++;
        if( mNumCameras>=256 ) 
            break;
    }

    return true;
}

void piCameraMgrWin::End( void )
{
}

int piCameraMgrWin::GetNumDevices( void )
{ 
    return mNumCameras;
}

bool piCameraMgrWin::GetDeviceInfo( int id, wchar_t **name, int *numcaps, int **caps )
{ 
    if( id<0 || id>mNumCameras )
        return false;

    MyDeviceInfo *dev = mDevices + id;
    name[0] = dev->pName;
    numcaps[0] = dev->pNumCaps;
    caps[0] = dev->pCaps;
    return true;
}

piCamera *piCameraMgrWin::CreateCamera( int deviceID, int capID, int *xres, int *yres, int *rate )
{
    if (deviceID<0 || deviceID >= mNumCameras)
        return nullptr;

    const MyDeviceInfo *dev = mDevices + deviceID;

    piCameraWin *cam = new piCameraWin();

    if( !cam->Init( dev, capID, xres, yres, rate ) )
        return nullptr;

    return cam;
}

void piCameraMgrWin::DestroyCamera( piCamera *cam )
{
    piCameraMgrWin *camwin = (piCameraMgrWin*)cam;
    camwin->End();
    delete camwin;
}

piCameraMgr * piCameraMgr::Create( void )
{
    return new piCameraMgrWin();
}

void piCameraMgr::Destroy(piCameraMgr *me)
{
    delete me;
}
