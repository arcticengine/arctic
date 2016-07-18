//#define WIN32_LEAN_AND_MEAN
//#define WIN32_EXTRA_LEAN

#include <windows.h>
#include <winuser.h>

#include <stdio.h>

#include "../piStr.h"
#include "../piTypes.h"

namespace piLibs {


int piSystemInfo_getIntegratedMultitouch( )
{
#if _MSC_VER < 1700 
	return 0;
#else
	// test for touch
	int value = GetSystemMetrics(SM_DIGITIZER);
	if (value & NID_READY) /* stack ready */
	{ 
		if (value  & NID_MULTI_INPUT) /* digitizer is multitouch */ 
		{
			if (value & NID_INTEGRATED_TOUCH) /* Integrated touch */
			{
				return 1;
			}
		}
	}
	return 0;
#endif
}

void piSystemInfo_getScreenResolution( int *res )
{
//SM_CXSCREEN
//SM_CXVIRTUALSCREEN
//SM_CXFULLSCREEN
    
    res[0] = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    res[1] = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

void piSystemInfo_getFreeRAM( uint64 *avail, uint64 *total )
{
#if 1
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof( ms );
	GlobalMemoryStatusEx( &ms );

    *avail = ms.ullAvailPhys;
    *total = ms.ullTotalPhys;
#else
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(MEMORYSTATUSEX);

    GlobalMemoryStatusEx( &statex );

/*    
    printf ("%ld percent of memory is in use.\n",
          statex.dwMemoryLoad);
*/

    // physical memory
    *avail = statex.ullAvailPhys;
    *total = statex.ullTotalPhys;

    // paging memory
    //statex.ullTotalPageFile;
    //statex.ullAvailPageFile;
    // virtual memory
    //statex.ullTotalVirtual
    //statex.ullAvailVirtual
#endif

}

int piSystemInfo_getCPUs( void )
{
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return( si.dwNumberOfProcessors );
}

void piSystemInfo_getOS( wchar_t *str, int length )
{
#if _MSC_VER<=1300
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx( &osvi );


    switch( osvi.dwMinorVersion )
        {
        case 0:  
            if( osvi.dwMajorVersion==4 )
                sstrncpy( str, length, "Windows 95 or NT 4.0", 0 );     
            else
                sstrncpy( str, length, "Windows 2000", 0 ); 
            break;
        case 1:  sstrncpy( str, length, "Windows XP", 0 ); break;
        case 2:  sstrncpy( str, length, "Windows Server 2003", 0 ); break;
        case 10: sstrncpy( str, length, "Windows 98", 0 ); break;
        case 90: sstrncpy( str, length, "Windows Me", 0 ); break;
        default: sstrncpy( str, length, "Unknown", 0 ); break;
        }
#else

    #define BUFSIZE 80

    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
   
    wchar_t tmp[128];
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    BOOL bOsVersionInfoEx;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
    {
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
        {
            piwstrcpy(str, length, L"Microsoft Windows ");
            return;
        }
    }

   // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

   pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),"GetNativeSystemInfo");
   if( NULL != pGNSI )
      pGNSI(&si);
   else 
      GetSystemInfo(&si);

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:
            //------------------
            if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
            {
                if( osvi.wProductType == VER_NT_WORKSTATION )
                    piwstrcpy(str, length, L"Windows Vista ");
                else 
                    piwstrcpy(str, length, L"Windows Server \"Longhorn\" " );
            }
            else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
            {
                if( GetSystemMetrics(89) )
                    piwstrcpy(str, length, L"Microsoft Windows Server 2003 \"R2\" ");
                else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                    piwstrcpy(str, length, L"Microsoft Windows XP Professional x64 Edition ");
                else 
                    piwstrcpy(str, length, L"Microsoft Windows Server 2003, ");
            }
            else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
                piwstrcpy(str, length, L"Microsoft Windows XP ");
            else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
                piwstrcpy(str, length, L"Microsoft Windows 2000 ");
            else if ( osvi.dwMajorVersion <= 4 )
                piwstrcpy(str, length, L"Microsoft Windows NT ");
            else 
                piwstrcpy(str, length, L"Microsoft Windows ");
            //------------------
            // Test for specific product on Windows NT 4.0 SP6 and later.
            if( bOsVersionInfoEx )
            {
                // Test for the workstation type.
                if ( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64)
                {
                    if( osvi.dwMajorVersion == 4 )
                        piwstrncat(str, length, L"Workstation 4.0 ", 0 );
                    else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                        piwstrncat(str, length, L"Home Edition ", 0 );
                    else 
                        piwstrncat(str, length, L"Professional ", 0 );
                }
                else if ( osvi.wProductType == VER_NT_SERVER || osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
                {
                    if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
                    {
                        if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
                        {
                            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                                piwstrncat(str, length, L"Datacenter Edition for Itanium-based Systems", 0 );
                            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                                piwstrncat(str, length, L"Enterprise Edition for Itanium-based Systems", 0 );
                        }
                        else if( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
                        {
                            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                                piwstrncat(str, length, L"Datacenter x64 Edition ", 0 );
                            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                                piwstrncat(str, length, L"Enterprise x64 Edition ", 0 );
                            else 
                                piwstrncat(str, length, L"Standard x64 Edition ", 0 );
                        }
                        else
                        {
                            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                                piwstrncat(str, length, L"Datacenter Edition ", 0 );
                            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                                piwstrncat(str, length, L"Enterprise Edition ", 0 );
                            else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
                                piwstrncat(str, length, L"Web Edition ", 0 );
                            else 
                                piwstrncat(str, length, L"Standard Edition ", 0 );
                        }
                    }
                    else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
                    {
                        if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            piwstrncat(str, length, L"Datacenter Server ", 0 );
                        else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            piwstrncat(str, length, L"Advanced Server ", 0 );
                        else 
                            piwstrncat(str, length, L"Server ", 0 );
                    }
                    else  // Windows NT 4.0 
                    {
                        if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            piwstrncat(str, length, L"Server 4.0, Enterprise Edition ", 0 );
                        else 
                            piwstrncat(str, length, L"Server 4.0 ", 0 );
                    }
                }
            }
            // Test for specific product on Windows NT 4.0 SP5 and earlier
            else  
            {
                HKEY hKey;
                TCHAR szProductType[BUFSIZE];
                DWORD dwBufLen=BUFSIZE*sizeof(TCHAR);
                LONG lRet;

                lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_QUERY_VALUE, &hKey );
                if( lRet != ERROR_SUCCESS )
                    {
                    piwstrcpy(str, length, L"Microsoft Windows");
                    return;
                    }

                lRet = RegQueryValueEx( hKey, TEXT("ProductType"), NULL, NULL, (LPBYTE) szProductType, &dwBufLen);
                RegCloseKey( hKey );

                if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE*sizeof(TCHAR)) )
                    {
                    piwstrcpy(str, length, L"Microsoft Windows");
                    return;
                    }

