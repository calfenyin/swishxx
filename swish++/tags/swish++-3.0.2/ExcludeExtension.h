/*
**	SWISH++
**	ExcludeExtension.h
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

#ifndef ExcludeExtension_H
#define ExcludeExtension_H

// local
#include "conf_set.h"

//*****************************************************************************
//
// SYNOPSIS
//
	class ExcludeExtension : public conf_set
//
// DESCRIPTION
//
//	An ExcludeExtension is-a conf_set containing the set of filename
//	extensions of files to exclude during either indexing or extraction.
//
//	This is the same as either index's or extract's -E command-line
//	option.
//
//*****************************************************************************
{
public:
	ExcludeExtension() : conf_set( "ExcludeExtension" ) { }
};

#endif	/* ExcludeExtension_H */