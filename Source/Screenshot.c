//
// gcc Screenshot.c -o Screenshot.exe -lgdi32
//

#define _WIN32_WINNT 0x0500
#define WINVER       0x0500

#include <windows.h>
#include <string.h>
#include <stdio.h>

BOOL SaveScreenshotToBMP( HDC *hdcCompatibleDisplay, HBITMAP *hbmCompatibleDisplay, CHAR *szFileName );

INT main( INT argc, CHAR *argv[] )
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
							fprintf( stderr, "ERROR! Cannot save screenshot as bitmap.\n" );
						}
					}
					else
					{
						fprintf( stderr, "ERROR! Cannot transfer bitmap color data.\n" );
					}
				}

				if ( !DeleteObject( hbmCompatibleDisplay ) )
				{
					fprintf( stderr, "ERROR! Cannot delete display object.\n" );
				}
			}
			else
			{
				fprintf( stderr, "ERROR! Cannot create compatible bitmap.\n" );
			}

			if ( !DeleteDC( hdcCompatibleDisplay ) )
			{
				fprintf( stderr, "ERROR! Cannot delete compatible device context.\n" );
			}
		}
		else
		{
			fprintf( stderr, "ERROR! Cannot create compatible device context.\n" );
		}

		if ( !DeleteDC( hdcDisplay ) )
		{
			fprintf( stderr, "ERROR! Cannot delete device context.\n" );
		}
	}
	else
	{
		fprintf( stderr, "ERROR! Cannot create device context.\n" );
	}

	return 0;
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
							fprintf( stderr, "ERROR! Cannot write to bitmap file.\n" );
						}

						if ( !WriteFile( hFile, (VOID *)pbmihInfo, sizeof( BITMAPINFOHEADER ) + pbmihInfo->biClrUsed * sizeof( RGBQUAD ), &dwBytesWritten, NULL ) )
						{
							fprintf( stderr, "ERROR! Cannot write to bitmap file.\n" );
						}

						dwImageSize = pbmihInfo->biSizeImage;

						if ( !WriteFile( hFile, (VOID *)pBits, dwImageSize, &dwBytesWritten, NULL ) )
						{
							fprintf( stderr, "ERROR! Cannot write to bitmap file.\n" );
						}

						CloseHandle( hFile );
					}
					else
					{
						fprintf( stderr, "ERROR! Cannot create bitmap file.\n" );
					}
				}
				else
				{
					fprintf( stderr, "ERROR! Cannot copy bitmap information.\n" );
				}

				GlobalFree( (HGLOBAL)pBits );
			}
			else
			{
				fprintf( stderr, "ERROR! Cannot allocate memory via GlobalAlloc.\n" );
			}

			LocalFree( pbmiInfo );
		}
		else
		{
			fprintf( stderr, "ERROR! Cannot allocate memory via LocalAlloc.\n" );
		}
	}
	else
	{
		fprintf( stderr, "ERROR! Cannot get graphics object information.\n" );
	}

	return bReturn;
}

// Written by Reed Arvin | reedlarvin@gmail.com