                if ( lstrcmpi( TEXT("WINNT"), szProductType) == 0 )
                    piwstrncat(str, length, L"Workstation ", 0 );
                if ( lstrcmpi( TEXT("LANMANNT"), szProductType) == 0 )
                    piwstrncat(str, length, L"Server ", 0 );
                if ( lstrcmpi( TEXT("SERVERNT"), szProductType) == 0 )
                    piwstrncat(str, length, L"Advanced Server ", 0 );

                piwsprintf(tmp,128,L"%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
                piwstrncat(str, length, tmp, 0 );
            }

        // Display service pack (if any) and build number.
        if( osvi.dwMajorVersion == 4 &&  lstrcmpi( osvi.szCSDVersion, TEXT("Service Pack 6") ) == 0 )
        { 
            HKEY hKey;
            LONG lRet;

            // Test for SP6 versus SP6a.
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), 0, KEY_QUERY_VALUE, &hKey );
            if( lRet == ERROR_SUCCESS )
                {
                piwsprintf(tmp,128,L"Service Pack 6a (Build %d)", osvi.dwBuildNumber & 0xFFFF );
                piwstrncat(str, length, tmp, 0 );
                }
            else // Windows NT 4.0 prior to SP6a
                {
                piwsprintf(tmp,128,L"%s (Build %d)", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF );
                piwstrncat(str, length, tmp, 0 );
                }

            RegCloseKey( hKey );
        }
        else // not Windows NT 4.0 
        {
                piwsprintf(tmp,128,L"%s (Build %d)", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF );
                piwstrncat(str, length, tmp, 0 );
        }
        break;

    // Test for the Windows Me/98/95.
    case VER_PLATFORM_WIN32_WINDOWS:
        if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
        {
            piwstrcpy(str, length, L"Microsoft Windows 95 ");
            if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B')
                piwstrncat(str, length, L"OSR2 ", 0 );
        } 
        else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
        {
            piwstrcpy(str, length, L"Microsoft Windows 98 ");
            if ( osvi.szCSDVersion[1]=='A' || osvi.szCSDVersion[1]=='B')
                piwstrncat(str, length, L"SE ", 0 );
        } 
        else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
        {
          piwstrcpy(str, length, L"Microsoft Windows Millennium Edition");
        } 
      break;

    case VER_PLATFORM_WIN32s:
        piwstrcpy(str, length, L"Microsoft Win32s");
        break;
    }

#endif
}

