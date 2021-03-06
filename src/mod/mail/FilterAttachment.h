/*
**      SWISH++
**      src/mod/mail/FilterAttachment.h
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

#ifndef FilterAttachment_H
#define FilterAttachment_H

// local
#include "conf_filter.h"

///////////////////////////////////////////////////////////////////////////////

/**
 * A %FilterAttachment is-a conf_filter for mapping a MIME types to a filter
 * (being a Unix process called via command-line).  Certain MIME types need to
 * be filtered first, e.g., converted to plain text.
 */
class FilterAttachment : public conf_filter {
public:
  FilterAttachment() : conf_filter( "FilterAttachment" ) { }
};

extern FilterAttachment attachment_filters;

///////////////////////////////////////////////////////////////////////////////

#endif /* FilterAttachment_H */
/* vim:set et sw=2 ts=2: */
