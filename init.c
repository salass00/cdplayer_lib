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
#include "cdplayer.library_rev.h"

/* Version Tag */
static const TEXT USED verstag[] = VERSTAG;

/*
 * The system (and compiler) rely on a symbol named _start which marks
 * the beginning of execution of an ELF file. To prevent others from 
 * executing this library, and to keep the compiler/linker happy, we
 * define an empty _start symbol here.
 *
 * On the classic system (pre-AmigaOS 4.x) this was usually done by
 * moveq #0,d0
 * rts
 *
 */
int32 _start(void) {
	/* If you feel like it, open DOS and print something to the user */
	return RETURN_FAIL;
}

/* Open the library */
static struct CDPlayerBase *libOpen(struct LibraryManagerInterface *Self, ULONG version) {
	struct CDPlayerBase *libBase = (struct CDPlayerBase *)Self->Data.LibBase;

	if (version > VERSION) {
		return NULL;
	}

	/* Add any specific open code here 
	   Return 0 before incrementing OpenCnt to fail opening */

	/* Add up the open count */
	libBase->LibNode.lib_OpenCnt++;

	return libBase;
}

/* Close the library */
static BPTR libClose(struct LibraryManagerInterface *Self) {
	struct CDPlayerBase *libBase = (struct CDPlayerBase *)Self->Data.LibBase;

	/* Make sure to undo what open did */

	/* Make the close count */
	libBase->LibNode.lib_OpenCnt--;

	return ZERO;
}


/* Expunge the library */
static BPTR libExpunge(struct LibraryManagerInterface *Self) {
	struct CDPlayerBase *libBase = (struct CDPlayerBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	BPTR result = ZERO;

	if (libBase->LibNode.lib_OpenCnt == 0) {
		struct Player *player;
		result = libBase->SegList;

		/* Undo what the init code did */
		while ((player = (struct Player *)IExec->RemHead(libBase->Players)) != NULL) {
			KillPlayer(libBase, player);
		}

		IExec->FreeSysObject(ASOT_LIST, libBase->Players);

		IExec->FreeSysObject(ASOT_SEMAPHORE, libBase->PlayerSemaphore);

		IExec->DropInterface((struct Interface *)libBase->IUtility);
		IExec->CloseLibrary(libBase->UtilityBase);

		IExec->DropInterface((struct Interface *)libBase->IDOS);
		IExec->CloseLibrary(libBase->DOSBase);

		IExec->Remove((struct Node *)libBase);
		IExec->DeleteLibrary((struct Library *)libBase);
	} else {
		result = ZERO;
		libBase->LibNode.lib_Flags |= LIBF_DELEXP;
	}

	return result;
}

/* The ROMTAG Init Function */
static struct CDPlayerBase *libInit(struct CDPlayerBase *libBase, BPTR seglist, struct ExecIFace *IExec) {
	libBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->LibNode.lib_Node.ln_Pri  = 0;
	libBase->LibNode.lib_Node.ln_Name = CDPLAYERNAME;
	libBase->LibNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->LibNode.lib_Version      = VERSION;
	libBase->LibNode.lib_Revision     = REVISION;
	libBase->LibNode.lib_IdString     = VSTRING;
	libBase->IExec                    = IExec;
	
	/* Save pointer to our loaded code (the SegList) */
    libBase->SegList = seglist;

	libBase->DOSBase = IExec->OpenLibrary("dos.library", 52);
	libBase->IDOS = (struct DOSIFace *)IExec->GetInterface(libBase->DOSBase, "main", 1, NULL);

	libBase->UtilityBase = IExec->OpenLibrary("utility.library", 52);
	libBase->IUtility = (struct UtilityIFace *)IExec->GetInterface(libBase->UtilityBase, "main", 1, NULL);

	libBase->PlayerSemaphore = IExec->AllocSysObject(ASOT_SEMAPHORE, NULL);

	libBase->Players = IExec->AllocSysObjectTags(ASOT_LIST, ASOLIST_Min, TRUE, TAG_END);

    if (libBase->DOSBase != NULL &&
		libBase->IDOS != NULL &&
		libBase->UtilityBase != NULL &&
		libBase->IUtility != NULL &&
		libBase->PlayerSemaphore != NULL &&
		libBase->Players != NULL)
	{
		return libBase;
	}

	IExec->FreeSysObject(ASOT_LIST, libBase->Players);

	IExec->FreeSysObject(ASOT_SEMAPHORE, libBase->PlayerSemaphore);

	IExec->DropInterface((struct Interface *)libBase->IUtility);
	IExec->CloseLibrary(libBase->UtilityBase);

	IExec->DropInterface((struct Interface *)libBase->IDOS);
	IExec->CloseLibrary(libBase->DOSBase);

	IExec->DeleteLibrary((struct Library *)libBase);

	return NULL;
}

static uint32 _generic_Obtain(struct Interface *Self) {
	return ++Self->Data.RefCount;
}

static uint32 _generic_Release(struct Interface *Self) {
	return --Self->Data.RefCount;
}

/* Manager interface vectors */
static CONST APTR lib_manager_vectors[] = {
	(APTR)_generic_Obtain,
	(APTR)_generic_Release,
	NULL,
	NULL,
	(APTR)libOpen,
	(APTR)libClose,
	(APTR)libExpunge,
	NULL,
	(APTR)-1
};

/* "__library" interface tag list */
static CONST struct TagItem lib_managerTags[] = {
	{ MIT_Name,        (Tag)"__library"         },
	{ MIT_VectorTable, (Tag)lib_manager_vectors },
	{ MIT_Version,     1                        },
	{ TAG_DONE,        0                        }
};

/* ------------------- Library Interface(s) ------------------------ */

#include "cdplayer_vectors.c"

/* Uncomment this line (and see below) if your library has a 68k jump table */
extern APTR VecTable68K[];

static CONST struct TagItem mainTags[] = {
	{ MIT_Name,        (Tag)"main"       },
	{ MIT_VectorTable, (Tag)main_vectors },
	{ MIT_Version,     1                 },
	{ TAG_DONE,        0                 }
};

static CONST CONST_APTR libInterfaces[] = {
	lib_managerTags,
	mainTags,
	NULL
};

static CONST struct TagItem libCreateTags[] = {
	{ CLT_DataSize,   sizeof(struct CDPlayerBase) },
	{ CLT_InitFunc,   (Tag)libInit                },
	{ CLT_Interfaces, (Tag)libInterfaces          },
	/* Uncomment the following line if you have a 68k jump table */
	{ CLT_Vector68K,  (Tag)VecTable68K            },
	{ TAG_DONE,       0                           }
};

/* ------------------- ROM Tag ------------------------ */
static const struct Resident USED lib_res = {
	RTC_MATCHWORD,
	(struct Resident *)&lib_res,
	(APTR)(&lib_res + 1),
	RTF_NATIVE|RTF_AUTOINIT, /* Add RTF_COLDSTART if you want to be resident */
	VERSION,
	NT_LIBRARY, /* Make this NT_DEVICE if needed */
	0, /* PRI, usually not needed unless you're resident */
	CDPLAYERNAME,
	VSTRING,
	(APTR)libCreateTags
};