void piSystemInfo_getProcessor( wchar_t *str, int length, int *mhz )
{
	HKEY pHRoot;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System", 0, KEY_READ, &pHRoot);
	if (res != ERROR_SUCCESS)
        {
        piwstrncpy( str, length, L"Unknown", 0 );
        RegCloseKey(pHRoot);
        return;
        }

	HKEY key;
	if (RegOpenKey(pHRoot, L"CentralProcessor\\0", &key) != ERROR_SUCCESS)
	    {
        piwstrncpy( str, length, L"Unknown", 0 );
        RegCloseKey(pHRoot);
        return;
        }

	unsigned char data[1024];
	unsigned long datasize = 1024;
	DWORD type;

	if( RegQueryValueEx(key, L"ProcessorNameString", 0, &type, data, &datasize) != ERROR_SUCCESS )
    {
		piwstrncpy( str, length, L"Unknown", 0 );
    }
	else if (type == REG_SZ)
	{
		data[datasize] = 0;

        //int i=0; while( data[i]==' ' ) i++;

		//piwsprintf( str, 64, L"%d", datasize-i );
		piwstrncpy( str, length, (wchar_t*)(data+0), 0 );
		str[datasize/2 - 1] = 0;

		//if( !MultiByteToWideChar(CP_ACP,0,(char*)data,datasize,str,length) ) piwstrncpy( str, length, L"Unknown", 0 );

    }


    if( RegQueryValueEx(key, L"~MHz", 0, &type, data, &datasize) != ERROR_SUCCESS )
    {
		*mhz = 13;
    }
	else if (type == REG_DWORD)
    {
        *mhz = *((int*)data);
    }

	RegCloseKey(key);
    RegCloseKey(pHRoot);

}



static wchar_t *dia[7] = { L"Mon", L"Tue", L"Wen", L"Thu", L"Fri", L"Sat", L"Sun" };

void piSystemInfo_getTime( wchar_t *str, int length )
{
    SYSTEMTIME t;

    GetSystemTime( &t );

    piwsprintf( str, length, L"%d:%d:%d of %s %d/%d/%d", t.wHour, t.wMinute, t.wSecond, dia[t.wDayOfWeek], t.wDay, t.wMonth, t.wYear );
}

static int char2num( wchar_t c )
{
    if( c>='0' && c<='9' )
        return c - '0';

    if( c>='a' && c<='f' )
        return 10 + c - 'a';

    return -1;
}


void piSystemInfo_getGfxCardIdentification( wchar_t *vendorID, int vlen, wchar_t *deviceID, int dlen)
{
    #if _MSC_VER>=1300

    DISPLAY_DEVICE dd;
    dd.cb = sizeof(DISPLAY_DEVICE);

    int i = 0;

    while( EnumDisplayDevices(NULL, i, &dd, 0) )
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            const wchar_t *tmp = dd.DeviceID;
            //-----------
            int vid = (char2num(tmp[ 8])<<12) | (char2num(tmp[ 9])<< 8) | 
                      (char2num(tmp[10])<< 4) | (char2num(tmp[11])<< 0);
            if( vid==0x10DE )
                piwstrcpy( vendorID, vlen, L"NVIDIA" );
            else if( vid==0x1002 )
                piwstrcpy( vendorID, vlen, L"ATI" );
            else if( vid==0x102B )
                piwstrcpy( vendorID, vlen, L"MATROX" );
            else if( vid==0x163C || vid==0x8086 )
                piwstrcpy( vendorID, vlen, L"INTEL" );
            else if( vid==0x104a )
                piwstrcpy( vendorID, vlen, L"POWERVR" );
            else
                piwstrcpy( vendorID, vlen, L"Unkown" );
            //------------
            piwstrcpy( deviceID, dlen, (const wchar_t*)dd.DeviceString );
            return;
        }
        i++;
    }
    #endif

    piwstrcpy( vendorID, vlen, L"Unkown" );
    piwstrcpy( deviceID, dlen, L"Unkown" );

}


uint64 piSystemInfo_getVideoMemory( void )
{
    size_t vidMemSizeTotal = 0; 
/*
	VRRegistry registry1(L"HARDWARE\\DEVICEMAP");
	VRUniString res1 = registry1.readString(L"VIDEO", L"\\Device\\Video0", L"");


	if (res1.getLength() > 0)
	{
		/// path into registry is valid, remove header of path
		VRUniString header(L"\\Registry\\Machine\\");
		int index = res1.findSubString(header);
		if (index == 0)
		{
			/// the following line will generate a negative number, effectively
			/// taking the last N characters of res1:
			res1 = res1.sub((int)header.getLength() - (int)res1.getLength());	
			if (res1.getLength() > 0)
			{		
				VRRegistry registry2(res1);
				//VRUniString res2 = registry2.readString("", "Device Description", "");
				//VRLog::wprintf(VRWLOG_DEBUG, L"    Device descr.: '%s'", res2);
					
				vidMemSizeTotal = registry2.readUInt(L"", L"HardwareInformation.MemorySize", -1);
				//VRLog::wprintf(VRWLOG_DEBUG, L"    Video memory: '%d'", pVidMemSizeTotal);		
				
				if (vidMemSizeTotal == -1)
					vidMemSizeTotal = 0;			
			}
		}
	}
*/
    return vidMemSizeTotal;
}



static BOOL CALLBACK mMonitorEnum(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    int *Count = (int*)dwData;
    (*Count)++;
    return TRUE;
}

int piSystemInfo_getNumMonitors( void )
{
    int count = 0;
    if (!EnumDisplayMonitors(NULL, NULL, mMonitorEnum, (LPARAM)&count))
        count = 1;
    return count;
}

}