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

#ifndef __DIRSOUND_H__
#define __DIRSOUND_H__

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include <mmsystem.h>
#include "dsound.h"

/*-------------------------------------------------------------------*/
/*  Constants for DirectSound                                        */
/*-------------------------------------------------------------------*/

#define ds_NUMCHANNELS      8
#define ds_CHANSPERSAMPLE   1
#define ds_BITSPERSAMPLE	  8

#define ds_SAMPLERATE 		  22050
#define ds_LOOPS				2

/*-------------------------------------------------------------------*/
/*  Class Definitions                                                */
/*-------------------------------------------------------------------*/
class DIRSOUND
{
	public:
    /*-------------------------------------------------------------------*/
    /*  Constructor/Destructor                                           */
    /*-------------------------------------------------------------------*/
		DIRSOUND(HWND hwnd);
		~DIRSOUND();

    /*-------------------------------------------------------------------*/
    /*  Global Functions                                                 */
    /*-------------------------------------------------------------------*/
		void UnLoadWave(WORD channel);
		void Start(WORD channel, BOOL looping);
		void Stop(WORD channel);

    /*-------------------------------------------------------------------*/
    /*  Local Functions                                                  */
    /*-------------------------------------------------------------------*/
	  WORD AllocChannel(void);
		void CreateBuffer(WORD channel);
		void DestroyBuffer(WORD channel);
		void FillBuffer(WORD channel, int n);

    /*-------------------------------------------------------------------*/
    /*  Local Variables                                                  */
    /*-------------------------------------------------------------------*/
		HWND					hwnd; 			/* Window handle to application */
	  LPDIRECTSOUND lpdirsnd;

		/* Used for management of each sound channel  */
		BYTE								 *sound[ds_NUMCHANNELS];
		DWORD 							 len[ds_NUMCHANNELS];
		LPDIRECTSOUNDBUFFER  lpdsb[ds_NUMCHANNELS];
};
#endif /* __DIRSOUND_H__ */
