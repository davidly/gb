//
// Get Bitmap: creates a bitmap containing contents of a window
//

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#pragma warning( disable: 4458 ) // gdi+ generates these warnings
#include <gdiplus.h>
#pragma warning( default: 4458 ) // gdi+ generates these warnings
#include <shlwapi.h>

#include <stdio.h>

#include <memory>
#include <string>
#include <regex>
#include <vector>

using namespace Gdiplus;
using namespace std;

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "kernel32.lib" )
#pragma comment( lib, "gdi32.lib" )
#pragma comment( lib, "gdiplus.lib" )
#pragma comment( lib, "shlwapi.lib" )

void Usage( const char * pcMessage = 0 )
{
    if ( 0 != pcMessage )
        printf( "error: %s\n", pcMessage );

    printf( "usage: gb [-v] [-w] [appname] [outputfile]\n" );
    printf( "  Get Bitmap: creates a bitmap containing contents of a window\n" );
    printf( "  arguments: [appname]       Name of the app to capture. Can contain wildcards.\n" );
    printf( "             [outputfile]    Name of the image file to create.\n" );
    printf( "             [-v]            Verbose logging of debugging information.\n" );
    printf( "             [-w]            Capture the whole window, not just the client area.\n" );
    printf( "  sample usage: (arguments can use - or /)\n" );
    printf( "    gb inbox*outlook outlook.png  Saves a snapshot of the window in the PNG file\n" );
    printf( "    gb *excel excel.jpg           Saves a snapshot of the window to the JPG file\n" );
    printf( "    gb -w calc* calc.jpg          Saves a snapshot of the whole window to the JPG file\n" );
    printf( "    gb 0x6bf0 calc.bmp            Saves a snapshot of the window with this PID to the BMP file\n" );
    printf( "    gb                            Shows titles and positions of visible top-level windows\n" );
    printf( "  notes:\n" );
    printf( "    - if no arguments are specified then all window positions are printed\n" );
    printf( "    - appname can contain ? and * characters as wildcards\n" );
    printf( "    - appname is case insensitive\n" );
    printf( "    - appname can alternatively be a window handle or process id in decimal or hex\n" );
    printf( "    - only visible, top-level windows are considered\n" );
    printf( "    - the screen is grabbed, so obscurring windows are shown\n" );
    printf( "    - if no image type can be inferred from the extension, JPG is assumed\n" );
    printf( "    - extra pixels surrounding windows exist because Windows draws narrow inner borders\n" );
    printf( "      and transparent outer edges for borders that are in fact wide\n" );
    printf( "    - 'modern' app client area includes the chrome\n" );
    exit( 0 );
} //Usage

bool g_Enumerate = true;                                        // enumerate windows; don't capture bitmap
bool g_WholeWindow = false;                                     // the whole window, not just the client area
bool g_Verbose = false;                                         // print debug information
bool g_CaptureSuccess = false;                                  // did we capture a bitmap?
bool g_FoundMatchingWindow = false;                             // did we find a matching window?
WCHAR g_BitmapName[ MAX_PATH ] = {0};                           // output filename
WCHAR g_AppName[ MAX_PATH ] = {0};                              // name of the app to find, can contain wildcards
WCHAR g_AppNameRegex[ 3 * _countof( g_AppName ) ] = {0};        // regex equivalent of g_AppName
unsigned long long g_AppId = 0;                                 // hwnd or procid to use instead of g_AppName

int GetEncoderClsid( const WCHAR * format, CLSID * pClsid )
{
    UINT num = 0, size = 0;
    GetImageEncodersSize( & num, & size );
    if ( 0 == size )
        return -1;

    vector<byte> codecInfo( size );
    ImageCodecInfo * pImageCodecInfo = (ImageCodecInfo *) codecInfo.data();
    GetImageEncoders( num, size, pImageCodecInfo );

    for ( UINT j = 0; j < num; j++ )
    {
        if ( 0 == _wcsicmp( pImageCodecInfo[ j ].MimeType, format ) )
        {
            *pClsid = pImageCodecInfo[ j ].Clsid;
            return j;
        }
    }

    return -1;
} //GetEncoderClsid

const WCHAR * InferOutputType( WCHAR *ext )
{
    if ( !_wcsicmp( ext, L".bmp" ) )
        return L"image/bmp";

    if ( !_wcsicmp( ext, L".gif" ) )
        return L"image/gif";

    if ( ( !_wcsicmp( ext, L".jpg" ) ) || ( !_wcsicmp( ext, L".jpeg" ) ) )
        return L"image/jpeg";

    if ( !_wcsicmp( ext, L".png" ) )
        return L"image/png";

    if ( ( !_wcsicmp( ext, L".tif" ) ) || ( !_wcsicmp( ext, L".tiff" ) ) )
        return L"image/tiff";

    return L"image/jpeg";     // default
} //InferOutputType

