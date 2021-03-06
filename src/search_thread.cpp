/*
**      SWISH++
**      src/search_thread.cpp
**
**      Copyright (C) 1998-2015  Paul J. Lucas
**
**      This program is free software; you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation; either version 2 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program; if not, write to the Free Software
**      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// local
#include "config.h"
#include "pjl/fdbuf.h"
#include "search.h"
#include "search_thread.h"
#include "util.h"

// standard
#include <cctype>
#include <cerrno>
#include <climits>                      /* for ARG_MAX */
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>                 /* for recv(3) */
#include <time.h>
#include <unistd.h>                     /* for close(2) */

//
// We need to know the maximum number of command-line arguments so we can split
// a command-line string into individual arguments.  If the OS defines the
// POSIX.1 ARG_MAX macro, see if it's insanely large (Solaris's limit is over a
// million!) because we don't want to allocate that much space for argument
// pointers since it would probably blow our thread stack space; however, if
// it's small, we might as well use that number since there's no reason to
// exceed it.
//
// See also: W. Richard Stevens.  "Advanced Programming in the Unix
// Environment," Addison-Wesley, Reading, MA, 1993.  pp. 32-40.
//
#define REASONABLE_ARG_MAX 50
#ifdef ARG_MAX
# if ARG_MAX > REASONABLE_ARG_MAX
#   undef ARG_MAX
# endif
#endif /* ARG_MAX */
#ifndef ARG_MAX
# define ARG_MAX REASONABLE_ARG_MAX
#endif /* ARG_MAX */

using namespace PJL;
using namespace std;

unsigned search_thread::socket_timeout;

extern void reset_socket( int fd );

// local functions
static int  split_args( char *s, char *argv[], int arg_max );
static bool timed_read_line( int fd, char *buf, int buf_size, int seconds );

///////////////////////////////////////////////////////////////////////////////

search_thread::thread* search_thread::create( thread_pool &p ) const {
  return new search_thread( p );
}

/**
 * Reads a "command-line" from the client via a socket, service a request, and
 * return the results via the same socket.
 *
 * @param arg The \c i member is the socket file descriptor.
 */
void search_thread::main( argument_type arg ) {
#define SEARCH_DAEMON_OPTIONS_ONLY
#include "search_options.cpp"           /* defines opt_spec */

# ifdef DEBUG_threads
  cerr << "in search_thread::main()\n";
# endif

  char buf[ 1024 ];
  bool ok = false;
  if ( timed_read_line( arg.i, buf, sizeof buf, socket_timeout ) ) {

#   ifdef DEBUG_threads
    cerr << "query=" << buf << "\n";
#   endif

    char*   argv_vec[ ARG_MAX ];
    char**  argv = argv_vec;
    int     argc = split_args( buf, argv, ARG_MAX );
    fdbuf   buf( arg.i );
    ostream out( &buf );

    if ( !argc ) {
      out << usage;
    } else if ( argc == ARG_MAX ) {
      out << error << "more than " << ARG_MAX << " arguments" << endl;
    } else {
      search_options const opt( &argc, &argv, opt_spec, out );
      if ( opt )
        ok = service_request( argv, opt, out, out );
    }
    out << flush;
  }

  if ( !ok ) {
    //
    // It was a bad request because it (a) timed out, (b) had too few or many
    // arguments, (c) had an error in usage, or (d) was malformed.  That being
    // the case, reset the TCP connection.
    //
    // The reason for doing this is so we don't potentially have a socket
    // lingering in TIME-WAIT from a client that was too dumb to give us a
    // valid request in the first place.  This helps alleviate denial-of-
    // service attacks (if that's what's going on).
    //
    reset_socket( arg.i );
  }

  ::close( arg.i );
}

/**
 * Splits a string into individual, argv-like arguments at whitespace.  This
 * code is based on \c buf_args() in [Stevens 1993], p. 495, except that it:
 *
 *    1. Is thread-safe by not using \c strtok().
 *    2. Discards leading whitespace in the buffer.
 *    3. Just does the split and doesn't call any function.
 *
 * See also:
 *    W. Richard Stevens.  "Advanced Programming in the Unix Environment,"
 *    Addison-Wesley, Reading, MA, 1993.  p. 495.
 *
 * @param s The string to be split.
 * @param argv The array to deposit the pointers to arguments in.
 * @param arg_max The maximum number of argument to allow.
 * @return Upon success, returns the number of arguments; upon failure, returns
 * arg_max.
 */
static int split_args( char *s, char *argv[], int arg_max ) {
  for ( ; *s && isspace( *s ); ++s )    // skip leading whitespace
    ;
  if ( !*s )
    return 0;

  int argc = 0;

  while ( (argv[ argc++ ] = s) ) {
    if ( argc >= arg_max - 1 )          // -1 to allow for null at end
      return arg_max;
    if ( (s = ::strpbrk( s, " \t\n\r" )) ) {
      *s = '\0';
      //
      // We must skip *ALL* whitespace characters separating arguments.
      //
      while ( *++s && isspace( *s ) )
        ;
    }
  } // while
  return argc;
}

/**
 * Reads a line of text (a string of characters ending in either a carriage
 * return or a newline) from a Unix file descriptor and store it in the given
 * buffer, null-terminated; but time-out if we don't get it in a certain amount
 * of time.  The carriage return or newline is discarded.
 *
 * See also:
 *    W. Richard Stevens.  "Unix Network Programming, Vol 1, 2nd ed."
 *    Prentice-Hall, Upper Saddle River, NJ, 1998.  pp. 352-353.
 *
 * @param fd The Unix file descriptor to read from.
 * @param buf The buffer to read into.
 * @param buf_size The size of \a buf.
 * @param seconds The number of seconds until a time-out.
 * @return Returns \c true only if an entire line was read in the time allotted.
 */
static bool timed_read_line( int fd, char *buf, int buf_size, int seconds ) {
  //
  // In a single-threaded application, we could simply use alarm(2) to set a
  // time-out before reading; however, in a multi-threaded application, we
  // can't since there can be at most one alarm set for an entire process:
  // individual threads can not have independent alarms.
  //
  // Therefore, what we do instead is to use select(2) to do the blocking, but
  // with a time-out specified.
  //
  time_t const start_time = ::time( nullptr );
  int seconds_remaining = seconds;
  while ( seconds_remaining > 0 ) {
    fd_set rset;
    FD_ZERO( &rset );
    FD_SET( fd, &rset );

    struct timeval tv;
    tv.tv_sec  = seconds_remaining;
    tv.tv_usec = 0;

    if ( ::select( fd + 1, &rset, nullptr, nullptr, &tv ) < 1 )
      break;
    if ( !FD_ISSET( fd, &rset ) )       // shouldn't happen, but...
      break;

    ssize_t const bytes_read = ::recv( fd, buf, buf_size, 0 );
    if ( bytes_read == -1 )             // error
      break;
    buf += bytes_read, buf_size -= bytes_read;
    if ( buf[-1] == '\r' || buf[-1] == '\n' || buf_size <= 0 ) {
      buf[-1] = '\0';
      return true;                    // got a line: woohoo!
    }
    //
    // We haven't gotten a complete line yet: see how much time has elapsed
    // and, if there's more time left before the time-out expires, try to read
    // some more.
    //
    time_t const elapsed_time = ::time( nullptr ) - start_time;
    seconds_remaining = seconds - elapsed_time;
  } // while

  return false;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
