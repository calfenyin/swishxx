/*
**      SWISH++
**      src/mod/latex/commands.h
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

#ifndef latex_command_map_H
#define latex_command_map_H

// local
#include "pjl/less.h"

// standard
#include <map>

///////////////////////////////////////////////////////////////////////////////

/**
 * A %command contains the information we need about LaTeX commands.
 */
struct command {
  char const *name;
  char const *action;
};

/**
 * A %command_map is-a map from the character strings for LaTeX commands to
 * instances of the command class declared above.  The only reason for having a
 * derived class rather than a typedef is so that we can have a custom
 * constructor that initializes itself.
 *
 * The constructor is private, however, to ensure that only instance() can be
 * called to initialize and access a single, static instance.
 */
class command_map : public std::map<char const*,command> {
public:
  static command_map const& instance();

private:
  command_map();
};

///////////////////////////////////////////////////////////////////////////////

#endif /* latex_command_map_H */
/* vim:set et sw=2 ts=2: */
