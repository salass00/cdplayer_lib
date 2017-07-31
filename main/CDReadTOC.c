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

BYTE _CDPlayer_CDReadTOC(struct CDPlayerIFace *Self, struct CD_TOC *toc, struct IOStdReq *io_ptr) {
	struct CDPlayerBase *libBase  = (struct CDPlayerBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
	UBYTE                cmd[10]  = { 0x43, 0, 0, 0, 0, 0, 0, 0x03, 0x24, 0 };
	UBYTE                buf[804];
	BYTE                 error;
	ULONG                tocsize, num_tracks, track, bp;

	IUtility->ClearMem(buf, sizeof(buf));
	IUtility->ClearMem(toc, sizeof(*toc));

	error = DoSCSICmd(libBase, io_ptr, cmd, sizeof(cmd),
		buf, sizeof(buf), SCSIF_READ|SCSIF_AUTOSENSE);
	if (error)
		return error;

	tocsize = ((ULONG)buf[0] << 8) | (ULONG)buf[1];
	if (tocsize < 10)
		return TDERR_DiskChanged;

	num_tracks = ((tocsize - 2) >> 3) - 1;
	toc->cdc_NumTracks = num_tracks;

	for (track = 0; track <= num_tracks; track++) {
		bp = 4 + (track << 3);
		toc->cdc_Flags[track] = (buf[bp + 1] & 4) >> 2;
		toc->cdc_Addr[track] = ((ULONG)buf[bp + 4] << 24) | ((ULONG)buf[bp + 5] << 16) | ((ULONG)buf[bp + 6] << 8) | (ULONG)buf[bp + 7];
	}

	return IOERR_SUCCESS;
}
