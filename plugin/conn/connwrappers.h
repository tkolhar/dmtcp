/****************************************************************************
 *   Copyright (C) 2006-2010 by Jason Ansel, Kapil Arya, and Gene Cooperman *
 *   jansel@csail.mit.edu, kapil@ccs.neu.edu, gene@ccs.neu.edu              *
 *                                                                          *
 *   This file is part of the dmtcp/src module of DMTCP (DMTCP:dmtcp/src).  *
 *                                                                          *
 *  DMTCP:dmtcp/src is free software: you can redistribute it and/or        *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP:dmtcp/src is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#pragma once
#ifndef CONN_WRAPPERS_H
#define CONN_WRAPPERS_H

#if __GLIBC_PREREQ(2,5)
# define READLINK_RET_TYPE ssize_t
#else
# define READLINK_RET_TYPE int
#endif

#include "dmtcpplugin.h"

#define _real_socket NEXT_FNC(socket)
#define _real_connect NEXT_FNC(connect)
#define _real_bind NEXT_FNC(bind)
#define _real_listen NEXT_FNC(listen)
#define _real_accept NEXT_FNC(accept)
#define _real_accept4 NEXT_FNC(accept4)
#define _real_setsockopt NEXT_FNC(setsockopt)
#define _real_getsockopt NEXT_FNC(getsockopt)
#define _real_socketpair NEXT_FNC(socketpair)

#define _real_open NEXT_FNC(open)
#define _real_open64 NEXT_FNC(open64)
#define _real_fopen NEXT_FNC(fopen)
#define _real_fopen64 NEXT_FNC(fopen64)
#define _real_openat NEXT_FNC(openat)
#define _real_openat64 NEXT_FNC(openat64)
#define _real_opendir NEXT_FNC(opendir)
#define _real_close NEXT_FNC(close)
#define _real_fclose NEXT_FNC(fclose)
#define _real_closedir NEXT_FNC(closedir)
#define _real_dup NEXT_FNC(dup)
#define _real_dup2 NEXT_FNC(dup2)
#define _real_dup3 NEXT_FNC(dup3)
#define _real_xstat NEXT_FNC(__xstat)
#define _real_xstat64 NEXT_FNC(__xstat64)
#define _real_lxstat NEXT_FNC(__lxstat)
#define _real_lxstat64 NEXT_FNC(__lxstat64)
#define _real_readlink NEXT_FNC(readlink)
#define _real_exit NEXT_FNC(exit)
#define _real_syscall NEXT_FNC(syscall)
#define _real_unsetenv NEXT_FNC(unsetenv)
#define _real_ptsname_r NEXT_FNC(ptsname_r)
#define _real_ttyname_r NEXT_FNC(ttyname_r)
#define _real_getpt NEXT_FNC(getpt)
#define _real_posix_openpt NEXT_FNC(posix_openpt)
#define _real_openlog NEXT_FNC(openlog)
#define _real_closelog NEXT_FNC(closelog)
#define _real_mq_open NEXT_FNC(mq_open)
#define _real_mq_close NEXT_FNC(mq_close)
#define _real_mq_timedsend NEXT_FNC(mq_timedsend)
#define _real_mq_timedreceive NEXT_FNC(mq_timedreceive)
#define _real_mq_notify NEXT_FNC(mq_notify)

#define _real_epoll_create NEXT_FNC(epoll_create)
#define _real_epoll_create1 NEXT_FNC(epoll_create1)
#define _real_epoll_ctl NEXT_FNC(epoll_ctl)
#define _real_epoll_wait NEXT_FNC(epoll_wait)
#define _real_epoll_pwait NEXT_FNC(epoll_pwait)
#define _real_eventfd NEXT_FNC(eventfd)
#define _real_signalfd NEXT_FNC(signalfd)
#define _real_inotify_init NEXT_FNC(inotify_init)
#define _real_inotify_init1 NEXT_FNC(inotify_init1)
#define _real_inotify_add_watch NEXT_FNC(inotify_add_watch)
#define _real_inotify_rm_watch NEXT_FNC(inotify_rm_watch)

#define _real_fcntl NEXT_FNC(fcntl)
#define _real_select NEXT_FNC(select)
#define _real_poll NEXT_FNC(poll)
#define _real_system NEXT_FNC(system)
#define _real_pthread_mutex_lock NEXT_FNC(pthread_mutex_lock)
#define _real_pthread_mutex_unlock NEXT_FNC(pthread_mutex_unlock)
#endif // CONN_WRAPPERS_H
