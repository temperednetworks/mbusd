/*
 * OpenMODBUS/TCP to RS-232/485 MODBUS RTU gateway
 *
 * tty.c - terminal I/O related procedures
 *
 * Copyright (c) 2002-2003, Victor Antonovich (avmlink@vlink.ru)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: tty.c,v 1.1 2003/09/13 20:38:28 kapyar Exp $
 */

#include "tty.h"

extern cfg_t cfg;

static int tty_break;

/*
 * Flag signal SIG
 */
void 
tty_sighup(void)
{
  tty_break = TRUE;
  return;
}

/*
 * Init serial link parameters MOD to PORT name, SPEED and TRXCNTL type
 */
void 
#ifdef  TRXCTL
tty_init(ttydata_t *mod, char *port, int speed, int trxcntl)
#else
tty_init(ttydata_t *mod, char *port, int speed)
#endif
{
  mod->fd = -1;
  mod->port = port;
  mod->speed = speed;
#ifdef  TRXCTL
  mod->trxcntl = trxcntl;
#endif
}

#ifdef HAVE_LIBUTIL
char *tty_get_name(char *ttyfullname);

char *
tty_get_name(char *ttyfullname)
{
  static char ttynamebuf[INTBUFSIZE + 1];
  char *ttyname = ttynamebuf, *ttynameptr = ttyname;
  strncpy(ttynamebuf, ttyfullname, INTBUFSIZE);
  for (ttynameptr = strtok(ttynamebuf, "/");
       ttynameptr;
       ttynameptr = strtok(NULL, "/"))
  {
    ttyname = ttynameptr;
  }
  return ttyname;
}
#endif


/*
 * Opening serial link whith parameters in MOD
 */
int 
tty_open(ttydata_t *mod)
{
#ifdef HAVE_LIBUTIL
  int buferr, uuerr;
  char *ttyname = tty_get_name(mod->port);
#endif
  if (mod->fd > 0)
    return RC_AOPEN;        /* if already open... */
  tty_break = FALSE;
#ifdef HAVE_LIBUTIL
  if ((uuerr = uu_lock(ttyname)) != UU_LOCK_OK)
  {
    buferr = errno;
#ifdef LOG
    log(0, "uu_lock(): can't lock tty device %s (%s)",
        ttyname, uu_lockerr(uuerr));
#endif  
    errno = buferr;
    return RC_ERR;
  }
#endif
  mod->fd = open(mod->port, O_RDWR | O_NONBLOCK);
  if (mod->fd < 0)
    return RC_ERR;          /* attempt failed */
  return tty_set_attr(mod);
}

/*
 * Setting up tty device MOD attributes
 */
int 
tty_set_attr(ttydata_t *mod)
{
  int flag;

  if (tcgetattr(mod->fd, &mod->savedtios))
    return RC_ERR;
  memcpy(&mod->tios, &mod->savedtios, sizeof(mod->tios));
  mod->tios.c_cflag &= ~(CSTOPB | PARENB | PARODD | CRTSCTS);
  mod->tios.c_cflag |= CS8 | CREAD | CLOCAL;
  mod->tios.c_iflag = FALSE;
  mod->tios.c_oflag = FALSE;
  mod->tios.c_lflag = FALSE;
  mod->tios.c_cc[VTIME] = 0;
  mod->tios.c_cc[VMIN] = 1;
  cfsetspeed(&mod->tios, tty_transpeed(mod->speed));
  if (tcsetattr(mod->fd, TCSANOW, &mod->tios))
    return RC_ERR;
#if defined(TIOCSETA)
  ioctl(mod->fd, TIOCSETA, &mod->tios);
#else
  /* if TIOCSETA is not defined, try to fallback to TCSETA */
  ioctl(mod->fd, TCSETA, &mod->tios);
#endif
  tcflush(mod->fd, TCIOFLUSH);
#ifdef  TRXCTL
  tty_clr_rts(mod->fd);
#endif
  flag = fcntl(mod->fd, F_GETFL, 0);
  if (flag < 0)
    return RC_ERR;
  return fcntl(mod->fd, F_SETFL, flag | O_NONBLOCK);
}

