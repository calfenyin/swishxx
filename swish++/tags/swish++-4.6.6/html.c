/*
**	SWISH++
**	html.c
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
#include <cstring>
#include <vector>
#include <utility>			/* for pair<> */

// local
#include "config.h"
#include "elements.h"
#include "entities.h"
#include "ExcludeClass.h"
#include "ExcludeMeta.h"
#include "html.h"
#include "IncludeMeta.h"
#include "index.h"
#include "meta_map.h"
#include "platform.h"
#include "TitleLines.h"
#include "util.h"
#include "word_util.h"

#ifndef	PJL_NO_NAMESPACES
using namespace std;
#endif

extern int		exclude_class_count;
extern ExcludeClass	exclude_class_names;
extern ExcludeMeta	exclude_meta_names;
extern IncludeMeta	include_meta_names;
extern TitleLines	num_title_lines;

bool			find_attribute(
				file_vector::const_iterator &begin,
				file_vector::const_iterator &end,
				char const *attribute
			);
bool			is_html_comment(
				file_vector::const_iterator &pos,
				file_vector::const_iterator end
			);
bool			skip_html_tag(
				file_vector::const_iterator &pos,
				file_vector::const_iterator end
			);
bool			tag_cmp(
				file_vector::const_iterator &pos,
				file_vector::const_iterator end,
				char const *tag
			);

//*****************************************************************************
//
// SYNOPSIS
//
	char entity_to_ascii(
		register file_vector::const_iterator &c,
		register file_vector::const_iterator end
	)
//
// DESCRIPTION
//
//	Convert either a numeric or character entity reference to its ASCII
//	character equivalent (if it has one).  A numeric reference is a
//	character sequence having the form:
//
//		&#d;
//		&#xh;
//		&#Xh;
//
//	where 'd' is a sequence of 1 or more decimal digits [0-9] and 'h' is a
//	sequence of 1 or more hexadecimal digits [0-9A-Fa-f].  A character
//	entity reference is a character sequence having the form:
//
//		&ref;
//
//	The numeric and character entities converted are listed in the
//	entities.c file.
//
// PARAMETERS
//
//	c	This iterator is to be positioned at the character past the
//		'&'; if an entity is found, it is left after the ';'.
//
//	end	The iterator marking the end of the file.
//
// RETURN VALUE
//
//	Returns the ASCII equivalent of the entity or ' ' (space) if either
//	there is no equivalent or the entity is malformed.
//
// EXAMPLE
//
//	The references in "r&eacute;sum&#233;" will be converted to the letter
//	'e' resulting in the "resume" string.
//
// SEE ALSO
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs.  "On SGML and HTML: SGML
//	constructs used in HTML: Entities," HTML 4.0 Specification, section
//	3.2.3, World Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/intro/sgmltut.html#h-3.2.3
//
//	International Standards Organization.  "ISO 8859-1: Information
//	Processing -- 8-bit single-byte coded graphic character sets -- Part 1:
//	Latin alphabet No. 1," 1987.
//
//	International Standards Organization.  "ISO 8879: Information
//	Processing -- Text and Office Systems -- Standard Generalized Markup
//	Language (SGML)," 1986.
//
//*****************************************************************************
{
	////////// See if it's a numeric character reference //////////////////

	bool const is_num = (c != end && *c == '#');
	bool const is_hex = (is_num && ++c != end && (*c == 'x' || *c == 'X'));
	if ( is_hex ) ++c;

	////////// Find the terminating ';' ///////////////////////////////////

	char entity_buf[ Entity_Max_Size + 1 ];
	int entity_len = 0;

	while ( c != end && *c != ';' ) {
		if ( ++entity_len > Entity_Max_Size )
			return ' ';			// give up looking
		entity_buf[ entity_len - 1 ] = *c++;
	}
	if ( c == end )					// didn't find it
		return ' ';
	++c;						// put past ';'

	entity_buf[ entity_len ] = '\0';

	////////// Look up character entity reference /////////////////////////

	if ( !is_num )
		return char_entity_map::instance()[ entity_buf ];

	////////// Parse a numeric character reference ////////////////////////

	register unsigned n = 0;
	for ( register char const *e = entity_buf; *e; ++e ) {
		if ( is_hex ) {
			if ( !isxdigit( *e ) )		// bad hex num
				return ' ';
			n = (n << 4) | ( isdigit( *e ) ?
				*e - '0' : tolower( *e ) - 'a' + 10
			);
		} else {
			if ( !isdigit( *e ) )		// bad dec num
				return ' ';
			n = n * 10 + *e - '0';
		}
	}

	return	n < sizeof( iso8859_map ) / sizeof( iso8859_map[0] ) ?
		iso8859_map[ n ] : ' ';
}

