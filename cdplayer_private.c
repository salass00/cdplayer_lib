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
#include <devices/ahi.h>

#define FRAME_SIZE       2352

#define CDDA_FRAMES      150
#define CDDA_BUFFER_SIZE (FRAME_SIZE*CDDA_FRAMES)

#define AHI_FRAMES       15
#define AHI_BUFFER_SIZE  (FRAME_SIZE*AHI_FRAMES)

#define ADDR2MSF(x,m,s,f) do { \
	m = ((x) / 75UL) / 60UL; \
	s = ((x) / 75UL) % 60UL; \
	f = (x) % 75UL; \
	} while (0)

#ifndef MIN
#define MIN(a,b) ((a)<=(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>=(b)?(a):(b))
#endif

#define SWAP_PTR(a,b) { APTR tmp_ptr = a; a = b; b = tmp_ptr; }

ULONG GetTrackNumber(const struct CD_TOC *toc, const ULONG addr) {
	const ULONG num_tracks = toc->cdc_NumTracks;
	ULONG       track;

	for (track = 0; track < num_tracks; track++) {
		if (addr >= toc->cdc_Addr[track] && addr < toc->cdc_Addr[track + 1]) {
			return track + 1;
		}
	}
	return 0;
}

BYTE DoSCSICmd(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr,
	APTR cmd, ULONG cmd_len, APTR buf, ULONG buf_len, ULONG flags)
{
	struct ExecIFace    *IExec    = libBase->IExec;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct SCSICmd       scsicmd;
	UBYTE                scsisense[252];

	if (!io_ptr->io_Device) {
		return IOERR_OPENFAIL;
	}

	IUtility->ClearMem(&scsicmd, sizeof(scsicmd));

	scsicmd.scsi_Data        = buf;
	scsicmd.scsi_Length      = buf_len;
	scsicmd.scsi_SenseData   = scsisense;
	scsicmd.scsi_SenseLength = sizeof(scsisense);
	scsicmd.scsi_Command     = cmd;
	scsicmd.scsi_CmdLength   = cmd_len;
	scsicmd.scsi_Flags       = flags;

	io_ptr->io_Command = HD_SCSICMD;
	io_ptr->io_Data    = &scsicmd;
	io_ptr->io_Length  = sizeof(scsicmd);

	return IExec->DoIO((struct IORequest *)io_ptr);
}

struct Player *FindPlayer(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr) {
	struct ExecIFace *IExec = libBase->IExec;
	struct Player    *player;

	player = (struct Player *)IExec->GetHead(libBase->Players);
	while (player != NULL) {
		if (player->Device == io_ptr->io_Device &&
			player->Unit == io_ptr->io_Unit)
		{
			return player;
		}
		player = (struct Player *)IExec->GetSucc((struct Node *)player);
	}

	return NULL;
}

struct Player *NewPlayer(struct CDPlayerBase *libBase, struct IOStdReq *io_ptr) {
	struct ExecIFace    *IExec    = libBase->IExec;
	struct DOSIFace     *IDOS     = libBase->IDOS;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct Player       *player;

	player = IExec->AllocSysObjectTags(ASOT_NODE,
		ASONODE_Min,  TRUE,
		ASONODE_Size, sizeof(*player),
		TAG_END);

