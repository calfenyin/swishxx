/*
**	SWISH++
**	token.h
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

#ifndef	token_H
#define	token_H

// local
#include "config.h"
#include "fake_ansi.h"			/* for explicit, std */
#include "platform.h"

// standard
#ifdef	PJL_GCC_2xx
#include <strstream>
#else
#include <sstream>
#endif

class token_stream;

//*****************************************************************************
//
// SYNOPSIS
//
	class token
//
// DESCRIPTION
//
//	This class is used to contain a token parsed from a token_stream.
//
//*****************************************************************************
{
public:
	enum type {
		no_token,
		and_token,
		equal_token,
		lparen_token,
		not_token,
		or_token,
		rparen_token,
		word_star_token,
		word_token
	};

	token() : type_( no_token )		{ }
	explicit token( token_stream &in )	{ in >> *this; }

	// default copy constructor is fine
	// default assignment operator is fine

	operator	type() const		{ return type_; }
	int		length() const		{ return len_; }
	char const*	str() const		{ return buf_; }
	char const*	lower_str() const	{ return lower_buf_; }

	friend token_stream& operator>>( token_stream&, token& );
private:
	type		type_;
	char		buf_[ Word_Hard_Max_Size + 1 ];
	char		lower_buf_[ Word_Hard_Max_Size + 1 ];
	int		len_;
};

#ifdef	PJL_GCC_2xx
#define	TOKEN_STREAM_BASE std::istrstream
#else
#define	TOKEN_STREAM_BASE std::istringstream
#endif

//*****************************************************************************
//
// SYNOPSIS
//
	class token_stream : public TOKEN_STREAM_BASE
//
// DESCRIPTION
//
//	A token_stream is-an istrstream that has the additional ability to put
//	back previously parsed tokens used as look-ahead.  Note that it puts
//	back whole, already-parsed tokens rather than characters so they don't
//	have to be parsed again.
//
//*****************************************************************************
{
public:
	token_stream( char const *s ) : TOKEN_STREAM_BASE( s ), top_( -1 ) { }
	void	put_back( token const &t ) { stack_[ ++top_ ] = t; }
	friend token_stream& operator>>( token_stream&, token& );
private:
	// Our query parser needs at most 2 look-ahead tokens.
	token	stack_[ 2 ];
	int	top_;

	token*	put_back() { return top_ >= 0 ? stack_ + top_-- : 0; }
};

#undef	TOKEN_STREAM_BASE

#endif	/* token_H */
