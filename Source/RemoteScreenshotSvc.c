//
// gcc -c RemoteScreenshotSvc.c -o RemoteScreenshotSvc.exe -lgdi32
//

#include <windows.h>
#include <string.h>
#include <stdio.h>

INT WINAPI           ServiceMain( VOID );
VOID WINAPI MyServiceCtrlHandler( DWORD dwOption );
VOID        GetDesktopScreenshot( VOID );
BOOL         SaveScreenshotToBMP( HDC *hdcCompatibleDisplay, HBITMAP *hbmCompatibleDisplay, CHAR *szFileName );
VOID             WriteToErrorLog( CHAR *szErrorMsg );

SERVICE_STATUS        MyServiceStatus;
SERVICE_STATUS_HANDLE MyServiceStatusHandle;

INT main( INT argc, CHAR *argv[] )
{
	SERVICE_TABLE_ENTRY DispatchTable[] = { { "RemoteScreenshotSvc", (LPSERVICE_MAIN_FUNCTION)ServiceMain }, { NULL, NULL } };

	StartServiceCtrlDispatcher( DispatchTable );

	return 0;
}

INT WINAPI ServiceMain( VOID )
{
	MyServiceStatus.dwServiceType             = SERVICE_WIN32;
	MyServiceStatus.dwCurrentState            = SERVICE_STOP;
	MyServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
	MyServiceStatus.dwWin32ExitCode           = 0;
	MyServiceStatus.dwServiceSpecificExitCode = 0;
	MyServiceStatus.dwCheckPoint              = 0;
	MyServiceStatus.dwWaitHint                = 0;

	MyServiceStatusHandle = RegisterServiceCtrlHandler( "RemoteScreenshotSvc", MyServiceCtrlHandler );

	if ( MyServiceStatusHandle != 0 )
	{
		MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;

		if ( SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus ) )
		{
			MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
 
			if ( SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus ) )
			{
				GetDesktopScreenshot();
			}
		}
	}

	MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

	if ( SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus ) )
	{
		MyServiceStatus.dwCurrentState = SERVICE_ACCEPT_STOP;

		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
	}

	return 0;
}

VOID WINAPI MyServiceCtrlHandler( DWORD dwOption )
{ 
	switch ( dwOption )
	{
		case SERVICE_CONTROL_PAUSE:
			MyServiceStatus.dwCurrentState = SERVICE_PAUSED;

			SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

			break;

		case SERVICE_CONTROL_CONTINUE:
			MyServiceStatus.dwCurrentState = SERVICE_RUNNING;

			SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

			break;
 
		case SERVICE_CONTROL_STOP:
			break;

		case SERVICE_CONTROL_INTERROGATE:
			break;

		default:
			break;
	}
}