//*****************************************************************************
//
// SYNOPSIS
//
	bool find_attribute(
		file_vector::const_iterator &begin,
		file_vector::const_iterator &end,
		char const *attribute
	)
//
// DESCRIPTION
//
//	Given an attribute's name, find its value within an HTML or XHTML
//	element's start tag, e.g., find the value of the NAME attribute within
//	the META element.  Case is irrelevant.
//
//	The HTML 4.0 specification is vague in stating whether whitespace is
//	legal around the '=' character separating an attribute's name from its
//	value; hence, this function is lenient in that it will consider:
//
//		NAME = "Author"
//	and:
//		NAME="Author"
//
//	equivalent.
//
// PARAMETERS
//
//	begin		The iterator marking the beginning of where to look. If
//			the attribute is found, this iterator is repositioned
//			to be at the first character of the value; otherwise,
//			it is not touched.
//
//	end		The iterator marking the end of where to look (usually
//			positioned at the closing '>' character of the HTML
//			or XHTML tag).  If the attribute is found, this
//			iterator is repositioned to one past the value's end;
//			otherwise, it is not touched.
//
//	attribute	The name of the attribute to find; it must be in lower
//			case.
//
// RETURN VALUE
//
//	Returns true only if the attribute was found.
//
// CAVEAT
//
//	This function only handles attributes with explicit values, i.e., those
//	followed by an '=' and value, and not Boolean attributes.  But, since
//	it was written with the intent of only parsing attributes in META tags,
//	namely NAME and CONTENT, it's OK.
//
// SEE ALSO
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs.  "On SGML and HTML: SGML
//	constructs used in HTML: Attributes," HTML 4.0 Specification, section
//	3.2.2, World Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/intro/sgmltut.html#h-3.2.2
//
//*****************************************************************************
{
	register file_vector::const_iterator c = begin;
	while ( c != end ) {
		if ( !isalpha( *c ) ) {
			++c;
			continue;
		}
		//
		// Found the start of a potentially matching attribute name.
		//
		register char const *a = attribute;
		while ( c != end && to_lower( *c ) == *a )
			++c, ++a;
		while ( c != end && ( isalpha( *c ) || *c == '-' ) )
			++c;
		while ( c != end && isspace( *c ) )
			++c;
		if ( c == end )
			break;
		if ( *c != '=' )		// we do only NAME=VALUE form
			continue;
		while ( ++c != end && isspace( *c ) )
			;
		if ( c == end )
			break;
		//
		// Determine the span of the attribute's value: if it started
		// with a quote, it's terminated only by the matching closing
		// quote; if not, it's terminated by a whitespace character (or
		// running into 'end').
		//
		// This is more lenient than the HTML 4.0 specification in that
		// it allows non-quoted values to contain characters other than
		// the set [A-Za-z0-9.-], i.e., any non-whitespace character.
		//
		char const quote = ( *c == '"' || *c == '\'' ) ? *c : 0;
		if ( quote && ++c == end )
			break;
		file_vector::const_iterator const b = c;
		for ( ; c != end; ++c )
			if ( quote ) {		// stop at matching quote only
				if ( *c == quote )
					break;
			} else if ( isspace( *c ) )
				break;		// stop at whitespace

		if ( !*a ) {			// attribute name matched...
			begin = b, end = c;	// ...and got entire value  :)
			return true;
		}
		if ( c == end )
			break;
		++c;				// attribute name didn't match
	}
	return false;
}

