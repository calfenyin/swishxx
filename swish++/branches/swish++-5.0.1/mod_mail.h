/*
**	SWISH++
**	mod_mail.h
**
**	Copyright (C) 2000  Paul J. Lucas
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

#ifdef	MOD_MAIL

#ifndef	mod_mail_H
#define	mod_mail_H

// standard
#include <vector>

// local
#include "auto_vec.h"
#include "indexer.h"

//*****************************************************************************
//
// SYNOPSIS
//
	class mail_indexer : public indexer
//
// DESCRIPTION
//
//	A mail_indexer is-an indexer for indexing mail (and news) files.  The
//	following message formats are handled: plain text, enriched text,
//	quoted-printable, HTML, MIME, vCard.  Header names and vCard types are
//	treated as meta names.
//
// SEE ALSO
//
//	David H. Crocker.  "RFC 822: Standard for the Format of ARPA Internet
//	Text Messages," Department of Electrical Engineering, University of
//	Delaware, August 1982.
//
//	Frank Dawson and Tim Howes.  "RFC 2426: vCard MIME Directory Profile,"
//	Network Working Group of the Internet Engineering Task Force, September
//	1998.
//
//	Ned Freed and Nathaniel S. Borenstein.  "RFC 2045: Multipurpose
//	Internet Mail Extensions (MIME) Part One: Format of Internet Message
//	Bodies," RFC 822 Extensions Working Group of the Internet Engineering
//	Task Force, November 1996.
//
//	Tim Berners-Lee.  "RFC 1563: The text/enriched MIME Content-type,"
//	Network Working Group of the Internet Engineering Task Force, January
//	1994.
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs.  "HTML 4.0
//	Specification," World Wide Web Consortium, April 1998.
//
//*****************************************************************************
{
public:
	mail_indexer() : indexer( "mail" ) { }

	virtual char const*	find_title( PJL::mmap_file const& ) const;
	virtual void		index_words(
					encoded_char_range const&,
					int meta_id = No_Meta_ID
				);
	virtual void		new_file();
private:
	// The boundary stack keeps track of all the boundary strings for MIME
	// messages since they can nest.
	//
	// Note: I can't use an actual STL stack since I need to be able to
	// clear the entire stack and, unfortunately, clear() isn't supported
	// for stacks...an oversight in STL, IMHO.
	//
	typedef std::vector< std::string > stack_type;
	static stack_type	boundary_stack_;

	enum content_type {
		Not_Indexable,		// a type we don't know how to index
		Text_Plain,
		Text_Enriched,
#ifdef	MOD_HTML
		Text_HTML,
#endif
		Text_vCard,
		Message_RFC822,
		Multipart,
	};

	typedef	std::pair< content_type, content_transfer_encoding >
		message_type;

	struct key_value {
		PJL::auto_vec<char> key;
		char const *value_begin, *value_end;
	};

	void			mail_indexer::index_enriched(
					encoded_char_range const&
				);
	message_type		mail_indexer::index_headers(
					char const *&begin, char const *end
				);
	void			mail_indexer::index_multipart(
					char const *&begin, char const *end
				);
	void			mail_indexer::index_vcard(
					char const *&begin, char const *end
				);
	static bool		parse_header(
					char const *&begin, char const *end,
					key_value*
				);

	static bool		did_last_header;
};

#endif	/* mod_mail_H */

#endif	/* MOD_MAIL */