VOID GetDesktopScreenshot( VOID )
{
	HDC               hdcDisplay;
	HDC     hdcCompatibleDisplay;
	INT                   nWidth;
	INT                  nHeight;
	HBITMAP hbmCompatibleDisplay;
	HBITMAP    hbmCurrentDisplay;

	hdcDisplay = NULL;

	hdcDisplay = CreateDC( "DISPLAY", NULL, NULL, NULL );

	if ( hdcDisplay != NULL )
	{
		hdcCompatibleDisplay = NULL;

		hdcCompatibleDisplay = CreateCompatibleDC( hdcDisplay );

		if ( hdcCompatibleDisplay != NULL )
		{
			nWidth  = GetDeviceCaps( hdcDisplay, HORZRES );
			nHeight = GetDeviceCaps( hdcDisplay, VERTRES );

			hbmCompatibleDisplay = NULL;

			hbmCompatibleDisplay = CreateCompatibleBitmap( hdcDisplay, nWidth, nHeight );

			if ( hbmCompatibleDisplay != NULL )
			{
				hbmCurrentDisplay = NULL;

				hbmCurrentDisplay = SelectObject( hdcCompatibleDisplay, hbmCompatibleDisplay );

				if ( hbmCurrentDisplay != NULL )
				{
					if ( BitBlt( hdcCompatibleDisplay, 0, 0, nWidth, nHeight, hdcDisplay, 0, 0, SRCCOPY ) )
					{
						if ( !SaveScreenshotToBMP( &hdcCompatibleDisplay, &hbmCompatibleDisplay, "Screenshot.bmp" ) )
						{
							WriteToErrorLog( "ERROR! Cannot save screenshot as bitmap on remote host.\n" );
						}
					}
					else
					{
						WriteToErrorLog( "ERROR! Cannot transfer bitmap color data on remote host.\n" );
					}
				}

				if ( !DeleteObject( hbmCompatibleDisplay ) )
				{
					WriteToErrorLog( "ERROR! Cannot delete display object on remote host.\n" );
				}
			}
			else
			{
				WriteToErrorLog( "ERROR! Cannot create compatible bitmap on remote host.\n" );
			}

			if ( !DeleteDC( hdcCompatibleDisplay ) )
			{
				WriteToErrorLog( "ERROR! Cannot delete compatible device context on remote host.\n" );
			}
		}
		else
		{
			WriteToErrorLog( "ERROR! Cannot create compatible device context on remote host.\n" );
		}

		if ( !DeleteDC( hdcDisplay ) )
		{
			WriteToErrorLog( "ERROR! Cannot delete device context on remote host.\n" );
		}
	}
	else
	{
		WriteToErrorLog( "ERROR! Cannot create device context on remote host.\n" );
	}
}

