/*
**      SWISH++
**      src/encodings/quoted_printable.cpp
**
**      Copyright (C) 2002-2015  Paul J. Lucas
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
#include "encoded_char.h"
#include "util.h"

// standard
#include <cstring>

using namespace std;

///////////////////////////////////////////////////////////////////////////////

encoded_char_range::value_type
encoding_quoted_printable( encoded_char_range::const_pointer,
                           encoded_char_range::const_pointer &c,
                           encoded_char_range::const_pointer end ) {
  //
  // Check to see if the character at the current position is an '=': if not,
  // the character is an ordinary character; if so, the character is a quoted-
  // printable encoded character and needs to be decoded.
  //
  if ( *c != '=' )
    return *c++;
  if ( ++c == end )
    return ' ';

  encoded_char_range::value_type h1;
  while ( true ) {
    h1 = *c++;
    if ( h1 == '\r' ) {
      //
      // The '=' was the last character on a line so this is supposed to be a
      // "soft line break": we therefore have to skip over it entirely making
      // things appear as though it's not even there by returning the character
      // after the break.
      //
      if ( c == end  || (*c == '\n' && ++c == end) )
        return ' ';
      if ( *c != '=' )
        return *c++;
      //
      // The character after the soft line break just so happens to be another
      // '=' so we have to start all over again.
      //
      if ( ++c == end )
        return ' ';
      continue;
    }
    if ( h1 == '\n' ) {
      //
      // Although "soft line breaks" are supposed to be represented by CR-LF
      // pairs, we're being robust here and allowing just an LF by itself.
      //
      if ( c == end )
        return ' ';
      if ( *c != '=' )
        return *c++;
      if ( ++c == end )
        return ' ';
      continue;
    }
    break;
  } // while

  if ( !isxdigit( h1 ) || c == end ) {
    //
    // If it's not a hexadecimal digit or it's the last character, it's
    // malformed.
    //
    return ' ';
  }
  auto const h2 = *c++;
  if ( !isxdigit( h2 ) ) {
    //
    // This shouldn't happen in proper quoted-printable text.
    //
    return ' ';
  }

  return static_cast<encoded_char_range::value_type>(
    //
    // We're being robust by ensuring the hexadecimal characters are upper
    // case.
    //
    ( isdigit( h1 ) ? h1 - '0' : toupper( h1 ) - 'A' + 10 ) << 4 |
    ( isdigit( h2 ) ? h2 - '0' : toupper( h2 ) - 'A' + 10 )
  );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
