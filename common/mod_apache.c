/* 
**  mod_apache.c -- Apache 2 module
**  Insert module name below where indicated by **NAME**.
**  (c) 2004-8, Questrel, Inc.
*/

#include <signal.h>	  /* for kill() */
#include <stdio.h>	  /* for fprintf() */
#include <string.h>	  /* for memcpy() */
#include <sys/types.h>    /* for msgget(), etc. */
#include <sys/msg.h>	  /* for msgget(), etc. */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"

#include "apr.h"
#include "apr_buckets.h"

#include <stdlib.h>	  /* for malloc(), realloc(), free() */

/* Start the child process. */
#ifdef WIN32

#include <windows.h>
#include <direct.h>	  // for _getcwd()

/*
 * getcwd - translation from POSIX to ISO C++ system call
 */

#define getcwd(a, b) _getcwd(a, b)

/*
 * sleep - convert from Unix seconds to Windows milliseconds
 * Does not support interrupts.
 */

static unsigned sleep(unsigned x) {
  Sleep(1000 * x);
  return 0;
}

static int spawn_server(void) {

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );

  if (CreateProcess(
    TEXT("cgi-bin\\sched.exe"),				  /**NAME**/  
    TEXT("sched -c"),		  			  /* Command line. Pass co-process switch. */ 
    NULL,						  /* Process handle not inheritable. */
    NULL,						  /* Thread handle not inheritable. */
    FALSE,						  /* Set handle inheritance to FALSE. */
    0,							  /* No creation flags. */
    NULL,						  /* Use parent's environment block. */
    "C:\\Program Files\\Apache Software Foundation\\Apache2.2\\cgi-bin",  /* Working directory is Apache cgi-bin directory. */ 
    &si,						  /* Pointer to STARTUPINFO structure. */
    &pi )						  /* Pointer to PROCESS_INFORMATION structure. */
    )
      return TRUE;
  else {

    TCHAR szBuf[80]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );

    wsprintf(szBuf, 
      "%s failed with error %d: %s", 
      TEXT("CreateProcess"), dw, lpMsgBuf); 

    MessageBox(NULL, szBuf, "Error", MB_OK); 

    LocalFree(lpMsgBuf);

    return FALSE;

  }
}

#else /* !WIN32 */

static int spawn_server(void) {

  int pid;
  if (!(pid = fork())) {
    chdir("/home/mlb/www/cgi-bin"); /**NAME**/
    execl("sched", "sched", "-c", 0); /**NAME**/ /* pass co-process switch */
  }
  return pid != -1;
}

#endif /* !WIN32 */

struct msg_t {
  long	  mtype;	/* message type */
  char	  mtext[1];	/* message body */
};

/* send query to child and send response to user */
static void process_query(request_rec *r, char *query) {
  char buf[256];
  struct msqid_ds d;
  int qin = msgget(0xCAD, 0);
  int qout = msgget(0xCAC, 0);
  if (qin == -1) {
    if (!spawn_server())
      ap_rprintf(r, "Unable to start server from directory: %s", getcwd(buf, sizeof(buf)));
    else {
      sleep(2); /* sleep(1) might sleep less than one second */
      qin = msgget(0xCAD, 0);
      qout = msgget(0xCAC, 0);
    }
  }
  if (qin == -1 || qout == -1)
    ap_rprintf(r, "Unable to contact server");
  else if (msgctl(qout, IPC_STAT, &d)) /* get state of queue */
    ap_rprintf(r, "Unable to find state of message queue.  Error: %s", strerror(errno));
  else if (kill(d.msg_lrpid, 0) && !spawn_server()) /* check that server is alive or start it */
    ap_rprintf(r, "Unable to start server from directory: %s", getcwd(buf, sizeof(buf)));
  else {
    apr_size_t l = strlen(query) + 1; /* send trailing NUL */
    size_t msgsz = sizeof(struct msg_t) + l;
    if (msgsz > d.msg_qbytes)
      ap_rprintf(r, "Query length of %d is too long.", l);
    else {
      struct msg_t *msg = (struct msg_t *)apr_palloc(r->pool, d.msg_qbytes + sizeof(long) + 1); // room for NUL
      msg->mtype = 1;
      memcpy(msg->mtext, query, l); 
      if (msgsnd(qout, (void *)msg, msgsz, IPC_NOWAIT) == -1)
	ap_rprintf(r, "Unable to send message. Error is: %s", strerror(errno));
      else {
	if ((l = msgrcv(qin, (void *)msg, d.msg_qbytes, 0, 0)) == -1)
	  ap_rprintf(r, "Unable to receive message. Error is: %s", strerror(errno));
	else {
	  apr_size_t html_len = msg->mtype;
	  if (msg->mtype == 0) {
	    ap_rprintf(r, "Response length %d", msg->mtype);
	    return;
	  }
	  if (html_len != 0) {
	    char *html = (char *)apr_palloc(r->pool, html_len);
	    if (html == 0)
	      ap_rprintf(r, "Unable to allocate HTML buffer. Error is: %s", strerror(errno));
	    else {
	      char *htmlp = html;
	      apr_size_t remaining_length = html_len;
	      while (remaining_length) {
		if (l > remaining_length) {
		  ap_rprintf(r, "Message is too long. Error is: %s", strerror(errno));
		  break;
		}
		memcpy((void *)htmlp, msg->mtext, l);
		htmlp += l;
		remaining_length -= l;
		if (remaining_length) {
		  msg->mtype = 2;
		  if (msgsnd(qout, (void *)msg, 1, IPC_NOWAIT) == -1) {
		    ap_rprintf(r, "Unable to send message. Error is: %s", strerror(errno));
		    break;
		  }
		  if ((l = msgrcv(qin, (void *)msg, d.msg_qbytes, 0, 0)) == -1) {
		    ap_rprintf(r, "Unable to receive message. Error is: %s", strerror(errno));
		    break;
		  }
		  if (msg->mtype == 0) {
		    ap_rprintf(r, "Response length %d", msg->mtype);
		    return;
		  }
		}
	      }
	      if (ap_rwrite(html, html_len, r) != html_len)
		ap_rprintf(r, "Unable to write HTML. Error is: %s", strerror(errno));
	    }
	  }
	}
      }
    }
  }
}