bool SaveHBitmap( HBITMAP hbitmap, const WCHAR * path )
{
    Status s = GenericError;

    const WCHAR * pwcMime = InferOutputType( PathFindExtension( path ) );
    CLSID encoderClsid;
    int encoderID = GetEncoderClsid( pwcMime, &encoderClsid );
    if ( -1 != encoderID )
    {
        Bitmap bmp( hbitmap, 0 );

        s = bmp.Save( path, &encoderClsid, NULL );
        if ( Ok != s )
            printf( "unable to write the output image, gdp+ status %#x\n", s );
    }
    else
    {
        printf( "can't find encoder for mime type %ws\n", pwcMime );
        s = InvalidParameter;
    }

    return ( Ok == s );
} //SaveHBitmap

#pragma warning( disable: 4100 ) // unreferenced formal parameter
BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam )
{
    BOOL visible = IsWindowVisible( hwnd );
    if ( !visible )
        return true;

    static WCHAR awcText[ 1000 ];
    int len = GetWindowText( hwnd, awcText, _countof( awcText ) );
    if ( 0 == len )
    {
        //printf( "can't retrieve window text, error %d\n", GetLastError() );
        return true;
    }

    // Ignore the console window in which this app is running

    static WCHAR awcConsole[ 1000 ];
    DWORD consoleTitleLen = GetConsoleTitle( awcConsole, _countof( awcConsole ) );
    if ( ( 0 != consoleTitleLen ) && !wcscmp( awcConsole, awcText ) )
        return true;

    //if ( 40 == len )
    //    for ( int i = 0; i < len; i++ )
    //        printf( "i %#x, char %d == %wc\n", i, awcText[ i ], awcText[ i ] );

    // 0x2000 - 0x206f is punctuation, which printf handles badly. convert to spaces

    for ( int i = 0; i < len; i++ )
        if ( awcText[ i ] >= 0x2000 && awcText[ i ] <= 0x206f )
            awcText[ i ] = ' ';

    DWORD procId = 0;
    GetWindowThreadProcessId( hwnd, & procId );

    WINDOWPLACEMENT wp;
    wp.length = sizeof wp;
    BOOL ok = GetWindowPlacement( hwnd, & wp );
    if ( !ok )
        return true;

    WINDOWINFO wi;
    wi.cbSize = sizeof wi;
    ok = GetWindowInfo( hwnd, & wi );
    if ( ok && ( ( SW_NORMAL == wp.showCmd ) || ( SW_MAXIMIZE == wp.showCmd ) ) )
    {
        RECT rcWindow = wi.rcWindow;

        if ( g_Enumerate )
        {
            printf( " %6d %6d %6d %6d %#11llx %#7d '%ws'\n",
                    rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom, (__int64) hwnd, procId, awcText );
        }
        else
        {
            wstring match( g_AppNameRegex );
            std::wregex e( match, wregex::ECMAScript | wregex::icase );
            wstring text( awcText );

            if ( std::regex_match( text, e ) || procId == g_AppId || hwnd == (HWND) g_AppId )
            {
                g_FoundMatchingWindow = true;
                HDC hdcDesktop = GetDC( 0 );
                if ( 0 != hdcDesktop )
                {
                    HDC hdcMem = CreateCompatibleDC( hdcDesktop );
                    if ( 0 != hdcMem )
                    {
                        int width = rcWindow.right - rcWindow.left;
                        int height = rcWindow.bottom - rcWindow.top;
                        int sourceX = rcWindow.left;
                        int sourceY = rcWindow.top;

                        if ( g_Verbose )
                        {
                            printf( "original window left and top: %d %d\n", rcWindow.left, rcWindow.top );
                            printf( "original window right and bottom: %d %d\n", rcWindow.right, rcWindow.bottom );
                            printf( "sourceX %d sourceY %d, copy width %d height %d\n", sourceX, sourceY, width, height );
                        }

                        RECT rectClient;
                        if ( !g_WholeWindow && GetClientRect( hwnd, & rectClient ) )
                        {
                            if ( g_Verbose )
                                printf( "client rect: %d %d %d %d\n", rectClient.left, rectClient.top, rectClient.right, rectClient.bottom );

                            width = rectClient.right;
                            height = rectClient.bottom;

                            POINT ptUL = { 0, 0 };
                            MapWindowPoints( hwnd, HWND_DESKTOP, & ptUL, 1 );

                            sourceX = ptUL.x;
                            sourceY = ptUL.y;

                            if ( g_Verbose )
                                printf( "final sourceX %d, sourceY %d, width %d, height %d\n", sourceX, sourceY, width, height );
                        }

                        HBITMAP hbMem = CreateCompatibleBitmap( hdcDesktop, width, height );
                        if ( 0 != hbMem )
                        {
                            HANDLE hbOld = SelectObject( hdcMem, hbMem );
                            BOOL bltOK = BitBlt( hdcMem, 0, 0, width, height, hdcDesktop, sourceX, sourceY, SRCCOPY );
                            if ( bltOK )
                            {
                                g_CaptureSuccess = SaveHBitmap( hbMem, g_BitmapName );

                                if ( g_CaptureSuccess )
                                    printf( "bitmap created successfully: %ws\n", g_BitmapName );
                            }
                            else
                                printf( "unable to blt from the desktop to the memory hdc, error %d\n", GetLastError() );

                            SelectObject( hdcMem, hbOld );
                            DeleteObject( hbMem );
                        }
                        else
                            printf( "unable create create a compatible Bitmap\n" );

                        DeleteDC( hdcMem ); // from Create(), not Get()
                    }
                    else
                        printf( "unable to get the compatible memory hdc, error %d\n", GetLastError() );

                    ReleaseDC( 0, hdcDesktop ); // From Get(), not Create()
                }
                else
                    printf( "unable to get the desktop hdc, error %d\n", GetLastError() );

                return false; // end the enumeration early because we found the window
            }
        }
    }

    return true;
} //EnumWindowsProc

