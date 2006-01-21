/*
**      PJL C++ Library
**      option_stream.c
**
**      Copyright (C) 1999  Paul J. Lucas
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

// standard
#include <algorithm>                            /* for copy() */
#include <cstdlib>                              /* for abort(3) */
#include <cstring>
#include <iterator>
#include <iostream>

// local
#include "platform.h"
#include "option_stream.h"

using namespace std;

namespace PJL {

#define ERROR   os.err_ << os.argv_[0] << ": error: option "

//*****************************************************************************
//
// SYNOPSIS
//
        option_stream::option_stream(
            int argc, char *argv[], spec const specs[], ostream &err
        )
//
// DESCRIPTION
//
//      Construct (initialize) an option_stream.
//
// PARAMETERS
//
//      argc    The number of arguments.
//
//      argv    A vector of the arguments; argv[argc] is null.
//
//      specs   A vector of option specifications, i.e., the options that are
//              allowed and their argument parameters.
//
//      err     The ostream to emit error messages to.
//
//*****************************************************************************
    : argc_( argc ), argv_( argv ), specs_( specs ), err_( err ), argi_( 0 ),
      next_c_( 0 ), end_( false )
{
    // do nothing else
}

//*****************************************************************************
//
// SYNOPSIS
//
        option_stream& operator>>( option_stream &os, option_stream::option &o )
//
// DESCRIPTION
//
//      Parse and extract an option from an option stream (argv values).
//      Options begin with either a '-' for short options or a "--" for long
//      options.  Either a '-' or "--" by itself explicitly ends the options;
//      however, the difference is that '-' is returned as the first non-option
//      whereas "--" is skipped entirely.
//
//      When there are no more options, the option_stream converts to bool as
//      false.  The option_stream's shift() member is the number of options
//      parsed which the caller can use to adjust argc and argv.
//
//      Short options can take an argument either as the remaining characters
//      of the same argv or in the next argv unless the next argv looks like an
//      option by beginning with a '-').
//
//      Long option names can be abbreviated so long as the abbreviation is
//      unambiguous.  Long options can take an argument either directly after a
//      '=' in the same argv or in the next argv (but without an '=') unless
//      the next argv looks like an option by beginning with a '-').
//
// PARAMETERS
//
//      os  The option_stream to extract options from.
//
//      o   The option to deposit into.
//
// RETURN VALUE
//
//      Returns the passed-in option_stream.
//
//*****************************************************************************
{
    register char *arg;
    register option_stream::spec const *s, *found = 0;
    bool was_short_option = false;

    o.short_name_ = '\0';

    if ( os.next_c_ && *os.next_c_ ) {
        //
        // We left off within a grouped set of short options taking no
        // arguments, i.e., instead of -a -b -c, the user did -abc and next_c_
        // points to a character past 'a'.
        //
        arg = os.next_c_;
        goto short_option;
    }

    if ( ++os.argi_ >= os.argc_ )               // no more arguments
        goto the_end;
    arg = os.argv_[ os.argi_ ];
    if ( !arg || *arg != '-' || !*++arg )       // no more options
        goto the_end;

    if ( *arg == '-' ) {
        if ( !*++arg ) {                        // "--": end of options
            ++os.argi_;
            goto the_end;
        }
        was_short_option = false;

        //
        // Got the start of a long option: find the last character of its name.
        //
        char *end;
        for ( end = arg; *end && *end != '='; ++end ) ;

        //
        // Now look through the options table for a match.
        //
        for ( s = os.specs_; s->long_name; ++s ) {
            unsigned const len = end - arg;
            if ( ::strncmp( arg, s->long_name, len ) )
                continue;
            if ( ::strlen( s->long_name ) == len ) {
                found = s;                      // exact match
                break;
            }
            if ( !found ) {
                found = s;                      // partial match
                continue;
            }
            ERROR << '"';
            ::copy( arg, end, ostream_iterator<char>(os.err_, "") );
            os.err_ << "\" is ambiguous\n";
            goto option_error;
        }
        if ( !found ) {
            ERROR << '"';
            ::copy( arg, end, ostream_iterator<char>(os.err_, "") );
            os.err_ << "\" unrecognized\n";
            goto option_error;
        }

        //
        // Got a long option: now see about its argument.
        //
        arg = 0;
        switch ( found->arg_type ) {

            case option_stream::os_arg_none:
                if ( *end == '=' ) {
                    ERROR << '"' << found->long_name
                          << "\" takes no argument\n";
                    goto option_error;
                }
                break;

            case option_stream::os_arg_req:
            case option_stream::os_arg_opt:
                if ( *end == '=' ) {
                    arg = ++end;
                    break;
                }
                goto arg_next;

            default:
                goto bad_spec;
        }

        goto check_arg;
    }

short_option:
    was_short_option = true;
    //
    // Got a single '-' for a short option: look for it in the specs.
    //
    for ( s = os.specs_; s->short_name; ++s )
        if ( *arg == s->short_name ) {
            found = s;
            break;
        }
    if ( !found ) {
        ERROR << '"' << *arg << "\" unrecognized\n";
        goto option_error;
    }

    //
    // Found the short option: now see about its argument.
    //
    switch ( found->arg_type ) {

        case option_stream::os_arg_none:
            //
            // Set next_c_ in case the user gave a grouped set of short options
            // (see the comment near the beginning) so we can pick up where we
            // left off on the next call.
            //
            os.next_c_ = arg + 1;
            arg = 0;
            break;

        case option_stream::os_arg_req:
        case option_stream::os_arg_opt:
            //
            // Reset next_c_ since an option with either a required or optional
            // argument terminates a grouped set of options.
            //
            os.next_c_ = 0;

            if ( !*++arg ) {
                //
                // The argument, if any, is given in the next argv.
                //
arg_next:       if ( ++os.argi_ >= os.argc_ )
                    break;
                arg = os.argv_[ os.argi_ ];
                if ( *arg == '-' ) {
                    //
                    // The next argv looks like an option so assume it is one
                    // and that there is no argument for this option.
                    //
                    arg = 0;
                }
            }
            break;

        default:
            goto bad_spec;
    }

check_arg:
    if ( (arg && *arg) || found->arg_type != option_stream::os_arg_req ) {
        o.short_name_ = found->short_name;
        o.arg_ = arg;
        return os;
    }
    ERROR << '"';
    if ( was_short_option )
        os.err_ << found->short_name;
    else
        os.err_ << found->long_name;
    os.err_ << "\" requires an argument\n";
    // fall through
option_error:
    o.arg_ = '\0';
    return os;
the_end:
    os.end_ = true;
    return os;
bad_spec:
    ERROR << "invalid option argument spec: " << found->arg_type << '\n';
    ::abort();
}

} // namespace PJL

