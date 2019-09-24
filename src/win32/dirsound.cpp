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

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include "dsound.h"
#include "dirsound.h"
#include "../system.h"

/*-------------------------------------------------------------------*/
/*  Constructor                                                      */
/*-------------------------------------------------------------------*/
DIRSOUND::DIRSOUND(HWND hwnd)
{
	DWORD ret;
	WORD x;

	for ( x = 0;x < ds_NUMCHANNELS; x++ )
	{
		lpdsb[x] = NULL;
	}

  // init DirectSound
	ret = DirectSoundCreate(NULL, &lpdirsnd, NULL);

	if (ret != DS_OK)
	{
    printf( "Sound Card is needed to execute this application." );
		exit(-1);
	}

  // set cooperative level
#if 1
  ret = lpdirsnd->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
#else
  ret = lpdirsnd->SetCooperativeLevel( hwnd, DSSCL_NORMAL );
#endif

	if ( ret != DS_OK )
	{
    printf( "SetCooperativeLevel() Failed." );
		exit(-1);
	}
}

/*-------------------------------------------------------------------*/
/*  Destructor                                                       */
/*-------------------------------------------------------------------*/
DIRSOUND::~DIRSOUND()
{
	WORD x;

	for ( x=0; x < ds_NUMCHANNELS; x++ )
	{
		if ( lpdsb[x] != NULL )
		{
			DestroyBuffer( x );

			if (sound[x] != NULL)
			{
				delete sound[x];
			}
		}
	}
	(void)lpdirsnd->Release();	/* Nothing to do, if it errors */
}

/*-------------------------------------------------------------------*/
/*  FillBuffer() : Fill Sound Buffer                                 */
/*-------------------------------------------------------------------*/
void DIRSOUND::FillBuffer( WORD channel, int n )
{
	LPVOID write1;
	DWORD length1;
	LPVOID write2;
	DWORD length2;
	HRESULT hr;

  hr = lpdsb[channel]->Lock( n * len[channel], len[channel], 
    &write1, &length1, &write2, &length2, 0 );

	if (hr == DSERR_BUFFERLOST)
	{
		lpdsb[channel]->Restore();

		hr = lpdsb[channel]->Lock( n * len[channel], len[channel], 
      &write1, &length1, &write2, &length2, 0 );
	}

	if (hr != DS_OK)
	{
    printf( "Lock() Failed." );
		exit(-1);
	}

	CopyMemory( write1, sound[channel], length1 );

	if (write2 != NULL)
	{
		CopyMemory(write2, sound[channel] + length1, length2);
	}

	hr = lpdsb[channel]->Unlock(write1, length1, write2, length2);

	if (hr != DS_OK)
	{
    printf( "Unlock() Failed." );
		exit(-1);
	}
}

/*-------------------------------------------------------------------*/
/*  CreateBuffer() : Create IDirectSoundBuffer                       */
/*-------------------------------------------------------------------*/
void DIRSOUND::CreateBuffer(WORD channel)
{
	DSBUFFERDESC dsbdesc;
	PCMWAVEFORMAT pcmwf;
	HRESULT hr;

	memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
	pcmwf.wf.wFormatTag 		 = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels			 = ds_CHANSPERSAMPLE;
	pcmwf.wf.nSamplesPerSec  = ds_SAMPLERATE;
	pcmwf.wf.nBlockAlign		 = ds_CHANSPERSAMPLE * ds_BITSPERSAMPLE / 8;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample		 = ds_BITSPERSAMPLE;

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize				= sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags 			= 0;
	dsbdesc.dwBufferBytes = len[channel] * ds_LOOPS;
	dsbdesc.lpwfxFormat 	= (LPWAVEFORMATEX)&pcmwf;

	hr = lpdirsnd->CreateSoundBuffer(&dsbdesc, &lpdsb[channel], NULL);

	if (hr != DS_OK)
	{
    printf( "CreateSoundBuffer() Failed." );
		exit(-1);
	}
}

/*-------------------------------------------------------------------*/
/*  DestoryBuffer() : Destory IDirectSoundBuffer                     */
/*-------------------------------------------------------------------*/
void DIRSOUND::DestroyBuffer(WORD channel)
{
	DWORD hr;

	hr = lpdsb[channel]->Release();

	if (hr != DS_OK)
	{
    printf( "Release() Failed." );
		exit(-1);
	}

	lpdsb[channel] = NULL;
}

/*-------------------------------------------------------------------*/
/*  AllocChannel() : Allocate channel for IDirectSoundBuffer         */
/*-------------------------------------------------------------------*/
WORD DIRSOUND::AllocChannel(void)
{
	WORD x;

	for (x=0;x<ds_NUMCHANNELS;x++)
	{
		if (lpdsb[x] == NULL)
		{
			break;
		}
	}

	if ( x == ds_NUMCHANNELS )
	{
    /* No available channel */
    printf( "AllocChannel() Failed." );
		exit(-1); 		
	}

	return (x);
}

/*-------------------------------------------------------------------*/
/*  Play() : Play Sound                                              */
/*-------------------------------------------------------------------*/
void DIRSOUND::Start(WORD channel, BOOL looping)
{
	HRESULT hr;

	hr = lpdsb[channel]->Play( 0, 0, looping == TRUE ? DSBPLAY_LOOPING : 0 );

	if ( hr != DS_OK )
	{
    printf( "Play() Failed." );
		exit(-1);
	}
}

/*-------------------------------------------------------------------*/
/*  Stop() : Stop Sound                                              */
/*-------------------------------------------------------------------*/
void DIRSOUND::Stop(WORD channel)
{
	lpdsb[channel]->Stop();
}

/*-------------------------------------------------------------------*/
/*  UnLoadWave() : Destory Channel for IDirectSoundBuffer            */
/*-------------------------------------------------------------------*/
void DIRSOUND::UnLoadWave(WORD channel)
{
	DestroyBuffer(channel);

	if ( sound[channel] != NULL )
	{
		delete sound[channel];
	}
}

// End of dirsound.cpp