/*
**	SWISH++
**	do_file.c
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

/*
**	Note that this file is #include'd into index.c and extract.c.
*/

// standard
#include <vector>

//*****************************************************************************
//
// SYNOPSIS
//
	void do_file( char const *file_name )
//
// DESCRIPTION
//
//	Either index or extract text from the given file, but only if its
//	extension is among (not among) the specified set.  It will not follow
//	symbolic links unless the -l command-line option was given.
//
//	For indexing, a file is considered to be an HTML file only if its
//	extension is "htm", "html", or "shtml".
//
//	For extraction, the algorithm is loosely based on what the Unix
//	strings(1) command does except it goes a bit further to discard
//	things like Encapsulated PostScript and raw hex data.
//
// PARAMETERS
//
//	file_name	The file to process.
//
//*****************************************************************************
{
	char const *const slash = ::strrchr( file_name, '/' );
	char const *const base_name = slash ? slash + 1 : file_name;

	++num_examined_files;
	if ( verbosity > 3 )			// print base name of file
		cout << "  " << base_name << flush;

	////////// Simple checks to see if we should process the file /////////

	if ( !is_plain_file() ) {
		if ( verbosity > 3 )
			cout << " (skipped: not plain file)" << endl;
		return;
	}
#ifndef	PJL_NO_SYMBOLIC_LINKS
	if ( is_symbolic_link( file_name ) && !follow_symbolic_links ) {
		if ( verbosity > 3 )
			cout << " (skipped: symbolic link)" << endl;
		return;
	}
#endif

	////////// Perform filter name substitution(s) ////////////////////////

	typedef vector< filter > filter_list_type;
	filter_list_type filter_list;
	filter_list.reserve( 5 );

	char const *ext;
	while ( 1 ) {
		//
		// Check to see if the file name has an extension by looking
		// for a '.' in it and making sure that it is not the last
		// character.  If the extension contains a '/', then it's
		// really not an extension; rather, it's a file name like:
		// "/a.bizarre/file".
		//
		ext = ::strrchr( file_name, '.' );
		if ( !ext || !*++ext || ::strchr( ext, '/' ) ) {
			if ( verbosity > 3 )
				cout << " (skipped: no extension)" << endl;
			return;
		}

		//
		// Determine if the file needs to be filtered and, if so, set
		// the filename to what it would become if it were filtered.
		//
		FilterExtension::const_pointer const f = filters[ ext ];
		if ( !f )
			break;
		filter_list.push_back( *f );
		file_name = filter_list.back().substitute( file_name );
	}

	//
	// Skip the file if either the set of unacceptable extensions contains
	// the candidate or the set of acceptable extensions doesn't.
	//
	if ( exclude_extensions.find( ext ) ) {
		if ( verbosity > 3 )
			cout << " (skipped: extension excluded)" << endl;
		return;
	}
	if ( !include_extensions.empty() && !include_extensions.find( ext ) ) {
		if ( verbosity > 3 )
			cout << " (skipped: extension not included)" << endl;
		return;
	}

#ifdef	EXTRACT
	//
	// Check to see if the .txt file already exists; if so, skip it.
	//
	char file_name_txt[ MAXNAMLEN ];
	::strcpy( file_name_txt, file_name );
	::strcat( file_name_txt, ".txt" );
	if ( file_exists( file_name_txt ) ) {
		if ( verbosity > 3 )
			cout << " (skipped: .txt file already exists)" << endl;
		return;
	}
	ofstream txt( file_name_txt );
	if ( !txt ) {
		if ( verbosity > 3 )
			cout << " (skipped: can not create .txt file)" << endl;
		return;
	}
#endif

	//
	// Execute the filter(s) on the file.
	//
	FOR_EACH( filter_list_type, filter_list, f )
		if ( !( file_name = f->exec() ) ) {
			if ( verbosity > 3 )
				cout << " (skipped: could not filter)" << endl;
			return;
		}

	file_vector<char> file( file_name );
	if ( !file ) {
		if ( verbosity > 3 )
			cout << " (skipped: can not open)" << endl;
		return;
	}

	if ( verbosity == 3 )			// print base name of file
		cout << "  " << base_name << flush;

#ifdef	INDEX
	if ( file.empty() ) {
		//
		// Don't waste a file_info entry on it.
		//
		if ( verbosity > 2 )
			cout << " (0 words)" << endl;
		return;
	}

	bool const is_html = is_html_ext( ext );
	char const *title = is_html ? grep_title( file ) : 0;
	if ( !title ) {
		//
		// File either isn't HTML or it doesn't have a <TITLE> tag:
		// simply use its file name as its title.
		//
		if ( title = ::strrchr( file_name, '/' ) )
			++title;
		else
			title = file_name;
	}

	////////// Index the file /////////////////////////////////////////////

	new file_info( file_name, file.size(), title );
	index_words( file.begin(), file.end(), is_html );

	if ( verbosity > 2 )
		cout	<< " (" << file_info::current_file().num_words_
			<< " words)" << endl;

	if ( words.size() >= Word_Threshold )
		write_partial_index();
#endif
#ifdef	EXTRACT
	////////// Extract the file ///////////////////////////////////////////

	++num_extracted_files;
	extract_words( file.begin(), file.end(), txt );
#endif
}