/*
**      SWISH++
**      src/mod/id3/id3v1.h
**
**      Copyright (C) 2002  Paul J. Lucas
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

#ifdef MOD_id3

#ifndef id3v1_h
#define id3v1_h

// local
#include "util.h"
#include "word_util.h"

int const ID3v1_Tag_Size    = 128;

enum ID3v1_Field_Size {
    ID3v1_Title_Size    = 30,
    ID3v1_Artist_Size   = 30,
    ID3v1_Album_Size    = 30,
    ID3v1_Year_Size     = 4,
    ID3v1_Comments_Size = 30,
    ID3v1_Genre_Size    = 1
};

struct id3v1_genre {
    char const* name;
    int         length;
};

typedef unsigned id3v1_genre_id;

extern id3v1_genre const id3v1_genre_table[ 126 ];

//*****************************************************************************
//
// SYNOPSIS
//
        inline id3v1_genre const *find_genre( id3v1_genre_id id )
//
// DESCRIPTION
//
//      Finds the text label for a genre given its ID.
//
// PARAMETERS
//
//      id  The ID3v1 genre ID.
//
// RETURN VALUE
//
//      Returns said text label or null if the ID if out of range.
//
//*****************************************************************************
{
    return  id < NUM_ELEMENTS( id3v1_genre_table ) ?
            &id3v1_genre_table[ id ] : 0;
}

//*****************************************************************************
//
// SYNOPSIS
//
        inline bool has_id3v1_tag( char const *&c, char const *end )
//
// DESCRIPTION
//
//      Check the file for the presence of an ID3v1 tag.
//
// PARAMETERS
//
//      c       A pointer to the beginning of the file.
//
//      end     A pointer to one past the end of the file.
//
// RETURN VALUE
//
//      Returns true only if the file contains an ID3v1 tag.
//
//*****************************************************************************
{
    if ( end - c < ID3v1_Tag_Size )
        return false;
    c = end - ID3v1_Tag_Size;
    return move_if_match( c, end, "TAG" );
}

#endif /* id3v1_h */

#endif /* MOD_id3 */
/* vim:set et sw=4 ts=4: */
