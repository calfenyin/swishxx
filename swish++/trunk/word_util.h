/*
**	SWISH++
**	word_util.h
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

#ifndef	word_util_H
#define	word_util_H

// standard
#include <cctype>
#include <cstring>

// local
#include "config.h"
#include "fake_ansi.h"			/* for std */

extern char const iso8859_map[ 256 ];

//*****************************************************************************
//
// SYNOPSIS
//
	inline char iso8859_to_ascii( char c )
//
// DESCRIPTION
//
//	Convert an 8-bit ISO 8859-1 (Latin 1) character to its closest 7-bit
//	ASCII equivalent.  (This mostly means that accents are stripped.)
//
//	This function exists to ensure that the value of the character used
//	to index the iso8859_map[] vector declared above is unsigned.
//
// PARAMETERS
//
//	c	The character to be converted.
//
// RETURN VALUE
//
//	Returns said character.
//
// SEE ALSO
//
//	International Standards Organization.  "ISO 8859-1: Information
//	Processing -- 8-bit single-byte coded graphic character sets -- Part 1:
//	Latin alphabet No. 1," 1987.
//
//*****************************************************************************
{
	return iso8859_map[ static_cast<unsigned char>( c ) ];
}

//*****************************************************************************
//
// SYNOPSIS
//
	inline bool is_vowel( char c )
//
// DESCRIPTION
//
//	Determine whether a character is a lower-case vowel [aeiou].
//
// PARAMETERS
//
//	c	The character to be checked.
//
// RETURN VALUE
//
//	Returns true only if the character is a vowel.
//
//*****************************************************************************
{
	return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

//*****************************************************************************
//
// SYNOPSIS
//
	inline bool is_word_char( char c )
//
// DESCRIPTION
//
//	Check whether a given character is a "word character," one that is
//	valid to be in a word.
//
// PARAMETERS
//
//	c	The character to be checked.
//
// RETURN VALUE
//
//	Returns true only if the character is a "word character."
//
//*****************************************************************************
{
	return c > 0 &&
#if OPTIMIZE_WORD_CHARS
	( isalnum( c ) ||
		//
		// If you change Word_Chars in config.h from the default set
		// but would like to keep the optimization, edit the line below
		// to compare 'c' against every non-alphanumeric character in
		// your set of Word_Chars.
		//
		c == '&' || c == '\'' || c == '-' || c == '_'
	);
#else
	std::strchr( Word_Chars, tolower( c ) ) != 0;
#endif
}

//*****************************************************************************
//
// SYNOPSIS
//
	inline bool is_word_begin_char( char c )
//
// DESCRIPTION
//
//	Check whether a given character is a "word beginning character," one
//	that is valid to begin a word.
//
// PARAMETERS
//
//	c	The character to be checked.
//
// RETURN VALUE
//
//	Returns true only if the character is a "word beginning character."
//
//*****************************************************************************
{
#if OPTIMIZE_WORD_BEGIN_CHARS
	//
	// If you change Word_Begin_Chars in config.h from the default set but
	// would like to keep the optimization, edit the line below to compare
	// 'c' against every character in your set of Word_Begin_Chars.
	//
	return isalnum( c );
#else
	return std::strchr( Word_Begin_Chars, tolower( c ) ) != 0;
#endif
}

//*****************************************************************************
//
// SYNOPSIS
//
	inline bool is_word_end_char( char c )
//
// DESCRIPTION
//
//	Check whether a given character is a "word ending character," one that
//	is valid to end a word.
//
// RETURN VALUE
//
//	Returns true only if the character is a "word ending character."
//
//*****************************************************************************
{
#if OPTIMIZE_WORD_END_CHARS
	//
	// Same deal as with OPTIMIZE_WORD_BEGIN_CHARS.
	//
	return isalnum( c );
#else
	return std::strchr( Word_End_Chars, tolower( c ) ) != 0;
#endif
}

//*****************************************************************************
//
//	Miscelleneous.
//
//*****************************************************************************

extern bool	is_ok_word( char const *word );

#endif	/* word_util_H */
