/*
**	SWISH++
**	conf_var.c
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
#include <cctype>
#include <cstdlib>			/* for abort(3) */
#include <cstring>
#include <iostream>

// local
#include "config.h"
#include "conf_var.h"
#include "exit_codes.h"
#include "file_vector.h"
#include "util.h"

#ifndef	PJL_NO_NAMESPACES
using namespace std;
#endif

extern char const	*me;

int conf_var::current_config_file_line_no_ = 0;

//*****************************************************************************
//
// SYNOPSIS
//
	/* virtual */ conf_var::~conf_var()
//
// DESCRIPTION
//
//	Destructs a conf_var.  It is out-of-line only because it's virtual
//	(so its address is taken and put into the vtbl).
//
//*****************************************************************************
{
	// do nothing
}

//*****************************************************************************
//
// SYNOPSIS
//
	void conf_var::alias_name( char const *var_name )
//
// DESCRIPTION
//
//	Map a configuration file variable name to an instance of a conf_var
//	(really, an instance of some derived class).  Only a single instance of
//	any given variable may exist.
//
// PARAMETERS
//
//	var_name	The name of the variable.
//
//*****************************************************************************
{
	conf_var *&var = map_ref()[ var_name ];
	if ( var ) {
		cerr	<< "conf_var::conf_var(): \"" << var_name
			<< "\" registered more than once" << endl;
		::abort();
	}
	var = this;
}

//*****************************************************************************
//
// SYNOPSIS
//
	/* static */ conf_var::map_type& conf_var::map_ref()
//
// DESCRIPTION
//
//	Define and initialize (exactly once) a static data member for conf_var
//	and return a reference to it.  The reason for this function is to
//	guarantee that the map is initialized before its first use across all
//	translation units, something that would not guaranteed if it were a
//	static data member initialized at file scope.
//
//	We also load the map with all configuration variable names so we can
//	tell the difference between a variable that doesn't exist (and that we
//	will complain about to the user) and one that simply isn't used in a
//	particular executable (and will be silently ignored).
//
// RETURN VALUE
//
//	Returns a reference to a static instance of an initialized map_type.
//
// SEE ALSO
//
//	Margaret A. Ellis and Bjarne Stroustrup.  "The Annotated C++
//	Reference Manual."  Addison-Wesley, Reading, MA, 1990.  p. 19.
//
//*****************************************************************************
{
	static map_type m;
	if ( m.empty() ) {
		m[ "ExcludeClass"	] = 0;
		m[ "ExcludeFile"	] = 0;
		m[ "ExcludeMeta"	] = 0;
		m[ "ExtractExtension"	] = 0;
		m[ "FilesGrow"		] = 0;
		m[ "FilesReserve"	] = 0;
		m[ "FilterFile"		] = 0;
		m[ "FollowLinks"	] = 0;
		m[ "HTMLFile"		] = 0;
		m[ "IncludeFile"	] = 0;
		m[ "IncludeMeta"	] = 0;
		m[ "Incremental"	] = 0;
		m[ "IndexFile"		] = 0;
		m[ "RecurseSubdirs"	] = 0;
		m[ "ResultsMax"		] = 0;
		m[ "StemWords"		] = 0;
		m[ "StopWordFile"	] = 0;
		m[ "TempDirectory"	] = 0;
		m[ "TitleLines"		] = 0;
		m[ "Verbosity"		] = 0;
		m[ "WordFilesMax"	] = 0;
		m[ "WordPercentMax"	] = 0;
#ifdef	SEARCH_DAEMON
		m[ "PidFile"		] = 0;
		m[ "SearchDaemon"	] = 0;
		m[ "SocketFile"		] = 0;
		m[ "SocketQueueSize"	] = 0;
		m[ "SocketTimeout"	] = 0;
		m[ "ThreadsMax"		] = 0;
		m[ "ThreadsMin"		] = 0;
		m[ "ThreadTimeout"	] = 0;
#endif
	}
	return m;
}

//*****************************************************************************
//
// SYNOPSIS
//
	ostream& conf_var::msg( ostream &o, char const *label )