/*
 * Translate integer SPEED value to speed_t constant
 */
speed_t 
tty_transpeed(int speed)
{
  speed_t tspeed;
  switch (speed)
  {
  case 0:
    tspeed = B0;
    break;
#if defined(B50)
  case 50:
    tspeed = B50;
    break;
#endif
#if defined(B75)
  case 75:
    tspeed = B75;
    break;
#endif
#if defined(B110)
  case 110:
    tspeed = B110;
    break;
#endif
#if defined(B134)
  case 134:
    tspeed = B134;
    break;
#endif
#if defined(B150)
  case 150:
    tspeed = B150;
    break;
#endif
#if defined(B200)
  case 200:
    tspeed = B200;
    break;
#endif
#if defined(B300)
  case 300:
    tspeed = B300;
    break;
#endif
#if defined(B600)
  case 600:
    tspeed = B600;
    break;
#endif
#if defined(B1200)
  case 1200:
    tspeed = B1200;
    break;
#endif
#if defined(B1800)
  case 1800:
    tspeed = B1800;
    break;
#endif
#if defined(B2400)
  case 2400:
    tspeed = B2400;
    break;
#endif
#if defined(B4800)
  case 4800:
    tspeed = B4800;
    break;
#endif
#if defined(B7200)
  case 7200:
    tspeed = B7200;
    break;
#endif
#if defined(B9600)
  case 9600:
    tspeed = B9600;
    break;
#endif
#if defined(B12000)
  case 12000:
    tspeed = B12000;
    break;
#endif
#if defined(B14400)
  case 14400:
    tspeed = B14400;
    break;
#endif
#if defined(B19200)
  case 19200:
    tspeed = B19200;
    break;
#elif defined(EXTA)
  case 19200:
    tspeed = EXTA;
    break;
#endif
#if defined(B38400)
  case 38400:
    tspeed = B38400;
    break;
#elif defined(EXTB)
  case 38400:
    tspeed = EXTB;
    break;
#endif
#if defined(B57600)
  case 57600:
    tspeed = B57600;
    break;
#endif
#if defined(B115200)
  case 115200:
    tspeed = B115200;
    break;
#endif
  default:
    tspeed = DEFAULT_BSPEED;
  }
  return tspeed;
}

/*
 * Prepare tty device MOD to closing
 */
int 
tty_cooked(ttydata_t *mod)
{
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  if (!isatty(mod->fd))
    return RC_ERR;
  if (tcsetattr(mod->fd, TCSAFLUSH, &mod->savedtios))
    return RC_ERR;
  return RC_OK;
}

/*
 * Closing tty device MOD
 */
int 
tty_close(ttydata_t *mod)
{
#ifdef HAVE_LIBUTIL
  int buferr;
  char *ttyname = tty_get_name(mod->port);
#endif
  if (mod->fd < 0)
    return RC_ACLOSE;       /* already closed */
  if (tty_cooked(mod))
    return RC_ERR;
#ifdef HAVE_LIBUTIL
  if (close(mod->fd))
    return RC_ERR;
  if (uu_unlock(ttyname))
  {
    buferr = errno;
#ifdef LOG
    log(0, "uu_lock(): can't unlock tty device %s",
        ttyname);
#endif
    errno = buferr;
    return RC_ERR;
  }
  return RC_OK;
#else
  return close(mod->fd);
#endif
}

#ifdef  TRXCTL
/* Set RTS line to active state */
void 
tty_set_rts(int fd)
{
  int mstat = TIOCM_RTS;
  ioctl(fd, TIOCMBIS, &mstat);
}

/* Set RTS line to passive state */
void 
tty_clr_rts(int fd)
{
  int mstat = TIOCM_RTS;
  ioctl(fd, TIOCMBIC, &mstat);
}
#endif

/*
 * Delay for USEC microsecs
 */
void
tty_delay(int usec)
{
  struct timeval tv, ttv;
  long ts;
  gettimeofday(&tv, NULL);
  do
  {
    (void)gettimeofday(&ttv, NULL);
    ts = 1000000 * (ttv.tv_sec - tv.tv_sec) + (ttv.tv_usec - tv.tv_usec);
  } while (ts < usec && !tty_break);
}

