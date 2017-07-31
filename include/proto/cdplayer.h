#ifndef PROTO_CDPLAYER_H
#define PROTO_CDPLAYER_H

/*
**	$Id$
**	Includes Release 50.1
**
**	Prototype/inline/pragma header file combo
**
**	Copyright (c) 2010 Hyperion Entertainment CVBA.
**	All Rights Reserved.
*/

#ifndef LIBRARIES_CDPLAYER_H
#include <libraries/cdplayer.h>
#endif

/****************************************************************************/

#ifndef __NOLIBBASE__
 #ifndef __USE_BASETYPE__
  extern struct Library * CDPlayerBase;
 #else
  extern struct Library * CDPlayerBase;
 #endif /* __USE_BASETYPE__ */
#endif /* __NOLIBBASE__ */

/****************************************************************************/

#ifdef __amigaos4__
 #include <interfaces/cdplayer.h>
 #ifdef __USE_INLINE__
  #include <inline4/cdplayer.h>
 #endif /* __USE_INLINE__ */
 #ifndef CLIB_CDPLAYER_PROTOS_H
  #define CLIB_CDPLAYER_PROTOS_H 1
 #endif /* CLIB_CDPLAYER_PROTOS_H */
 #ifndef __NOGLOBALIFACE__
  extern struct CDPlayerIFace *ICDPlayer;
 #endif /* __NOGLOBALIFACE__ */
#else /* __amigaos4__ */
 #ifndef CLIB_CDPLAYER_PROTOS_H
  #include <clib/cdplayer_protos.h>
 #endif /* CLIB_CDPLAYER_PROTOS_H */
 #if defined(__GNUC__)
  #ifndef __PPC__
   #include <inline/cdplayer.h>
  #else
   #include <ppcinline/cdplayer.h>
  #endif /* __PPC__ */
 #elif defined(__VBCC__)
  #ifndef __PPC__
   #include <inline/cdplayer_protos.h>
  #endif /* __PPC__ */
 #else
  #include <pragmas/cdplayer_pragmas.h>
 #endif /* __GNUC__ */
#endif /* __amigaos4__ */

/****************************************************************************/

#endif /* PROTO_CDPLAYER_H */