extern "C" int wmain( int argc, WCHAR * argv[] )
{
    // Get physical coordinates in Windows API calls. Otherwise everything is scaled
    // for the DPI of each monitor. For multi-mon with different DPIs it gets complicated.
    // This one line simplifies the code considerably.

    SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );

    for ( int a = 1; a < argc; a++ )
    {
        WCHAR const * parg = argv[ a ];

        if ( L'-' == parg[0] || L'/' == parg[0] )
        {
            WCHAR p = (WCHAR) tolower( parg[1] );

            if ( 'w' == p )
                g_WholeWindow = true;
            else if ( 'v' == p )
                g_Verbose = true;
            else
                Usage( "invalid argument" );
        }
        else if ( 0 == g_AppName[ 0 ] )
        {
            wcscpy( g_AppName, parg );
            g_Enumerate = false;

            int r = 0;
            size_t len = wcslen( g_AppName );
            for ( size_t i = 0; i < len; i++ )
            {
                WCHAR c = g_AppName[ i ];

                if ( '*' == c )
                {
                    g_AppNameRegex[ r++ ] = '.';
                    g_AppNameRegex[ r++ ] = '*';
                }
                else if ( '?' == c )
                {
                    g_AppNameRegex[ r++ ] = '.';
                    g_AppNameRegex[ r++ ] = '?';
                }
                else
                {
                    g_AppNameRegex[ r++ ] = '[';
                    g_AppNameRegex[ r++ ] = c;
                    g_AppNameRegex[ r++ ] = ']';
                }
            }

            g_AppNameRegex[ r++ ] = '$';
            g_AppNameRegex[ r ] = 0;

            //printf( "app name: '%ws'\n", g_AppName );
            //printf( "app name regex: '%ws'\n", g_AppNameRegex );
        }
        else if ( 0 == g_BitmapName[ 0 ] )
        {
            wcscpy( g_BitmapName, parg );
        }
        else
            Usage( "too many arguments specified" );
    }

    if ( !g_Enumerate && 0 == g_BitmapName[0] )
        Usage( "output bitmap filename wasn't specified" );

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Status s = GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );

    if ( Ok != s )
    {
        printf( "can't initialize gdiplus, error %#x\n", s );
        Usage();
    }

    if ( g_Enumerate )
        printf( "   left    top  right bottom        hwnd     pid text\n" );
    else
    {
        WCHAR * pwcEnd = 0;
        g_AppId = wcstoull( g_AppName, &pwcEnd, 0 );

        if ( g_Verbose )
            printf( "app id %#llx == %lld\n", g_AppId, g_AppId );
    }

    EnumWindows( EnumWindowsProc, 0 );

    if ( !g_Enumerate && !g_FoundMatchingWindow )
        printf( "does the window exist? unable to capture a bitmap for %ws\n", g_AppName );

    GdiplusShutdown( gdiplusToken );

    return 0;
} //wmain