//*****************************************************************************

/*#define TEST_OPTION_STREAM /**/
#ifdef  TEST_OPTION_STREAM

using namespace PJL;

int main( int argc, char *argv[] ) {
    static option_stream::spec const spec[] = {
        "n-no-arg",     0, 'n',
        "m-no-arg",     0, 'm',
        "r-req-arg",    1, 'r',
        "o-opt-arg",    2, 'o',
        0,
    };
    option_stream opt_in( argc, argv, spec );
    option_stream::option opt;
    while ( opt_in >> opt ) {
        switch ( opt ) {
            case 'm':
                cout << "got --m-no-arg\n";
                break;
            case 'n':
                cout << "got --n-no-arg\n";
                break;
            case 'r':
                cout << "got --r-req-arg=" << opt.arg() << '\n';
                break;
            case 'o':
                cout << "got --o-opt-arg=";
                cout << (opt.arg() ? opt.arg() : "(null)") << '\n';
                break;
            default:
                cout << "got weird=" << (int)(char)opt << '\n';
        }
    }
    cout << "shift=" << opt_in.shift() << '\n';

    argc -= opt_in.shift();
    argv += opt_in.shift();

    cout << "argc=" << argc << endl;
    cout << "first non-option=" << (argv[0] ? argv[0] : "(null)") << endl;
}

#endif  /* TEST_OPTION_STREAM */
/* vim:set et sw=4 ts=4: */