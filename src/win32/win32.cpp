/*
 *  InfoGB - A portable GameBoy emulator
 *  Copyright (C) 2003  Jay's Factory <jays_factory@excite.co.jp>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  based on gbe - gameboy emulator
 *  Copyright (C) 1999  Chuck Mason, Steven Fuller, Jeff Miller
 */

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <commdlg.h>

#include "../system.h"
#include "../joypad.h"
#include "../cpu.h"
#include "../mem.h"
#include "../rom.h"
#include "../vram.h"

#include "resource.h"
#include "dirsound.h"

#define APP_NAME     "InfoGB v0.5J"
#define VBLANK_INT	 16.66667 

gameboy_proc_t *current_processor = NULL;
unsigned char gameboy_screen_mul  = 1;
force_type force_system           = NONE;
int infogb_ready                  = 0; 

/* for video */

#define GFX_BITDEPTH 16
#define GFX_BYTEDEPTH 2			/* This should always be ( GFX_BITDEPTH / 8 ) */

unsigned short rgbtab[ 256 ];

static unsigned long infogb_window_width = 0;
static unsigned long infogb_window_height = 0;

/* local functions */
void infogb_open_audio();
void infogb_close_audio();
static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
static void keyboard_handler(WPARAM code, BOOL press);

/* main */

static unsigned char* gfx_buffer = NULL;

static const char* CLASS_NAME = "InfoGB_CLASS";
HWND g_hwnd = NULL;
HDC g_hMainDC = NULL;
BITMAPINFOHEADER* g_pDib = NULL;
HBITMAP g_hDibSect = NULL;

static UINT uFrameCount;
static unsigned __int64 beginTime, midTime;
static unsigned __int64 ticksPerSecond;

int infogb_init(char *display)
{
    // Get the initial tick count
    QueryPerformanceCounter((LARGE_INTEGER*)&beginTime);
		midTime = beginTime;
		uFrameCount = 0;
    // Find out how many ticks occur per second
    QueryPerformanceFrequency((LARGE_INTEGER*)&ticksPerSecond);

    return GFX_BYTEDEPTH;
}

int infogb_close()
{
    unsigned __int64 endTime;
    DWORD secs;
    DWORD fps;
    TCHAR tszBuf[1024];

    if ( g_pDib )
    {
        free( g_pDib );
    }

    if ( g_hDibSect )
    {
        DeleteObject( g_hDibSect );
    }

    // Get the final tick count
    QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
    // Get number of ticks
    secs = (DWORD)((endTime - beginTime) / ticksPerSecond);
    fps = (secs == 0) ? 0 : (uFrameCount / secs);
    wsprintf(tszBuf, "Frames drawn: %ld\nFPS: %ld", uFrameCount, fps);
		MessageBox(NULL, tszBuf, APP_NAME, MB_OK);

    return 1;
}

