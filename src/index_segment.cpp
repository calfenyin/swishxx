/*
**      SWISH++
**      src/index_segment.cpp
**
**      Copyright (C) 1998-2015  Paul J. Lucas
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
#include "index_segment.h"

using namespace PJL;

///////////////////////////////////////////////////////////////////////////////

void index_segment::set_index_file( mmap_file const &file, segment_id id ) {
  auto c = begin_ = file.begin();
  auto p = reinterpret_cast<size_type const*>( c );
  num_entries_ = p[0];
  for ( int i = id; i > 0; --i ) {
    c += sizeof( num_entries_ ) + num_entries_ * sizeof( off_t );
    p = reinterpret_cast<size_type const*>( c );
    num_entries_ = p[0];
  } // for
  offset_ = reinterpret_cast<off_t const*>( &p[1] );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