//*****************************************************************************
//
// SYNOPSIS
//
	char const* grep_title( file_vector const &file )
//
// DESCRIPTION
//
//	Scan ("grep") through the first num_title_lines lines in an HTML or
//	XHTML file looking for <TITLE>...</TITLE> tags to extract the title.
//	Every non-space whitespace character in the title is converted to a
//	space; leading and trailing spaces are removed.
//
//	If the length of the title exceeds Title_Max_Size, then the title is
//	truncated and the last 3 characters of the truncated title are replaced
//	with an elipsis ("...").
//
// PARAMETERS
//
//	file	The file presumed to be an HTML or XHTML file.
//
// RETURN VALUE
//
//	Returns the title string or null if no title can be found.
//
// EXAMPLE
//
//	Given:
//
//		<TITLE>
//		This is
//		a title
//		</TITLE>
//
//	returns:
//
//		This is a title
//
// SEE ALSO
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs.  "The global structure of
//	an HTML document: The document head: The TITLE element," HTML 4.0
//	Specification, section 7.4.2, orld Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/intro/sgmltut.html#h-7.4.2
//
//*****************************************************************************
{
	static char const *const title_tag[] = {	// tag_index
		"title",				//	0
		"/title"				//	1
	};
	int tag_index = 0;
	int lines = 0;

	//
	// <TITLE>This is a title</TITLE>
	//        |              |
	//        after          before
	//
	// Mark the positions after the closing '>' of the start tag and before
	// the opening '<' of the end tag.  What's in between is the title.
	//
	file_vector::const_iterator after, before;

	file_vector::const_iterator c = file.begin();
	while ( c != file.end() ) {
		if ( *c == '\n' && ++lines > num_title_lines ) {
			//
			// Didn't find <TITLE> within first num_title_lines
			// lines of file: forget it.
			//
			return 0;
		}

		if ( *c != '<' ) {			// not a tag: forget it
			++c;
			continue;
		}

		//
		// Found the start of an HTML or XHTML tag: mark the position
		// before it in case it turns out to be the </TITLE> tag.
		//
		before = c++;

		if ( is_html_comment( c, file.end() ) )
			continue;

		//
		// Is the HTML or XHTML tag a TITLE tag?
		//
		bool const found_title_tag =
			tag_cmp( c, file.end(), title_tag[ tag_index ] );
		skip_html_tag( c, file.end() );		// skip until '>'
		if ( !found_title_tag )			// wrong tag
			continue;

		if ( tag_index == 0 ) {
			//
			// Found the <TITLE> tag: mark the position after it
			// and begin looking for the </TITLE> tag.
			//
			after = c;
			++tag_index;
			continue;
		}

		////////// Found the entire title /////////////////////////////

		// Remove leading spaces
		while ( after < before && isspace( *after ) )
			++after;

		// Remove trailing spaces
		while ( after < --before && isspace( *before ) ) ;
		++before;

		static char title[ Title_Max_Size + 1 ];
		int len = before - after;
		if ( len > Title_Max_Size ) {
			::strncpy( title, after, Title_Max_Size );
			::strcpy( title + Title_Max_Size - 3, "..." );
			len = Title_Max_Size;
		} else {
			::copy( after, before, title );
		}
		title[ len ] = '\0';

		// Normalize all whitespace chars to space chars.
		for ( register char *p = title; *p; ++p )
			if ( isspace( *p ) )
				*p = ' ';

		return title;
	}

	//
	// The file has less than num_title_lines lines and no <TITLE> was
	// found.
	//
	return 0;
}

