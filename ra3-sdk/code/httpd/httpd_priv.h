/*
** Copyright (c) 2002  Hughes Technologies Pty Ltd.  All rights
** reserved.
**
** Terms under which this software may be used or copied are
** provided in the  specific license associated with this product.
**
** Hughes Technologies disclaims all warranties with regard to this
** software, including all implied warranties of merchantability and
** fitness, in no event shall Hughes Technologies be liable for any
** special, indirect or consequential damages or any damages whatsoever
** resulting from loss of use, data or profits, whether in an action of
** contract, negligence or other tortious action, arising out of or in
** connection with the use or performance of this software.
**
**
** $Id: httpd_priv.h,v 1.5 2002/11/25 02:15:51 bambi Exp $
**
*/

/*
**  libhttpd Private Header File
*/


/***********************************************************************
** Standard header preamble.  Ensure singular inclusion, setup for
** function prototypes and c++ inclusion
*/

#ifndef LIB_HTTPD_PRIV_H

#define LIB_HTTPD_H_PRIV 1

#if !defined(__ANSI_PROTO)
#if defined(_WIN32) || defined(__STDC__) || defined(__cplusplus)
#  define __ANSI_PROTO(x)       x
#else
#  define __ANSI_PROTO(x)       ()
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define	LEVEL_NOTICE	"notice"
#define LEVEL_ERROR	"error"

char * httpd_unescape __ANSI_PROTO((char*));
char *httpd_escape_string __ANSI_PROTO((char*));
char httpd_from_hex  __ANSI_PROTO((char));
int httpd_decode __ANSI_PROTO((char*, char*, int));


void httpd_catFile __ANSI_PROTO((httpd*, char*));
void httpd_send403 __ANSI_PROTO((httpd*));
void httpd_send304 __ANSI_PROTO((httpd*));
void httpd_send404 __ANSI_PROTO((httpd*));
void httpd_sendText __ANSI_PROTO((httpd*, char*));
void httpd_sendFile __ANSI_PROTO((httpd*, char*));
void httpd_sendStatic __ANSI_PROTO((httpd*, char*));
void httpd_sendHeaders __ANSI_PROTO((httpd*, int,int);)
void httpd_cleanUrl __ANSI_PROTO((char*));
void httpd_freeVariables __ANSI_PROTO((httpVar*));
void httpd_formatTimeString __ANSI_PROTO((httpd*, char*, int));
void httpd_storeData __ANSI_PROTO((httpd*, char*));
void httpd_writeAccessLog __ANSI_PROTO((httpd*));
void httpd_writeErrorLog __ANSI_PROTO((httpd*, char*, char*));


int httpd_net_read __ANSI_PROTO((int, char*, int));
int httpd_net_write __ANSI_PROTO((int, char*, int));
int httpd_readBuf __ANSI_PROTO((httpd*, char*, int));
int httpd_readChar __ANSI_PROTO((httpd*, char*));
int httpd_readLine __ANSI_PROTO((httpd*, char*, int));
int httpd_checkLastModified __ANSI_PROTO((httpd*, int));
int httpd_sendDirectoryEntry __ANSI_PROTO((httpd*, httpContent*, char*));

httpContent *httpd_findContentEntry __ANSI_PROTO((httpd*, httpDir*, char*));
httpDir *httpd_findContentDir __ANSI_PROTO((httpd*, char*, int));

#endif  /* LIB_HTTPD_PRIV_H */