	if (player != NULL) {
		IUtility->ClearMem(&player->MinNode + 1, sizeof(*player) - sizeof(player->MinNode));

		player->Device = io_ptr->io_Device;
		player->Unit   = io_ptr->io_Unit;

		player->Volume.Chan[0] = CHAN0_DEF_VOLUME;
		player->Volume.Chan[1] = CHAN1_DEF_VOLUME;
		player->Volume.Chan[2] = CHAN2_DEF_VOLUME;
		player->Volume.Chan[3] = CHAN3_DEF_VOLUME;

		player->MsgMutex = IExec->AllocSysObject(ASOT_MUTEX, NULL);

		player->ReplyPort = IExec->AllocSysObjectTags(ASOT_PORT,
			ASOPORT_Signal,   SIGB_CHILD,
			ASOPORT_Action,   PA_IGNORE,
			TAG_END);

		player->PlayerMsg = IExec->AllocSysObjectTags(ASOT_MESSAGE,
			ASOMSG_Size,      sizeof(*player->PlayerMsg),
			ASOMSG_ReplyPort, player->ReplyPort,
			TAG_END);

		player->DeathMsg = IExec->AllocSysObjectTags(ASOT_MESSAGE,
			ASOMSG_Size,      sizeof(*player->DeathMsg),
			ASOMSG_ReplyPort, player->ReplyPort,
			TAG_END);

		if (player->MsgMutex != NULL &&
			player->ReplyPort != NULL &&
			player->PlayerMsg != NULL &&
			player->DeathMsg != NULL)
		{
			BPTR stdin, stdout, stderr;
		
			stdin = IDOS->Open("NIL:", MODE_OLDFILE);
			stdout = IDOS->Open("NIL:", MODE_NEWFILE);
			stderr = IDOS->Open("NIL:", MODE_NEWFILE);

			if (stdin != ZERO && stdout != ZERO && stderr != ZERO) {
				player->PlayerProc = IDOS->CreateNewProcTags(
					NP_Name,                 "cdplayer.library player process",
					NP_StackSize,            16384,
					NP_Input,                stdin,
					NP_Output,               stdout,
					NP_Error,                stderr,
					NP_CloseError,           TRUE,
					NP_CurrentDir,           ZERO,
					NP_Entry,                PlayerProcEntry,
					NP_Priority,             5,
					NP_NotifyOnDeathMessage, player->DeathMsg,
					TAG_END);
			}

			if (player->PlayerProc != NULL) {
				struct MsgPort   *replyport = player->ReplyPort;
				struct PlayerMsg *playermsg = player->PlayerMsg;
				struct PlayerMsg *replymsg;

				replyport->mp_SigTask = IExec->FindTask(NULL);
				replyport->mp_SigBit  = SIGF_CHILD;
				replyport->mp_Flags   = PA_SIGNAL;

				playermsg->Command = CDPLAY_START;

				playermsg->Content.Start.LibBase = libBase;
				playermsg->Content.Start.Player  = player;
				playermsg->Content.Start.IOPtr   = io_ptr;

				IExec->PutMsg(IDOS->GetProcMsgPort(player->PlayerProc), &playermsg->Msg);
				IExec->WaitPort(replyport);
				replymsg = (struct PlayerMsg *)IExec->GetMsg(replyport);

				replyport->mp_Flags = PA_IGNORE;

				/* Check that it's not DeathMessage */
				if (replymsg == playermsg) {
					IExec->AddTail(libBase->Players, (struct Node *)player);

					return player;
				} else {
					player->PlayerProc = NULL;
				}
			} else {
				IDOS->Close(stdin);
				IDOS->Close(stdout);
				IDOS->Close(stderr);
			}
		}

		FreePlayer(libBase, player);
	}

	return NULL;
}

void KillPlayer(struct CDPlayerBase *libBase, struct Player *player) {
	struct ExecIFace *IExec = libBase->IExec;

	if (player != NULL) {
		struct MsgPort   *replyport = player->ReplyPort;
		struct PlayerMsg *playermsg = player->PlayerMsg;

		IExec->Remove((struct Node *)player);

		replyport->mp_SigTask = IExec->FindTask(NULL);
		replyport->mp_SigBit  = SIGF_CHILD;
		replyport->mp_Flags   = PA_SIGNAL;

		playermsg->Command = CDPLAY_DIE;

		IExec->PutMsg(player->PlayerPort, &playermsg->Msg);
		IExec->WaitPort(replyport);
		IExec->GetMsg(replyport);

		replyport->mp_Flags = PA_IGNORE;

		FreePlayer(libBase, player);
	}
}

void FreePlayer(struct CDPlayerBase *libBase, struct Player *player) {
	struct ExecIFace *IExec = libBase->IExec;

	if (player != NULL) {
		IExec->FreeSysObject(ASOT_MESSAGE, player->PlayerMsg);
		IExec->FreeSysObject(ASOT_MESSAGE, player->DeathMsg);
		IExec->FreeSysObject(ASOT_PORT, player->ReplyPort);
		IExec->FreeSysObject(ASOT_MUTEX, player->MsgMutex);
		IExec->FreeSysObject(ASOT_NODE, player);
	}
}

BOOL DoPlayerCommand(struct CDPlayerBase *libBase, struct Player *player, struct PlayerCmd *playercmd) {
	struct ExecIFace *IExec = libBase->IExec;

	if (player != NULL) {
		struct MsgPort   *replyport = player->ReplyPort;
		struct PlayerMsg *playermsg = player->PlayerMsg;

		IExec->MutexObtain(player->MsgMutex);

		replyport->mp_SigTask = IExec->FindTask(NULL);
		replyport->mp_SigBit  = SIGF_CHILD;
		replyport->mp_Flags   = PA_SIGNAL;

		IExec->CopyMem(playercmd, &playermsg->Command, sizeof(*playercmd));

		IExec->PutMsg(player->PlayerPort, &playermsg->Msg);
		IExec->WaitPort(replyport);
		IExec->GetMsg(replyport);

		IExec->CopyMem(&playermsg->Command, playercmd, sizeof(*playercmd));

		replyport->mp_Flags = PA_IGNORE;

		IExec->MutexRelease(player->MsgMutex);

		return TRUE;
	}

	return FALSE;
}