//*****************************************************************************
//
// SYNOPSIS
//
	bool is_html_comment(
		register file_vector::const_iterator &c,
		register file_vector::const_iterator end
	)
//
// DESCRIPTION
//
//	Checks to see if the current HTML or XHTML element is the start of a
//	comment. If so, skip it scanning for the closing "-->" character
//	sequence.  The HTML specification permits whitespace between the "--"
//	and the ">" (for some strange reason).  Unlike skipping an ordinary
//	HTML or XHTML tag, quotes are not significant and no attempt must be
//	made either to "balance" them or to ignore what is in between them.
//
//	This function is more lenient than the HTML 4.0 specification in that
//	it allows for a string of hyphens within a comment since this is common
//	in practice; the specification considers this to be an error.
//
// PARAMETERS
//
//	c	The iterator to use.  It is presumed to be positioned at the
//		first character after the '<'.  If the tag is the start of a
//		comment, it is repositioned at the first character past the
//		tag, i.e., past the "!--"; otherwise, it is not touched.
//
//	end	The iterator marking the end of the file.
//
// RETURN VALUE
//
//	Returns true only if the current element is the beginning of a comment.
//
// SEE ALSO
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs.  "On SGML and HTML: SGML
//	constructs used in HTML: Comments," HTML 4.0 Specification, section
//	3.2.4, World Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/intro/sgmltut.html#h-3.2.4
//
//*****************************************************************************
{
	if ( tag_cmp( c, end, "!--" ) ) {
		while ( c != end ) {
			if ( *c++ != '-' )
				continue;
			while ( c != end && *c == '-' )
				++c;
			while ( c != end && isspace( *c ) )
				++c;
			if ( c != end && *c++ == '>' )
				break;
		}
		return true;
	}
	return false;
}

//*****************************************************************************
//
// SYNOPSIS
//
	void parse_html_tag(
		register file_vector::const_iterator &c,
		register file_vector::const_iterator end,
		bool is_new_file
	)