//
// DESCRIPTION
//
//	Emit the standard message preamble to the given ostream, that is the
//	program name, a line number if a configuration file is being parsed,
//	and a label.
//
// PARAMETERS
//
//	o	The ostream to write the message to.
//
//	label	The label to emit, usually "error" or "warning".
//
// RETURN VALUE
//
//	Returns the ostream passed in.
//
//*****************************************************************************
{
	o << me;
	if ( current_config_file_line_no_ ) {
		o << ": config file line " << current_config_file_line_no_;
		current_config_file_line_no_ = 0;
	}
	return o << ": " << label << ": ";
}

//*****************************************************************************
//
// SYNOPSIS
//
	void conf_var::parse_const_value( char const *line )
//
// DESCRIPTION
//
//	Parse a line that can't be modified by simply copying it and calling
//	parse_value() on the copy.
//
//*****************************************************************************
{
	char *const line_copy = new_strdup( line );
	parse_value( name_, line_copy );
	delete[] line_copy;
}

//*****************************************************************************
//
// SYNOPSIS
//
	/* static */ void conf_var::parse_file( char const *file_name )
//
// DESCRIPTION
//
//	Parse the lines in a configuration file setting variables accordingly.
//
// PARAMETERS
//
//	file_name	The name of the configuration file to parse.
//
//*****************************************************************************
{
	file_vector conf_file( file_name );
	if ( !conf_file ) {
		if ( !::strcmp( file_name, ConfigFile_Default ) )
			return;
		cerr	<< "could not read configuration from \""
			<< file_name << '"' << endl;
		::exit( Exit_Config_File );
	}

	register int line_no = 0;
	register file_vector::const_iterator c = conf_file.begin(), nl = c;

	while ( c != conf_file.end() && nl != conf_file.end() ) {
		if ( !( nl = ::strchr( c, '\n' ) ) )
			break;
		++line_no;
		//
		// See if the line is entirely whitespace optionally followed
		// by a comment starting with '#': if so, skip it.  If we don't
		// end up skipping it, leading whitespace will have been
		// skipped.
		//
		for ( ; c != nl; ++c ) {
			if ( isspace( *c ) )
				continue;
			if ( *c == '#' )
				goto next_line;
			break;
		}
		if ( c != nl ) {
			//
			// The line has something on it worth parsing further:
			// copy it (less leading and trailing whitespace) to a
			// modifyable buffer and null-terminate it to make that
			// task easier.
			//
			char buf[ 256 ];
			ptrdiff_t len = nl - c;
			::strncpy( buf, c, len );
			while ( len > 0 )
				if ( isspace( buf[ len - 1 ] ) )
					--len;
				else
					break;
			buf[ len ] = '\0';
			parse_line( buf, line_no );
		}
next_line:
		c = nl + 1;
	}
}

//*****************************************************************************
//
// SYNOPSIS
//
	/* static */ void conf_var::parse_line( char *line, int line_no )
//
// DESCRIPTION
//
//	Parse a non-comment or non-blank line from a the configuration file,
//	the first word of which is the variable name.  Look up the variable in
//	our map and delegate the parsing of the rest of the line to an instance
//	of a derived class that knows how to parse its own line format.
//
// PARAMETERS
//
//	line		A line from a configuration file to be parsed.
//
//	line_no		The line number of the line.
//
//*****************************************************************************
{
	current_config_file_line_no_ = line_no;
	::strtok( line, " \r\t" );		// just the variable name
	map_type::const_iterator const i = map_ref().find( line );
	if ( i == map_ref().end() ) {
		cerr	<< warning << '"' << line
			<< "\" in config. file unrecognized; ignored\n";
		return;
	}
	if ( i->second ) {
		//
		// Chop off trailing newline and remove leading whitespace from
		// value.
		//
		register char *value = ::strtok( 0, "\r\n" );
		while ( *value && isspace( *value ) )
			++value;
		i->second->parse_value( line, value );
	} // else
	//
	//	This config. variable is not used by the current executable:
	//	silently ignore it.
	//
	current_config_file_line_no_ = 0;
}

//*****************************************************************************
//
// SYNOPSIS
//
	/* static */ void conf_var::reset_all()
//
// DESCRIPTION
//
//	Reset all configuration variables to their default values.
//
//*****************************************************************************
{
	map_type &m = map_ref();
	TRANSFORM_EACH( map_type, m, i )
		i->second->reset();
}
