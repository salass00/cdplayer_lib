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

BYTE _CDPlayer_CDInfo(struct CDPlayerIFace *Self, struct CD_Info *cdi, struct IOStdReq *io_ptr) {
	struct CDPlayerBase *libBase  = (struct CDPlayerBase *)Self->Data.LibBase;
	struct ExecIFace    *IExec    = libBase->IExec;
	struct UtilityIFace *IUtility = libBase->IUtility;
	UBYTE                cmd[6]   = { 0x12, 0, 0, 0, 0x38, 0 };
	UBYTE                buf[56];
	BYTE                 error;

	IUtility->ClearMem(buf, sizeof(buf));
	IUtility->ClearMem(cdi, sizeof(*cdi));

	error = DoSCSICmd(libBase, io_ptr, cmd, sizeof(cmd),
		buf, sizeof(buf), SCSIF_READ|SCSIF_AUTOSENSE);

	if (error == IOERR_SUCCESS) {
		cdi->cdi_DeviceType      = buf[0] & 0x1f;
		cdi->cdi_RemovableMedium = buf[1] >> 7;
		cdi->cdi_ANSIVersion     = buf[2] & 0x07;

		IExec->CopyMem(&buf[8], cdi->cdi_VendorID, 8);
		IExec->CopyMem(&buf[16], cdi->cdi_ProductID, 16);
		IExec->CopyMem(&buf[32], cdi->cdi_ProductRev, 4);
		IExec->CopyMem(&buf[36], cdi->cdi_VendorSpec, 20);
	}

	return error;
}