static inline WORD readpcmle16(WORD *ptr) {
	WORD res;

	asm("lhbrx %0,%y1"
		: "=r" (res)
		: "Z" (*ptr));

	return res;
}

static inline void DecodeCDDA(WORD *src, WORD *dst, ULONG bytes, UBYTE vl, UBYTE vr) {
	ULONG i, samples = bytes >> 2;

	if (vl == 255) {
		WORD *src_l = src;
		WORD *dst_l = dst;
		for (i = 0; i < samples; i++) {
			*dst_l = readpcmle16(src_l);
			src_l += 2; dst_l += 2;
		}
	} else if (vl == 0) {
		WORD *dst_l = dst;
		for (i = 0; i < samples; i++) {
			*dst_l = 0;
			dst_l += 2;
		}
	} else {
		WORD *src_l = src;
		WORD *dst_l = dst;
		for (i = 0; i < samples; i++) {
			*dst_l = ((LONG)(readpcmle16(src_l)) * (LONG)vl) >> 8;
			src_l += 2; dst_l += 2;
		}
	}

	if (vr == 255) {
		WORD *src_r = src + 1;
		WORD *dst_r = dst + 1;
		for (i = 0; i < samples; i++) {
			*dst_r = readpcmle16(src_r);
			src_r += 2; dst_r += 2;
		}
	} else if (vr == 0) {
		WORD *dst_r = dst + 1;
		for (i = 0; i < samples; i++) {
			*dst_r = 0;
			dst_r += 2;
		}
	} else {
		WORD *src_r = src + 1;
		WORD *dst_r = dst + 1;
		for (i = 0; i < samples; i++) {
			*dst_r = ((LONG)(readpcmle16(src_r)) * (LONG)vr) >> 8;
			src_r += 2; dst_r += 2;
		}
	}
}

