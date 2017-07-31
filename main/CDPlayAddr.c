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

BYTE _CDPlayer_CDPlayAddr(struct CDPlayerIFace *Self, ULONG startaddr, ULONG endaddr, struct IOStdReq *io_ptr) {
	struct CDPlayerBase *libBase = (struct CDPlayerBase *)Self->Data.LibBase;
	struct ExecIFace    *IExec   = libBase->IExec;
	struct CD_TOC        toc;
	BYTE                 error;
	struct Player       *player;
	struct PlayerCmd     playercmd;
	BOOL                 success;
	
	error = Self->CDReadTOC(&toc, io_ptr);
	if (error)
		return error;

	if (startaddr > endaddr || startaddr < toc.cdc_Addr[0] || endaddr > toc.cdc_Addr[toc.cdc_NumTracks])
		return IOERR_BADADDRESS;

	playercmd.Command = CDPLAY_PLAY;

	playercmd.Content.Play.StartAddr = startaddr;
	playercmd.Content.Play.EndAddr   = endaddr;

	IExec->ObtainSemaphoreShared(libBase->PlayerSemaphore);
	player = FindPlayer(libBase, io_ptr);
	if (!player) {
		IExec->ReleaseSemaphore(libBase->PlayerSemaphore);

		IExec->ObtainSemaphore(libBase->PlayerSemaphore);
		player = NewPlayer(libBase, io_ptr);
		IExec->ReleaseSemaphore(libBase->PlayerSemaphore);

		IExec->ObtainSemaphoreShared(libBase->PlayerSemaphore);
		player = FindPlayer(libBase, io_ptr);
	}
	success = DoPlayerCommand(libBase, player, &playercmd);
	IExec->ReleaseSemaphore(libBase->PlayerSemaphore);

	return success ? 0 : IOERR_OPENFAIL;
}