//
// DESCRIPTION
//
//	This function does everything skip_html_tag() does but additionally
//	does extra parsing for certain HTML or XHTML elements:
//
//	1. If the tag contains a CLASS attribute whose value is among the set
//	   of class names specified as those not to index, then all the text up
//	   to the tag that ends the element will not be indexed.
//
//	   For an element that has an optional end tag, "the tag that ends the
//	   element" is either the element's end tag or a tag of another element
//	   that implicitly ends it; for an element that does not have an end
//	   tag, "the tag that ends the element" is the element's start tag.
//
//	2. If the tag contains a TITLE attribute, it indexes the words of its
//	   value.
//
//	3. If the tag is an AREA, IMG, or INPUT element and contains an ALT
//	   attribute, it indexes the words of its value.
//
//	4. If the tag is a META element, it parses its NAME and CONTENT
//	   attributes indexing the words of the latter associated with the
//	   former.
//
//	5. If the tag is an OBJECT element and contains a STANDBY attribute, it
//	   indexes the words of its value.
//
//	6. If the tag is a TABLE element and contains a SUMMARY attribute, it
//	   indexes the words of its value.
//
// PARAMETERS
//
//	c		The iterator to use.  It must be positioned at the
//			character after the '<'; it is repositioned at the
//			first character after the '>'.
//
//	end		The iterator marking the end of the file.
//
//	is_new_file	If true, we are just starting to index a new file.
//
// SEE ALSO
//
//	Dave Raggett, Arnaud Le Hors, and Ian Jacobs. "The global structure of
//	an HTML document: The document head: The title attribute," HTML 4.0
//	Specification, section 7.4.3, World Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/REC-html40/struct/global.html#adef-title
//
//	---.  "The global structure of an HTML document: The document head:
//	Meta data," HTML 4.0 Specification, section 7.4.4, World Wide Web
//	Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/struct/global.html#h-7.4.4
//
//	---.  "The global structure of an HTML document: The document body:
//	Element identifiers: the id and class attributes," HTML 4.0
//	Specification, section 7.5.2, World Wide Web Consortium, April 1998.
//		http://www.w3.org/TR/PR-html40/struct/global.html#h-7.5.2
//
//	---. "Tables: Elements for constructing tables: The TABLE element,"
//	HTML 4.0 Specification, section 11.2.1, World Wide Web Consortium,
//	April 1998.
//		http://www.w3.org/TR/REC-html40/struct/tables.html#adef-summary
//
//	---. "Objects, Images, and Applets: Generic inclusion: the OBJECT
//	element," HTML 4.0 Specification, section 13.3, World Wide Web
//	Consortium, April 1998.
//		http://www.w3.org/TR/REC-html40/struct/objects.html#adef-standby
//
//	---. "Objects, Images, and Applets: How to specify alternate text,"
//	HTML 4.0 Specification, section 13.8, World Wide Web Consortium, April
//	1998.
//		http://www.w3.org/TR/REC-html40/struct/objects.html#h-13.8
//
//*****************************************************************************
{
	if ( c == end )
		return;
	file_vector::const_iterator tag_begin = c;
	if ( skip_html_tag( c, end ) || *tag_begin == '!' )
		return;
	file_vector::const_iterator tag_end = c - 1;
	bool const is_end_tag = *tag_begin == '/';

	////////// Deal with elements of a class not to index /////////////////

	if ( !exclude_class_names.empty() ) {		// else, don't bother
		char tag[ Tag_Name_Max_Size + 2 ];	// 1 for '/', 1 for null
		{ // local scope
		//
		// Copy only the tag name by stopping at a whitespace character
		// (or running into tag_end); also convert it to lower case.
		// (We don't call to_lower() in util.c so as not to waste time
		// copying the entire tag with its attributes since we only
		// want the tag name.)
		//
		register char *to = tag;
		register file_vector::const_iterator from = tag_begin;
		while ( from != tag_end && !isspace( *from ) ) {
			//
			// Check to see if the tag is too long to be a valid
			// one for an HTML element: if it is, invalidate it by
			// writing "garbage" into the tag so something like
			// "BLOCKQUOTED" (an invalid tag) won't match
			// "BLOCKQUOTE" (a valid tag) when one letter shorter
			// and throw off element closures.
			//
			if ( to - tag >= Tag_Name_Max_Size + is_end_tag ) {
				to = tag;
				*to++ = '\1';
				break;
			}
			*to++ = to_lower( *from++ );
		}
		*to = '\0';
		} // local scope

		//
		// The element_stack keeps track of all the HTML or XHTML
		// elements we encounter until they are closed.  The first
		// member of the pair is a pointer to the element_map's
		// value_type or the key/value pair of the map where the key is
		// the element name and the value is a pointer to the element.
		// The second member of the pair is a flag indicating whether
		// the words between the start and end tags of that element are
		// not to be indexed (true = "don't index").
		//
		// Note: I can't use an actual STL stack since I need to be
		// able to clear the entire stack and, unfortunately, clear()
		// isn't supported for stacks...an oversight in STL, IMHO.
		//
		typedef	vector< pair< element_map::value_type const*, bool > >
			stack_type;
		static stack_type element_stack;

		if ( is_new_file )
			element_stack.clear();

		////////// Close open element(s) //////////////////////////////

		while ( !element_stack.empty() &&
			element_stack.back().first->second.close_tags.contains(
				tag
			)
		) {
			//
			// This tag closes the currently open element.
			//
			if ( element_stack.back().second ) {
				//
				// The currently open element is a member of
				// one of the classes not being being indexed:
				// decrement exclude_class_count.
				//
				--exclude_class_count;
			}
			char const *const
				start_tag = element_stack.back().first->first;
			element_stack.pop_back();

			//
			// We have to stop closing elements if we encounter the
			// start tag matching the end tag.
			//
			if ( !::strcmp( tag + 1, start_tag ) )
				break;
		}

		if ( is_end_tag ) {
			//
			// The tag is an end tag: it doesn't have attributes.
			//
			return;
		}

		////////// Look for CLASS attribute ///////////////////////////

		bool is_no_index_class = false;

		file_vector::const_iterator class_begin = tag_begin;
		file_vector::const_iterator class_end = tag_end;
		if ( find_attribute( class_begin, class_end, "class" ) ) {
			//
			// CLASS attribute values can contain multiple classes
			// separated by whitespace: we must iterate over all of
			// them to see if one of them is among the set not to
			// index.
			//
			char *names = to_lower( class_begin, class_end );
			register char const *name;
			while ( name = ::strtok( names, " \f\n\r\t\v" ) ) {
				if ( exclude_class_names.contains( name ) ) {
					is_no_index_class = true;
					break;
				}
				names = 0;
			}
		}

		element_map const &elements = element_map::instance();
		element_map::const_iterator const e = elements.find( tag );
		if ( e != elements.end() ) {
			//
			// We found the element in our internal table: now do
			// different stuff depending upon whether its end tag
			// is forbidden or not.
			//
			if ( e->second.end_tag != element::forbidden ) {
				//
				// The element's end tag isn't forbidden, so we
				// have to keep track of it on the stack and
				// whether the text between the start and end
				// tags should be indexed or not.
				//
				// Note that we have to keep track of all HTML
				// or XHTML elements even if they are not
				// members of a class not to be indexed because
				// they could be nested inside of an element
				// that is, e.g.:
				//
				//	<DIV CLASS=ignore>
				//	<SPAN CLASS=other>Hello</SPAN>
				//	</DIV>
				//
				element_stack.push_back( make_pair(
					&*e, is_no_index_class
				) );
				if ( is_no_index_class ) {
					//
					// A class name in the value of this
					// element's CLASS attribute is among
					// the set not to index: increment
					// exclude_class_count.
					//
					++exclude_class_count;
				}
			}
			if ( is_no_index_class ) {
				//
				// Regardless of this element's end tag, it is
				// to be ignored, so we can stop parsing it.
				//
				return;
			}
		} else {
			// We didn't find the element in our internal table:
			// ignore it.  We really should do something better
			// because this could potentially mess up the proper
			// closing of elements, but, since we know nothing
			// about this element, there's nothing better that can
			// be done.
		}
	}

	if ( is_end_tag ) {
		//
		// The tag is an end tag: it doesn't have attributes.
		//
		return;
	}

	////////// Look for a TITLE attribute /////////////////////////////////

	file_vector::const_iterator title_begin = tag_begin;
	file_vector::const_iterator title_end   = tag_end;
	if ( find_attribute( title_begin, title_end, "title" ) )
		index_words( title_begin, title_end, true );

	////////// Look for an ALT attribute //////////////////////////////////

	if (	tag_cmp( tag_begin, tag_end, "area"  ) ||
		tag_cmp( tag_begin, tag_end, "img"   ) ||
		tag_cmp( tag_begin, tag_end, "input" )
	) {
		if ( find_attribute( tag_begin, tag_end, "alt" ) )
			index_words( tag_begin, tag_end, true );
		return;
	}

	////////// Parse a META element ///////////////////////////////////////

	if ( tag_cmp( tag_begin, tag_end, "meta" ) ) {
		file_vector::const_iterator
			name_begin = tag_begin, name_end = tag_end,
			content_begin = tag_begin, content_end = tag_end;

		if (	find_attribute( name_begin, name_end, "name" ) &&
			find_attribute( content_begin, content_end, "content" )
		) {
			//
			// Canonicalize the value of the NAME attribute to
			// lower case.
			//
			char const *const
				name = to_lower( name_begin, name_end );

			//
			// Do not index the words in the value of the CONTENT
			// attribute if either the value of the NAME attribute
			// is among the set of meta names to exclude or not
			// among the set to include.
			//
			if ( exclude_meta_names.contains( name ) ||
				!include_meta_names.empty() &&
				!include_meta_names.contains( name )
			)
				return;

			//
			// Look up the vlaue of the NAME attribute to get its
			// associated unique integer ID.
			//
			meta_map::const_iterator const
				i = meta_names.find( name );
			int meta_id;
			if ( i != meta_names.end() )
				meta_id = i->second;
			else {			// new meta name: add it
				//
				// Do this in two statements intentionally
				// because C++ doesn't guarantee that the RHS
				// of assignment is evaluated first.
				//
				meta_id = meta_names.size();
				meta_names[ ::strdup( name ) ] = meta_id;
			}

			//
			// Index the words in the value of the CONTENT
			// attribute marking them as being associated with the
			// value of NAME attribute.
			//
			index_words( content_begin, content_end, true, meta_id);
		}
		return;
	}

	////////// Look for a STANDBY attribute ///////////////////////////////

	if ( tag_cmp( tag_begin, tag_end, "object" ) ) {
		if ( find_attribute( tag_begin, tag_end, "standby" ) )
			index_words( tag_begin, tag_end, true );
		return;
	}

	////////// Look for a SUMMARY attribute ///////////////////////////////

	if ( tag_cmp( tag_begin, tag_end, "table" ) ) {
		if ( find_attribute( tag_begin, tag_end, "summary" ) )
			index_words( tag_begin, tag_end, true );
		return;
	}
}