int PlayerProcEntry(STRPTR argstr, LONG arglen, struct ExecBase *SysBase) {
	int                  return_value = RETURN_FAIL;
	struct ExecIFace    *IExec = (struct ExecIFace *)SysBase->MainInterface;
	struct Process      *playerproc;
	struct PlayerMsg    *playermsg;
	struct Player       *player = NULL;
	struct MsgPort      *diskport = NULL;
	struct IOStdReq     *diskio = NULL;
	struct MsgPort      *ahiport = NULL;
	struct AHIRequest   *ahiio = NULL, *ahiio2 = NULL, *join = NULL;
	APTR                 ahibuf = NULL, ahibuf2 = NULL;
	APTR                 cddabuf = NULL, cddabuf2 = NULL;
	BOOL                 diskio_busy = FALSE;
	ULONG                cdda_addr = -1, cdda_pos = 0, cdda_frames = 0;
	struct MsgPort      *playerport;
	BOOL                 playing = FALSE;
	ULONG                startaddr = -1, addr = -1, endaddr = -1;
	struct SCSICmd       scsicmd = { 0 };
	UBYTE                scsisense[20];
	UBYTE                readcmd[12] = { 0xbe, 0x04, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0 };

	playerproc = (struct Process *)IExec->FindTask(NULL);

	IExec->WaitPort(&playerproc->pr_MsgPort);
	playermsg = (struct PlayerMsg *)IExec->GetMsg(&playerproc->pr_MsgPort);
	if (playermsg->Command != CDPLAY_START)
		goto error;

	player  = playermsg->Content.Start.Player;

	player->PlayerPort = IExec->AllocSysObject(ASOT_PORT, NULL);
	if (player->PlayerPort == NULL)
		goto error;

	diskport = IExec->AllocSysObject(ASOT_PORT, NULL);
	if (diskport == NULL)
		goto error;

	diskio = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_Duplicate,	playermsg->Content.Start.IOPtr,
		ASOIOR_Size,		sizeof(*diskio),
		ASOIOR_ReplyPort,	diskport,
		TAG_END);
	if (diskio == NULL)
		goto error;

	ahiport = IExec->AllocSysObject(ASOT_PORT, NULL);
	if (ahiport == NULL)
		goto error;

	ahiio = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_Size,		sizeof(*ahiio),
		ASOIOR_ReplyPort,	ahiport,
		TAG_END);
	if (ahiio == NULL)
		goto error;

	if (IExec->OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)ahiio, 0) != IOERR_SUCCESS) {
		ahiio->ahir_Std.io_Device = NULL;
		goto error;
	}

	ahiio2 = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_Duplicate,	ahiio,
		TAG_END);
	if (ahiio2 == NULL)
		goto error;

	ahibuf   = IExec->AllocVecTags(AHI_BUFFER_SIZE, AVT_Type, MEMF_SHARED, TAG_END);
	ahibuf2  = IExec->AllocVecTags(AHI_BUFFER_SIZE, AVT_Type, MEMF_SHARED, TAG_END);
	cddabuf  = IExec->AllocVecTags(CDDA_BUFFER_SIZE, AVT_Type, MEMF_SHARED, TAG_END);
	cddabuf2 = IExec->AllocVecTags(CDDA_BUFFER_SIZE, AVT_Type, MEMF_SHARED, TAG_END);

	if (ahibuf == NULL || ahibuf2 == NULL || cddabuf == NULL || cddabuf2 == NULL)
		goto error;

	IExec->ReplyMsg((struct Message *)playermsg);

	playerport = player->PlayerPort;

	scsicmd.scsi_SenseData   = scsisense;
	scsicmd.scsi_SenseLength = sizeof(scsisense);
	scsicmd.scsi_Command     = readcmd;
	scsicmd.scsi_CmdLength   = sizeof(readcmd);
	scsicmd.scsi_Flags       = SCSIF_READ|SCSIF_AUTOSENSE;

	diskio->io_Command = HD_SCSICMD;
	diskio->io_Data    = &scsicmd;
	diskio->io_Length  = sizeof(scsicmd);

	while (TRUE) {
		if (playing == FALSE)
			IExec->Wait(1 << playerport->mp_SigBit);

		while ((playermsg = (struct PlayerMsg *)IExec->GetMsg(playerport)) != NULL) {
			switch (playermsg->Command) {
				case CDPLAY_DIE:
					goto exit;
				case CDPLAY_PLAY:
					startaddr = addr = cdda_addr = playermsg->Content.Play.StartAddr;
					endaddr = playermsg->Content.Play.EndAddr;
					if (endaddr > addr) {
						playing = TRUE;
					} else {
						playing = FALSE;
						startaddr = addr = endaddr = cdda_addr = -1;
					}
					cdda_frames = 0;
					break;
				case CDPLAY_ACTIVE:
					playermsg->Content.Active = playing;
					break;
				case CDPLAY_GET_ADDR:
					playermsg->Content.Addr = addr;
					break;
				case CDPLAY_STOP:
					if (playing) {
						if (join) {
							IExec->AbortIO((struct IORequest *)join);
							IExec->WaitIO((struct IORequest *)join);
							join = NULL;
						}
						if (diskio_busy) {
							IExec->AbortIO((struct IORequest *)diskio);
							IExec->WaitIO((struct IORequest *)diskio);
							diskio_busy = FALSE;
						}
						playing = FALSE;
						startaddr = addr = endaddr = cdda_addr = -1;
						cdda_frames = 0;
					}
					break;
				case CDPLAY_PAUSE:
					if (playing) {
						if (diskio_busy) {
							IExec->AbortIO((struct IORequest *)diskio);
							IExec->WaitIO((struct IORequest *)diskio);
							diskio_busy = FALSE;
						}
						playing = FALSE;
						cdda_addr = addr;
						cdda_frames = 0;
					}
					break;
				case CDPLAY_RESUME:
					if (!playing && addr != (ULONG)-1 && addr >= startaddr && addr < endaddr) {
						playing = TRUE;
					}
					break;
				case CDPLAY_SET_ADDR:
					if (addr != (ULONG)-1 && addr >= startaddr && addr < endaddr) {
						ULONG newaddr = playermsg->Content.Addr;
						if (newaddr >= startaddr && newaddr <= endaddr) {
							if (diskio_busy) {
								IExec->AbortIO((struct IORequest *)diskio);
								IExec->WaitIO((struct IORequest *)diskio);
								diskio_busy = FALSE;
							}
							cdda_addr = addr = newaddr;
							cdda_frames = 0;
						}
					}
					break;
			}
			IExec->ReplyMsg((struct Message *)playermsg);
		}

		if (playing) {
			if (!cdda_frames) {
				ULONG frames, bytes;

				cdda_pos = 0;

				if (diskio_busy) {
					IExec->WaitIO((struct IORequest *)diskio);

					diskio_busy = FALSE;

					SWAP_PTR(cddabuf, cddabuf2);
				} else {
					frames = MIN(CDDA_FRAMES, endaddr - cdda_addr);
					bytes = frames * FRAME_SIZE;

					scsicmd.scsi_Data   = cddabuf;
					scsicmd.scsi_Length = bytes;

					readcmd[2] = cdda_addr >> 24;
					readcmd[3] = cdda_addr >> 16;
					readcmd[4] = cdda_addr >> 8;
					readcmd[5] = cdda_addr;
					readcmd[8] = frames;

					IExec->DoIO((struct IORequest *)diskio);
					cdda_addr += frames;
				}

				if (diskio->io_Error == IOERR_SUCCESS) {
					cdda_frames = CDDA_FRAMES;
					frames = MIN(CDDA_FRAMES, endaddr - cdda_addr);
					bytes = frames * FRAME_SIZE;

					scsicmd.scsi_Data = cddabuf2;
					scsicmd.scsi_Length = bytes;

					readcmd[2] = cdda_addr >> 24;
					readcmd[3] = cdda_addr >> 16;
					readcmd[4] = cdda_addr >> 8;
					readcmd[5] = cdda_addr;
					readcmd[8] = frames;

					IExec->SendIO((struct IORequest *)diskio);
					cdda_addr  += frames;
					diskio_busy = TRUE;
				}
			}

			if (cdda_frames) {
				ULONG        frames = MIN(AHI_FRAMES, endaddr - addr);
				ULONG        bytes = frames * FRAME_SIZE;
				union Volume volume;

				volume.Chan01 = player->Volume.Chan01;

				DecodeCDDA((WORD *)((UBYTE *)cddabuf + (cdda_pos * FRAME_SIZE)),
					ahibuf, bytes, volume.Chan[1], volume.Chan[0]);

				ahiio->ahir_Std.io_Command = CMD_WRITE;
				ahiio->ahir_Std.io_Offset  = 0;
				ahiio->ahir_Std.io_Data    = ahibuf;
				ahiio->ahir_Std.io_Length  = bytes;
				ahiio->ahir_Type           = AHIST_S16S;
				ahiio->ahir_Frequency      = 44100;
				ahiio->ahir_Volume         = 0x10000;
				ahiio->ahir_Position       = 0x8000;
				ahiio->ahir_Link           = join;

				IExec->SendIO((struct IORequest *)ahiio);

				if (join != NULL)
					IExec->WaitIO((struct IORequest *)join);

				join = ahiio;

				SWAP_PTR(ahiio, ahiio2);
				SWAP_PTR(ahibuf, ahibuf2);

				addr        += frames;
				cdda_pos    += frames;
				cdda_frames -= frames;

				if (addr == endaddr) {
					playing = FALSE;
					startaddr = addr = endaddr = cdda_addr = -1;
					cdda_pos = cdda_frames = 0;
				}
			} else {
				playing = FALSE;
				startaddr = addr = endaddr = cdda_addr = -1;
				cdda_pos = cdda_frames = 0;
			}

			if (playing == FALSE && diskio_busy) {
				IExec->AbortIO((struct IORequest *)diskio);
				IExec->WaitIO((struct IORequest *)diskio);
				diskio_busy = FALSE;
			}
		}
	}