int infogb_create_window(char *title, int width, int height)
{
    HINSTANCE hInstance = GetModuleHandle( NULL );
    WNDCLASS wc;

    memset(&wc, 0, sizeof(wc));
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.hInstance = hInstance;
	  wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON) );
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    if (RegisterClass(&wc) == 0)
    {
        return 0;
    }
    
    g_hwnd = CreateWindow(CLASS_NAME, title,
        WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width + (2 * GetSystemMetrics(SM_CXFIXEDFRAME)), height
            + (2 * GetSystemMetrics(SM_CYFIXEDFRAME)
            + GetSystemMetrics(SM_CYMENU) + 6),
        HWND_DESKTOP,
        NULL,
        hInstance,
        NULL);
    if (g_hwnd == NULL)
    {
        UnregisterClass(CLASS_NAME, hInstance);
        return 0;
    }

    infogb_window_width = width;
    infogb_window_height = height;

    g_hMainDC = GetDC(g_hwnd);

    g_pDib = (BITMAPINFOHEADER*)malloc(
        sizeof( BITMAPINFOHEADER ) + 256 * sizeof( RGBQUAD ) );
    if ( g_pDib == NULL )
    {
        return 0;
    }

    memset( g_pDib, 0, sizeof( BITMAPINFOHEADER ) );
    g_pDib->biSize = sizeof( BITMAPINFOHEADER );
    g_pDib->biWidth = width;
    g_pDib->biHeight = -height; // top down
    g_pDib->biPlanes = 1;
    g_pDib->biBitCount = GFX_BITDEPTH; // 16 bit color
    g_pDib->biCompression = BI_RGB;

    g_hDibSect = CreateDIBSection( g_hMainDC, (BITMAPINFO*)g_pDib,
        DIB_RGB_COLORS, (void **)&gfx_buffer, NULL, 0);
    if ( g_hDibSect == NULL )
    {
        return 0;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    
    return 1;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{	
  OPENFILENAME ofn;
  char szFileName[ 256 ];

	infogb_init(NULL);
	infogb_create_window(APP_NAME, 160, 144);
	infogb_open_audio();

	if ( lpCmdLine[ 0 ] != '\0' ) 
	{
		// drag & drop
		if ( lpCmdLine[ 0 ] == '"' ) {
			// If included space characters, strip dobule quote marks
      lpCmdLine[ strlen( lpCmdLine ) - 1 ] = '\0';
      lpCmdLine++;
		}
		strncpy( szFileName, lpCmdLine, sizeof szFileName );
	} else {
		// file selection
    memset( &ofn, 0, sizeof ofn );
    szFileName[ 0 ] = '\0';
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = g_hwnd;
    ofn.hInstance = hInstance;

    ofn.lpstrFilter = NULL; 
    ofn.lpstrCustomFilter = NULL; 
    ofn.nMaxCustFilter = 0; 
    ofn.nFilterIndex = 0; 
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof szFileName; 
    ofn.lpstrFileTitle = NULL; 
    ofn.nMaxFileTitle = 0; 
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL; 
    ofn.Flags = 0; 
    ofn.nFileOffset; 
    ofn.nFileExtension = 0; 
    ofn.lpstrDefExt = NULL; 
    ofn.lCustData = 0; 
    ofn.lpfnHook = NULL; 
    ofn.lpTemplateName = NULL; 

    if ( !GetOpenFileName( &ofn ) ) {		
			return 0;
		}
	} 

  if (load_rom(szFileName)) {
    if (initialize_memory() == 0) {
      free_memory();

      printf("Unable to allocate memory\n");
      return 0;
    }

    initialize_rom();
    gameboy_cpu_hardreset();
    gameboy_cpu_run();
  } else {
    printf("Unable to load '%s'\n", szFileName);
  }

	infogb_close_audio();
	infogb_close();
	return 1;
}

static LRESULT CALLBACK WindowProc
(
 HWND hWnd, 
 UINT message, 
 WPARAM wParam, 
 LPARAM lParam
 )
{
    static HINSTANCE   hInstance;
    
    switch( message )
    {
    case WM_CREATE:
        hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
        return 0;
        
    case WM_DESTROY:
				infogb_close_audio();
				infogb_close();

        ReleaseDC(hWnd, g_hMainDC);
        PostQuitMessage(0);
				exit(0);
        break;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/* video */

void infogb_vram_blit()
{
    SetDIBitsToDevice( g_hMainDC, 
        0, 0, 
        infogb_window_width, infogb_window_height, 
        0, 0, 0, infogb_window_height,
        gfx_buffer, (BITMAPINFO*)g_pDib, DIB_RGB_COLORS );

    ++uFrameCount;

		infogb_wait();
}

void infogb_set_color(int x, unsigned short c)
{
	rgbtab[ x & 0xff ] = c;
}

void infogb_plot_line(int y, int *index)
{
	unsigned int x;

	for (x = 0; x < infogb_window_width; x++ )
	{
    gfx_buffer[y * infogb_window_width * GFX_BYTEDEPTH + x * GFX_BYTEDEPTH + 1] 		
			= rgbtab[ index[x] & 0x00FF ] >> 8;
    gfx_buffer[y * infogb_window_width * GFX_BYTEDEPTH + x * GFX_BYTEDEPTH ] 
			= rgbtab[ index[x] & 0x00FF ] & 0xFF;
	}
}

/* key */

int infogb_poll_events()
{
    static MSG msg;

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_QUIT:
#if 0
					  joypad_press(GBE_O_ESCAPE);
            joypad_release(GBE_O_ESCAPE);
#endif
            return 0;

        case WM_KEYDOWN:
        case WM_KEYUP:
            keyboard_handler(msg.wParam, msg.message == WM_KEYDOWN);
            return 1;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 1;
}

void infogb_wait()
{
	/* wait */
	unsigned __int64 endTime;
  double secs;

	// Get the final tick count
	QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
	// Get number of ticks
	secs = (DWORD)((endTime - midTime) / ticksPerSecond);
  midTime = endTime;

	if ( secs < VBLANK_INT ) {
		Sleep( (DWORD)(VBLANK_INT - secs) );
	}
}

static void keyboard_handler(WPARAM code, BOOL press)
{
    int button = 0;

    switch ( code )
    {
    case VK_RETURN:
        button = GB_START;
        break;

    case VK_TAB:
        button = GB_SELECT;
        break;

    case VK_CONTROL:
        button = GB_B;
        break;

    case VK_SPACE:
        button = GB_A;
        break;

    case VK_LEFT:
        button = GB_LEFT;
        break;

    case VK_RIGHT:
        button = GB_RIGHT;
        break;

    case VK_UP:
        button = GB_UP;
        break;

    case VK_DOWN:
        button = GB_DOWN;
        break;

    }

    if ( button )
    {
        if ( press )
        {
            joypad_press( button );
        }
        else
        {
            joypad_release( button );
        }
    }
}

/* sound */

DIRSOUND* ds;
WORD lch, rch;

#define AUDIOBUFFER 4096

static int sample_pos = 0;
static int buffer_pos = 0;

void infogb_open_audio()
{
	ds = new DIRSOUND( g_hwnd );

	lch = ds->AllocChannel();
	rch = ds->AllocChannel();

	ds->sound[lch] = new BYTE[ AUDIOBUFFER ];
	ds->sound[rch] = new BYTE[ AUDIOBUFFER ];

	ds->len[lch] = AUDIOBUFFER;
	ds->len[rch] = AUDIOBUFFER;

  ds->CreateBuffer( lch );
	ds->CreateBuffer( rch );

	ds->Start( lch, TRUE );
	ds->Start( rch, TRUE );
}

void infogb_write_sample(short int l, short int r)
{
	if ( ds == NULL )
		return;

	ds->sound[lch][sample_pos] = (l >> 8);
 	ds->sound[rch][sample_pos] = (r >> 8); 
  sample_pos++;

  if (sample_pos >= AUDIOBUFFER) {
		ds->FillBuffer( lch, buffer_pos );
		ds->FillBuffer( rch, buffer_pos );	
    sample_pos = 0;
		if ( ++buffer_pos == ds_LOOPS ) {
			buffer_pos = 0;
		}
	}
}

void infogb_close_audio()
{
	ds->Stop(lch);
	ds->Stop(rch);

  delete ds;
}

/* end of win32.c */