BOOL SaveScreenshotToBMP( HDC *hdcCompatibleDisplay, HBITMAP *hbmCompatibleDisplay, CHAR szFileName[] )
{
	BOOL                    bReturn;
	BITMAP                   bmInfo;
	WORD                 wColorBits;
	BITMAPINFO            *pbmiInfo;
	BITMAPINFOHEADER     *pbmihInfo;
	BYTE                     *pBits;
	HANDLE                    hFile;
	BITMAPFILEHEADER            hdr;
	DWORD            dwBytesWritten;
	DWORD               dwImageSize;

	bReturn = FALSE;

	if ( GetObject( *hbmCompatibleDisplay, sizeof( BITMAP ), &bmInfo ) != 0 )
	{
		wColorBits = (WORD)( bmInfo.bmPlanes * bmInfo.bmBitsPixel );

		if ( wColorBits == 1 )
		{
			wColorBits = 1;
		}
		else if ( wColorBits <= 4 )
		{
			wColorBits = 4;
		}
		else if ( wColorBits <= 8 )
		{
			wColorBits = 8;
		}
		else if ( wColorBits <= 16 )
		{
			wColorBits = 16;
		}
		else if ( wColorBits <= 24 )
		{
			wColorBits = 24;
		}
		else
		{
			wColorBits = 32;
		}

		pbmiInfo = NULL;

		if ( wColorBits != 24 )
		{
			pbmiInfo = (BITMAPINFO *)LocalAlloc( LPTR, sizeof( BITMAPINFOHEADER ) + sizeof( RGBQUAD ) * ( 1 << wColorBits ) );
		}
		else
		{
			pbmiInfo = (BITMAPINFO *)LocalAlloc( LPTR, sizeof( BITMAPINFOHEADER ) );
		}

		if ( pbmiInfo != NULL )
		{
			pbmiInfo->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
			pbmiInfo->bmiHeader.biWidth    = bmInfo.bmWidth;
			pbmiInfo->bmiHeader.biHeight   = bmInfo.bmHeight;
			pbmiInfo->bmiHeader.biPlanes   = bmInfo.bmPlanes;
			pbmiInfo->bmiHeader.biBitCount = bmInfo.bmBitsPixel;

			if ( wColorBits < 24 )
			{
				pbmiInfo->bmiHeader.biClrUsed = ( 1 << wColorBits );
			}

			pbmiInfo->bmiHeader.biCompression  = BI_RGB;
			pbmiInfo->bmiHeader.biSizeImage    = ( ( pbmiInfo->bmiHeader.biWidth * wColorBits + 31 ) & ~31 ) / 8 * pbmiInfo->bmiHeader.biHeight;
			pbmiInfo->bmiHeader.biClrImportant = 0;

			pbmihInfo = (BITMAPINFOHEADER *)pbmiInfo;

			pBits = NULL;

			pBits = (BYTE *)GlobalAlloc( GMEM_FIXED, pbmihInfo->biSizeImage );

			if ( pBits != NULL )
			{
				if ( GetDIBits( *hdcCompatibleDisplay, *hbmCompatibleDisplay, 0, pbmihInfo->biHeight, pBits, pbmiInfo, DIB_RGB_COLORS ) != 0 )
				{
					bReturn = TRUE;

					hFile = CreateFile( szFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

					if ( hFile != INVALID_HANDLE_VALUE )
					{
						hdr.bfType      = 0x4d42;
						hdr.bfSize      = (DWORD)( sizeof( BITMAPFILEHEADER ) + pbmihInfo->biSize + pbmihInfo->biClrUsed * sizeof( RGBQUAD ) + pbmihInfo->biSizeImage );
						hdr.bfReserved1 = 0;
						hdr.bfReserved2 = 0;
						hdr.bfOffBits   = (DWORD)sizeof( BITMAPFILEHEADER ) + pbmihInfo->biSize + pbmihInfo->biClrUsed * sizeof( RGBQUAD );

						if ( !WriteFile( hFile, (VOID *)&hdr, sizeof( BITMAPFILEHEADER ), &dwBytesWritten, NULL ) )
						{
							WriteToErrorLog( "ERROR! Cannot write to bitmap file on remote host.\n" );
						}

						if ( !WriteFile( hFile, (VOID *)pbmihInfo, sizeof( BITMAPINFOHEADER ) + pbmihInfo->biClrUsed * sizeof( RGBQUAD ), &dwBytesWritten, NULL ) )
						{
							WriteToErrorLog( "ERROR! Cannot write to bitmap file on remote host.\n" );
						}

						dwImageSize = pbmihInfo->biSizeImage;

						if ( !WriteFile( hFile, (VOID *)pBits, dwImageSize, &dwBytesWritten, NULL ) )
						{
							WriteToErrorLog( "ERROR! Cannot write to bitmap file on remote host.\n" );
						}

						CloseHandle( hFile );
					}
					else
					{
						WriteToErrorLog( "ERROR! Cannot create bitmap file on remote host.\n" );
					}
				}
				else
				{
					WriteToErrorLog( "ERROR! Cannot copy bitmap information on remote host.\n" );
				}

				GlobalFree( (HGLOBAL)pBits );
			}
			else
			{
				WriteToErrorLog( "ERROR! Cannot allocate memory via GlobalAlloc on remote host.\n" );
			}

			LocalFree( pbmiInfo );
		}
		else
		{
			WriteToErrorLog( "ERROR! Cannot allocate memory via LocalAlloc on remote host.\n" );
		}
	}
	else
	{
		WriteToErrorLog( "ERROR! Cannot get graphics object information on remote host.\n" );
	}

	return bReturn;
}

VOID WriteToErrorLog( CHAR szErrorMsg[] )
{
	FILE *pOutputFile;

	pOutputFile = fopen( "ErrorLog.txt", "r" );

	if ( pOutputFile != NULL )
	{
		fclose( pOutputFile );
	}
	else
	{
		pOutputFile = fopen( "ErrorLog.txt", "w" );

		if ( pOutputFile != NULL )
		{
			fclose( pOutputFile );
		}
	}

	pOutputFile = fopen( "ErrorLog.txt", "a+" );

	if ( pOutputFile != NULL )
	{
		fprintf( pOutputFile, "%s", szErrorMsg );

		fclose( pOutputFile );
	}
}

// Written by Reed Arvin | reedlarvin@gmail.com
