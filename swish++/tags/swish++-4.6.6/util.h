/*
**	SWISH++
**	util.h
**
**	Copyright (C) 1998  Paul J. Lucas
**
**	This program is free software; you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation; either version 2 of the License, or
**	(at your option) any later version.
** 
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
** 
**	You should have received a copy of the GNU General Public License
**	along with this program; if not, write to the Free Software
**	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef	util_H
#define	util_H

// standard
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <string>
#ifndef	WIN32
#include <ctime>			/* needed by sys/resource.h */
#include <sys/resource.h>
#endif
#include <sys/stat.h>

//
// POSIX.1 is, IMHO, brain-damaged in the way it makes you determine the
// maximum path-name length, so we'll simply pick a sufficiently large constant
// such as 1024.  In practice, this is the actual value used on many SVR4 as
// well as 4.3+BSD systems.
//
// See also: W. Richard Stevens.  "Advanced Programming in the Unix
// Environment," Addison-Wesley, Reading, MA, 1993.  pp. 34-42.
//
#ifdef	PATH_MAX
#undef	PATH_MAX
#endif
int const		PATH_MAX = 1024;

// local
#include "config.h"
#include "file_vector.h"
#include "platform.h"				/* for PJL_NO_SYMBOLIC_LINKS */

extern char const*	me;

//*****************************************************************************
//
// SYNOPSIS
//
	template< int Buf_Size, int N > class char_buffer_pool
//
// DESCRIPTION
//
//	A char_buffer_pool maintains a small set ("pool") of size N of
//	available character buffers, each of size Buf_Size, and issues them in
//	a round-robin manner.
//
//	This is used by functions to return a character string without having
//	to allocate memory dynamically nor have previously returned strings
//	overwritten.
//
//*****************************************************************************
{
public:
	char_buffer_pool() : next_buf_index_( 0 ), cur_buf_( buf_[ 0 ] ) { }

	char*	current() const { return cur_buf_; }
	char*	next() {
			cur_buf_ = buf_[ next_buf_index_ ];
			next_buf_index_ = (next_buf_index_ + 1) % N;
			return cur_buf_;
		}
private:
	char	buf_[ N ][ Buf_Size ];
	int	next_buf_index_;
	char	*cur_buf_;
};

//*****************************************************************************
//
// SYNOPSIS
//
	template< class T > void max_out_limit( T resource )
//
// DESCRIPTION
//
//	Set the limit for the given resource to its maximum value.
//
// PARAMETERS
//
//	resource	The ID for the resource as given in sys/resources.h.
//
// NOTE
//
//	This can't be an ordinary function since the type "resource" isn't int
//	on some systems (e.g., Linux).
//
// SEE ALSO
//
//	W. Richard Stevens.  "Advanced Programming in the Unix Environment,"
//	Addison-Wesley, Reading, MA, 1993. pp. 180-184.
//
//*****************************************************************************
{
	struct rlimit r;
	::getrlimit( resource, &r );
	r.rlim_cur = r.rlim_max;
	::setrlimit( resource, &r );
}

//*****************************************************************************
//
//	File test functions.  Those that do not take an argument operate on the
//	last file stat'ed.
//
//*****************************************************************************

extern struct stat	stat_buf;		// somplace to do a stat(2) in

inline bool	file_exists( char const *path ) {
			return std::stat( path, &stat_buf ) != -1;
		}
inline bool	file_exists( std::string const &path ) {
			return file_exists( path.c_str() );
		}

inline off_t	file_size() { return stat_buf.st_size; }

inline bool	is_directory() {
			return S_ISDIR( stat_buf.st_mode );
		}
inline bool	is_directory( char const *path ) {
			return file_exists( path ) && is_directory();
		}
inline bool	is_directory( std::string const &path ) {
			return is_directory( path.c_str() );
		}

inline bool	is_plain_file() {
			return S_ISREG( stat_buf.st_mode );
		}
inline bool	is_plain_file( char const *path ) {
			return file_exists( path ) && is_plain_file();
		}
inline bool	is_plain_file( std::string const &path ) {
			return is_plain_file( path.c_str() );
		}

#ifndef	PJL_NO_SYMBOLIC_LINKS
inline bool	is_symbolic_link() {
			return S_ISLNK( stat_buf.st_mode & S_IFLNK );
		}
inline bool	is_symbolic_link( char const *path ) {
			return	std::lstat( path, &stat_buf ) != -1
				&& is_symbolic_link();
		}
inline bool	is_symbolic_link( std::string const &path ) {
			return is_symbolic_link( path.c_str() );
		}
#endif	/* PJL_NO_SYMBOLIC_LINKS */

//*****************************************************************************
//
//	Miscelleneous.
//
//*****************************************************************************

inline std::ostream&	error( std::ostream &o = cerr ) {
				return o << me << ": error: ";
			}
inline std::ostream&	error_string( std::ostream &o = cerr ) {
				return o << ": " << std::strerror( errno )
					<< endl;
			}

inline char*		new_strdup( char const *s ) {
				return std::strcpy(
					new char[ std::strlen( s ) + 1 ], s
				);
			}

			// ensure function semantics: 'c' is expanded once
inline char		to_lower( char c )	{ return tolower( c ); }
extern char*		to_lower( char const* );
#ifdef	SEARCH_DAEMON
extern char*		to_lower_r( char const* );
#endif
extern char*		to_lower( char const *begin, char const *end );

#define	FOR_EACH(T,C,I) \
	for ( T::const_iterator I = (C).begin(); I != (C).end(); ++I )

#define	TRANSFORM_EACH(T,C,I) \
	for ( T::iterator I = (C).begin(); I != (C).end(); ++I )

#endif	/* util_H */