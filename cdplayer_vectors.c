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

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef EXEC_INTERFACES_H
#include <exec/interfaces.h>
#endif
#ifndef LIBRARIES_CDPLAYER_H
#include <libraries/cdplayer.h>
#endif

BYTE _CDPlayer_CDEject(struct CDPlayerIFace *, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDPlay(struct CDPlayerIFace *, UBYTE starttrack, UBYTE endtrack, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDResume(struct CDPlayerIFace *, BOOL mode, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDStop(struct CDPlayerIFace *, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDJump(struct CDPlayerIFace *, LONG blocks, struct IOStdReq *io_ptr);
BOOL _CDPlayer_CDActive(struct CDPlayerIFace *, struct IOStdReq *io_ptr);
ULONG _CDPlayer_CDCurrentTitle(struct CDPlayerIFace *, struct IOStdReq * io_ptr);
BYTE _CDPlayer_CDTitleTime(struct CDPlayerIFace *, struct CD_Time *cd_time, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDGetVolume(struct CDPlayerIFace *, struct CD_Volume *vol, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDSetVolume(struct CDPlayerIFace *, const struct CD_Volume *vol, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDReadTOC(struct CDPlayerIFace *, struct CD_TOC *toc, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDInfo(struct CDPlayerIFace *, struct CD_Info *cdi, struct IOStdReq *io_ptr);
BYTE _CDPlayer_CDPlayAddr(struct CDPlayerIFace *, ULONG startaddr, ULONG endaddr, struct IOStdReq *io_ptr);

STATIC CONST APTR main_vectors[] = {
	(APTR)_generic_Obtain,
	(APTR)_generic_Release,
	NULL,
	NULL,
	(APTR)_CDPlayer_CDEject,
	(APTR)_CDPlayer_CDPlay,
	(APTR)_CDPlayer_CDResume,
	(APTR)_CDPlayer_CDStop,
	(APTR)_CDPlayer_CDJump,
	(APTR)_CDPlayer_CDActive,
	(APTR)_CDPlayer_CDCurrentTitle,
	(APTR)_CDPlayer_CDTitleTime,
	(APTR)_CDPlayer_CDGetVolume,
	(APTR)_CDPlayer_CDSetVolume,
	(APTR)_CDPlayer_CDReadTOC,
	(APTR)_CDPlayer_CDInfo,
	(APTR)_CDPlayer_CDPlayAddr,
	(APTR)-1
};
