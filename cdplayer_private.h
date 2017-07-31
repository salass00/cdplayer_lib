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

#ifndef CDPLAYER_PRIVATE_H
#define CDPLAYER_PRIVATE_H

#include <exec/exec.h>
#include <dos/dos.h>
#include <libraries/cdplayer.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/cdplayer.h>

struct CDPlayerBase {
	struct Library       LibNode;
	BPTR                 SegList;
	struct ExecIFace    *IExec;
	struct DOSIFace     *IDOS;
	struct UtilityIFace *IUtility;
	struct Library      *DOSBase;
	struct Library      *UtilityBase;
	APTR                 PlayerSemaphore;
	struct List         *Players;
};

enum {
	CDPLAY_START,
	CDPLAY_DIE,
	CDPLAY_PLAY,
	CDPLAY_ACTIVE,
	CDPLAY_GET_ADDR,
	CDPLAY_STOP,
	CDPLAY_PAUSE,
	CDPLAY_RESUME,
	CDPLAY_SET_ADDR
};

union PlayerCmdContent {
	struct {
		struct CDPlayerBase *LibBase;
		struct Player       *Player;
		struct IOStdReq     *IOPtr;
	} Start;
	struct {
		ULONG StartAddr;
		ULONG EndAddr;
	} Play;

	BOOL  Active;
	ULONG Addr;
};

struct PlayerCmd {
	ULONG                  Command;
	union PlayerCmdContent Content;
};

struct PlayerMsg {
	struct Message         Msg;
	ULONG                  Command;
	union PlayerCmdContent Content;
};

union Volume {
	UBYTE Chan[4];
	UWORD Chan01;
	ULONG Chan0123;
};

struct Player {
	struct MinNode       MinNode;
	struct Device       *Device;
	struct Unit         *Unit;
	struct Process      *PlayerProc;
	APTR                 MsgMutex;
	struct MsgPort      *ReplyPort;
	struct MsgPort      *PlayerPort;
	struct PlayerMsg    *PlayerMsg;
	struct DeathMessage *DeathMsg;
	union Volume         Volume;
};

#define CHAN0_DEF_VOLUME 255
#define CHAN1_DEF_VOLUME 255
#define CHAN2_DEF_VOLUME 0
#define CHAN3_DEF_VOLUME 0

/* cdplayer_private.c */
ULONG GetTrackNumber(const struct CD_TOC *toc, const ULONG addr);
BYTE DoSCSICmd(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr,
	APTR cmd, ULONG cmd_len, APTR buf, ULONG buf_len, ULONG flags);
struct Player *FindPlayer(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr);
struct Player *NewPlayer(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr);
void KillPlayer(struct CDPlayerBase *libBase, struct Player *player);
void FreePlayer(struct CDPlayerBase *libBase, struct Player *player);
BOOL DoPlayerCommand(struct CDPlayerBase *libBase, struct Player *player, struct PlayerCmd *playercmd);
int PlayerProcEntry(STRPTR argstr, LONG arglen, struct ExecBase *SysBase);

#endif