//*****************************************************************************
//
// SYNOPSIS
//
	bool skip_html_tag(
		register file_vector::const_iterator &c,
		register file_vector::const_iterator end
	)
//
// DESCRIPTION
//
//	Scan for the closing '>' of an HTML or XHTML tag skipping all
//	characters until it's found.  It takes care to ignore any '>' inside
//	either single or double quotation marks.  It properly handles HTML and
//	XHTML comments.
//
// PARAMETERS
//
//	c	The iterator to use.  It is presumed to start at any position
//		after the '<' and before the '>'; it is left at the first
//		character after the '>'.
//
//	end	The iterator marking the end of the file.
//
// RETURN VALUE
//
//	Returns true only if the tag is an HTML or XHTML comment.
//
//*****************************************************************************
{
	if ( is_html_comment( c, end ) )
		return true;

	register char quote = '\0';
	while ( c != end ) {
		if ( quote ) {			// ignore everything...
			if ( *c++ == quote )	// ...until matching quote
				quote = '\0';
			continue;
		}
		if ( *c == '\"' || *c == '\'' ) {
			quote = *c++;		// start ignoring stuff
			continue;
		}
		if ( *c++ == '>' )		// found it  :)
			break;
	}

	return false;
}

//*****************************************************************************
//
// SYNOPSIS
//
	bool tag_cmp(
		file_vector::const_iterator &c,
		register file_vector::const_iterator end,
		register char const *tag
	)
//
// DESCRIPTION
//
//	Compares the tag name starting at the given iterator to the given
//	string.  Case is irrelevant.
//
// PARAMETERS
//
//	c	The iterator to use.  It is presumed to be positioned at the
//		first character after the '<'.  If the tag name matches, it is
//		repositioned at the first character past the name; otherwise,
//		it is not touched.
//
//	end	The iterator marking the end of the file.
//
//	tag	The string to compare against; it must be in lower case.
//
// RETURN VALUE
//
//	Returns true only if the tag matches.
//
//*****************************************************************************
{
	register file_vector::const_iterator d = c;
	while ( *tag && d != end && *tag++ == to_lower( *d++ ) ) ;
	return *tag ? false : c = d;
}