/*
 * routine for reading in POST arguments -- copied from mod_cgi.c
 * returns a NUL terminated string of the arguments
 */
static char *get_post_args(request_rec *r, size_t *retval_len)
{
  apr_bucket_brigade *bb;
  int seen_eos;
  apr_status_t rv;
  char *retval;
  char *temp;

  /* Transfer any put/post args, CERN style... */
  bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
  seen_eos = 0;
  *retval_len = 1; /* make room for terminating NUL */
  retval = malloc(*retval_len);
  if (!retval)
    return 0;
  *retval = 0; /* insert the NUL */
  do {
    apr_bucket *bucket;

    rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
      APR_BLOCK_READ, HUGE_STRING_LEN);

    if (rv != APR_SUCCESS) {
      free(retval);
      return 0;
    }

    for (bucket = APR_BRIGADE_FIRST(bb);
      bucket != APR_BRIGADE_SENTINEL(bb);
      bucket = APR_BUCKET_NEXT(bucket))
    {
      const char *data;
      apr_size_t len;

      if (APR_BUCKET_IS_EOS(bucket)) {
	seen_eos = 1;
	break;
      }

      /* We can't do much with this. */
      if (APR_BUCKET_IS_FLUSH(bucket)) {
	continue;
      }

      /* read */
      apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);

      temp = realloc(retval, *retval_len + len);

      if (temp)
	retval = temp;
      else { /* arguments are too large */
	free(retval);
	return 0;
      }

      memcpy(retval + *retval_len - 1, data, len);  /* don't forget NUL */

      *retval_len += len;

      retval[*retval_len - 1] = 0;  /* reinsert NUL */

    }
    apr_brigade_cleanup(bb);
  }
  while (!seen_eos);

  return retval;
}

/* Handle a request */
static int handler(request_rec * r)
{
  char *args;
  char *pargs;
  size_t pargs_len;

  if (strcmp(r->uri, "/sched")) { /**NAME**/
    return DECLINED;
  }
  
  r->content_type = "text/html";

  if (!r->header_only)
    switch (r->method_number) {
    case M_GET:
      process_query(r, r->args);
      return OK;
    case M_POST:
      args = get_post_args(r, &pargs_len);
      if (!args)
        pargs = 0;
      else {
	pargs = apr_palloc(r->pool, pargs_len);
	if (pargs)
	  memcpy(pargs, args, pargs_len);
	free(args);
      }
      process_query(r, pargs);
      return OK;
    default:
      r->allowed |= (AP_METHOD_BIT << M_GET);
      r->allowed |= (AP_METHOD_BIT << M_POST);
      return DECLINED;
    }

  return OK;
}

static void register_hooks(apr_pool_t * p)
{
  ap_hook_handler(handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA sched_module = { /**NAME**/
    STANDARD20_MODULE_STUFF,
    NULL,			/* create per-directory config structure */
    NULL,			/* merge per-directory config structures */
    NULL,			/* create per-server config structure */
    NULL,			/* merge per-server config structures */
    NULL,			/* command apr_table_t */
    register_hooks		/* register hooks */
};
