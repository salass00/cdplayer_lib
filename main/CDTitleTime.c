/*
 * Copyright (C) 2011-2017 Fredrik Wikstrom <fredrik@a500.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "cdplayer_private.h"

BYTE _CDPlayer_CDTitleTime(struct CDPlayerIFace *Self, struct CD_Time *cd_time, struct IOStdReq *io_ptr) {
	struct CDPlayerBase *libBase = (struct CDPlayerBase *)Self->Data.LibBase;
	struct ExecIFace    *IExec = libBase->IExec;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct CD_TOC        toc;
	BYTE                 error;
	struct Player       *player;
	struct PlayerCmd     playercmd;
	BOOL                 success;
	ULONG                addr, track, num_tracks;

	IUtility->ClearMem(cd_time, sizeof(*cd_time));

	error = Self->CDReadTOC(&toc, io_ptr);
	if (error)
		return TDERR_DiskChanged;

	playercmd.Command = CDPLAY_GET_ADDR;

	IExec->ObtainSemaphoreShared(libBase->PlayerSemaphore);
	player = FindPlayer(libBase, io_ptr);
	success = DoPlayerCommand(libBase, player, &playercmd);
	IExec->ReleaseSemaphore(libBase->PlayerSemaphore);

	if (success)
		addr = playercmd.Content.Addr;
	else
		addr = 0;

	num_tracks = toc.cdc_NumTracks;
	track = GetTrackNumber(&toc, addr);
	if (track < 1)
		addr = 0;

	if (track >= 1 && track <= num_tracks) {
		cd_time->cdt_TrackCurBase = addr - toc.cdc_Addr[track-1];
		cd_time->cdt_TrackRemainBase = toc.cdc_Addr[track] - addr;
		cd_time->cdt_TrackCompleteBase = toc.cdc_Addr[track] - toc.cdc_Addr[track-1];
	}

	if (num_tracks >= 1) {
		cd_time->cdt_AllCurBase = addr;
		cd_time->cdt_AllRemainBase = toc.cdc_Addr[num_tracks] - addr;
		cd_time->cdt_AllCompleteBase = toc.cdc_Addr[num_tracks] - toc.cdc_Addr[0];
	}

	return IOERR_SUCCESS;
}
