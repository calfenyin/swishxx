/*
**      SWISH++
**      src/mod/html/ExcludeClass.h
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

#ifndef ExcludeClass_H
#define ExcludeClass_H

// local
#include "conf_set.h"

///////////////////////////////////////////////////////////////////////////////

/**
 * An %ExcludeClass is-a conf_set containing the set of CLASS names to exclude
 * during indexing.
 *
 * This is the same as index's \c -C command-line option.
 */
class ExcludeClass : public conf_set {
public:
  ExcludeClass() : conf_set( "ExcludeClass" ) { }
  CONF_SET_ASSIGN_OPS( ExcludeClass )
};

extern ExcludeClass exclude_class_names;

///////////////////////////////////////////////////////////////////////////////

#endif  /* ExcludeClass_H */
/* vim:set et sw=2 ts=2: */
