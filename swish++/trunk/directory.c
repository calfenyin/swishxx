/*
**	SWISH++
**	directory.c
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

// standard
#include <iostream>
#include <queue>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// local
#include "directory.h"
#include "RecurseSubdirs.h"
#include "util.h"
#include "Verbosity.h"

#ifndef	PJL_NO_NAMESPACES
using namespace std;
#endif

extern RecurseSubdirs	recurse_subdirectories;
extern Verbosity	verbosity;

extern void	do_file( char const *file_name );
		// Note that there are two do_file() functions: one each in
		// extract.c and index.c.  The do_directory() function below
		// calls whichever one it's linked into an executable with,
		// either "extract" or "index" respectively.

#ifndef	PJL_NO_SYMBOLIC_LINKS
FollowLinks	follow_symbolic_links;
#endif

//*****************************************************************************
//
// SYNOPSIS
//
	void do_directory( char const *dir_name )
//
// DESCRIPTION
//
//	Call do_file() for every file in the given directory; it will queue
//	subdirectories encountered that do no start with '.' and call
//	do_directory() on them.  It will not follow symbolic links unless the
//	-l command-line option was given on the command line.
//
//	This function uses a queue and recurses only once so as not to have
//	too many directories open concurrently.  This has the effect of
//	indexing in a breadth-first order rather than depth-first.
//
// PARAMETERS
//
//	dir_name	The full path of the directory of the files and
//			subdirectories to index.
//
//*****************************************************************************
{
	typedef queue< string > dir_queue_type;
	static dir_queue_type dir_queue;
	static int recursion;

#ifndef	PJL_NO_SYMBOLIC_LINKS
	if ( is_symbolic_link( dir_name ) && !follow_symbolic_links )
		return;
#endif

	DIR *const dir_p = ::opendir( dir_name );
	if ( !dir_p )					// can't open: skip
		return;

	if ( verbosity > 1 ) {
		if ( verbosity > 2 ) cout << '\n';
		cout << dir_name;
		if ( verbosity > 2 ) cout << ':';
		cout << '\n';
	}

	string const dir_string( dir_name );

	struct dirent const *dir_ent;
	while ( dir_ent = ::readdir( dir_p ) ) {
		if ( *dir_ent->d_name == '.' )		// skip dot files
			continue;
		string const path( ( dir_string + '/' ) + dir_ent->d_name );
		if ( is_directory( path ) && recurse_subdirectories )
			dir_queue.push( path );
		else
			do_file( path.c_str() );
	}

	::closedir( dir_p );
	if ( recursion )
		return;

	////////// Do all subdirectories //////////////////////////////////////

	while ( !dir_queue.empty() ) {
		dir_queue_type::value_type dir_name = dir_queue.front();
		dir_queue.pop();
		++recursion;
		do_directory( dir_name.c_str() );
		--recursion;
	}
}