exit:
	return_value = RETURN_OK;

error:
	if (ahiio != NULL && ahiio->ahir_Std.io_Device != NULL) {
		if (join) {
			IExec->AbortIO((struct IORequest *)join);
			IExec->WaitIO((struct IORequest *)join);
		}
		IExec->CloseDevice((struct IORequest *)ahiio);
	}

	if (diskio != NULL && diskio->io_Device != NULL) {
		if (diskio_busy) {
			IExec->AbortIO((struct IORequest *)diskio);
			IExec->WaitIO((struct IORequest *)diskio);
		}
		/* We didn't open the trackdisk driver,
		 * so it's not up to us to close it...
		 */
	}

	IExec->FreeVec(ahibuf);
	IExec->FreeVec(ahibuf2);

	IExec->FreeVec(cddabuf);
	IExec->FreeVec(cddabuf2);

	IExec->FreeSysObject(ASOT_IOREQUEST, ahiio2);
	IExec->FreeSysObject(ASOT_IOREQUEST, ahiio);
	IExec->FreeSysObject(ASOT_PORT, ahiport);

	IExec->FreeSysObject(ASOT_IOREQUEST, diskio);
	IExec->FreeSysObject(ASOT_PORT, diskport);

	if (player != NULL) {
		IExec->FreeSysObject(ASOT_PORT, player->PlayerPort);
	}

	return return_value;
}
