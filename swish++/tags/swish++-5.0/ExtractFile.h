/*
**	SWISH++
**	ExtractFile.h
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

#ifndef ExtractFile_H
#define ExtractFile_H

// local
#include "conf_set.h"

//*****************************************************************************
//
// SYNOPSIS
//
	class ExtractFile : public conf_set
//
// DESCRIPTION
//
//	An ExtractFile is-a conf_var containing the set of filename patterns
//	to include during extraction.
//
//	This is the same as extract's -e command-line option.
//
//*****************************************************************************
{
public:
	ExtractFile() : conf_set( "ExtractFile" ) { }
	CONF_SET_ASSIGN_OPS( ExtractFile )
private:
	virtual void	parse_value( char *line );
	virtual void	reset() { clear(); }
};

#endif	/* ExtractFile_H */