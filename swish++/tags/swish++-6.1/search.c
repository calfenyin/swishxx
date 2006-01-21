/*
**      SWISH++
**      search.c
**
**      Copyright (C) 1998  Paul J. Lucas
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
#include <algorithm>                            /* for binary_search(), etc */
#include <cstdlib>                              /* for exit(2) */
#include <cstring>
#include <functional>                           /* for binary_function<> */
#include <iostream>
#include <iterator>
#include <string>
#include <time.h>                               /* needed by sys/resource.h */
#include <sys/time.h>                           /* needed by FreeBSD systems */
#include <sys/resource.h>                       /* for RLIMIT_* */
#include <utility>                              /* for pair<> */
#include <vector>

// local
#include "auto_vec.h"
#include "classic_formatter.h"
#include "config.h"
#include "exit_codes.h"
#include "file_info.h"
#include "file_list.h"
#include "indexer.h"
#include "IndexFile.h"
#include "index_segment.h"
#include "less.h"
#include "mmap_file.h"
#include "my_set.h"
#include "omanip.h"
#include "option_stream.h"
#include "platform.h"
#include "query.h"
#include "ResultSeparator.h"
#include "ResultsFormat.h"
#include "results_formatter.h"
#include "ResultsMax.h"
#include "search.h"
#include "StemWords.h"
#include "token.h"
#include "util.h"
#ifdef  __SUNPRO_CC
#include "vector_adapter.h"
#endif
#include "version.h"
#include "WordFilesMax.h"
#include "WordPercentMax.h"
#ifdef  FEATURE_word_pos
#include "WordsNear.h"
#endif
#include "word_util.h"
#include "xml_formatter.h"
#ifdef  SEARCH_DAEMON
#include "Group.h"
#ifdef  __APPLE__
#include "LaunchdCooperation.h"
#endif
#include "PidFile.h"
#include "SearchBackground.h"
#include "SearchDaemon.h"
#include "SocketAddress.h"
#include "SocketFile.h"
#include "SocketQueueSize.h"
#include "SocketTimeout.h"
#include "ThreadsMax.h"
#include "ThreadsMin.h"
#include "ThreadTimeout.h"
#include "User.h"
#endif  /* SEARCH_DAEMON */

using namespace PJL;
using namespace std;

typedef pair< int, int > search_result;
//
//  A search_result is an individual search result where the first int is a
//  file index and the second int is that file's rank.

//*****************************************************************************
//
// SYNOPSIS
//
        struct sort_by_rank : binary_function<
            search_result const&, search_result const&, bool
        >
//
// DESCRIPTION
//
//      A sort_by_rank is-a binary_function used to sort search results by rank
//      in descending order (highest rank first).
//
//*****************************************************************************
{
    result_type operator()( first_argument_type a, second_argument_type b ) {
        return a.second > b.second;
    }
};

//*****************************************************************************
//
//  Global declarations
//
//*****************************************************************************

index_segment       directories, files, meta_names, stop_words, words;
IndexFile           index_file_name;
ResultsMax          max_results;
char const*         me;                         // executable name
ResultSeparator     result_separator;
ResultsFormat       results_format;
StemWords           stem_words;
WordFilesMax        word_files_max;
WordPercentMax      word_percent_max;
#ifdef  FEATURE_word_pos
WordsNear           words_near;
#endif
#ifdef  SEARCH_DAEMON
SearchDaemon        daemon_type;
Group               group;
#ifdef  __APPLE__
LaunchdCooperation  launchd_cooperation;
#endif
ThreadsMax          max_threads;
ThreadsMin          min_threads;
PidFile             pid_file_name;
SearchBackground    search_background;
SocketAddress       socket_address;
SocketFile          socket_file_name;
SocketQueueSize     socket_queue_size;
SocketTimeout       socket_timeout;
ThreadTimeout       thread_timeout;
User                user;

void                become_daemon();
#endif  /* SEARCH_DAEMON */
static void         dump_single_word( char const*, ostream& = cout );
static void         dump_word_window( char const*, int, int, ostream& = cout );
static ostream&     write_file_info( ostream&, char const* );

inline omanip< char const* > index_file_info( int index ) {
    return omanip< char const* >( write_file_info, files[ index ] );
}

//*****************************************************************************
//
// SYNOPSIS
//
        int main( int argc, char *argv[] )
//
// DESCRIPTION
//
//      Parse the command line, initialize, call other functions ... the usual
//      things that are done in main().
//
// PARAMETERS
//
//      argc    The number of arguments.
//
//      argv    A vector of the arguments; argv[argc] is null.  Aside from the
//              options below, the arguments form the query.
//
// SEE ALSO
//
//      W. Richard Stevens.  "Advanced Programming in the Unix Environment,"
//      Addison-Wesley, Reading, MA, 1993.
//
//      Bjarne Stroustrup.  "The C++ Programming Language, 3rd ed."
//      Addison-Wesley, Reading, MA, 1997.  pp. 116-118.
//
//*****************************************************************************
{
#include "search_options.c"             /* defines opt_spec */

    me = ::strrchr( argv[0], '/' );     // determine base name ...
    me = me ? me + 1 : argv[0];         // ... of executable

    /////////// Process command-line options //////////////////////////////////

    search_options const opt( &argc, &argv, opt_spec );
    if ( !opt )
        ::exit( Exit_Usage );

    //
    // First, parse the config. file (if any); then override variables with
    // command-line options.
    //
    conf_var::parse_file( opt.config_file_name_arg );

    if ( opt.index_file_name_arg )
        index_file_name = opt.index_file_name_arg;
    if ( opt.max_results_arg )
        max_results = opt.max_results_arg;
    if ( opt.results_format_arg )
        results_format = opt.results_format_arg;
    if ( opt.result_separator_arg )
        result_separator = opt.result_separator_arg;
    if ( opt.stem_words_opt )
        stem_words = true;
    if ( opt.word_files_max_arg )
        word_files_max = opt.word_files_max_arg;
    if ( opt.word_percent_max_arg )
        word_percent_max = opt.word_percent_max_arg;
#ifdef  FEATURE_word_pos
    if ( opt.words_near_arg )
        words_near = opt.words_near_arg;
#endif
#ifdef  SEARCH_DAEMON
    if ( opt.daemon_type_arg )
        daemon_type = opt.daemon_type_arg;
    if ( opt.group_arg )
        group = opt.group_arg;
#ifdef  __APPLE__
    if ( opt.launchd_opt )
        launchd_cooperation = true;
#endif
    if ( opt.max_threads_arg )
        max_threads = opt.max_threads_arg;
    if ( opt.min_threads_arg )
        min_threads = opt.min_threads_arg;
    if ( opt.pid_file_name_arg )
        pid_file_name = opt.pid_file_name_arg;
    if ( opt.search_background_opt
#ifdef  __APPLE__
        || launchd_cooperation
#endif
       )
        search_background = false;
    if ( opt.socket_address_arg )
        socket_address = opt.socket_address_arg;
    if ( opt.socket_file_name_arg )
        socket_file_name = opt.socket_file_name_arg;
    if ( opt.socket_queue_size_arg )
        socket_queue_size = opt.socket_queue_size_arg;
    if ( opt.socket_timeout_arg )
        socket_timeout = opt.socket_timeout_arg;
    if ( opt.thread_timeout_arg )
        thread_timeout = opt.thread_timeout_arg;
    if ( opt.user_arg )
        user = opt.user_arg;
#endif  /* SEARCH_DAEMON */

    //
    // If there were no arguments, see if that's OK given the config. file
    // variables and command-line options.
    //
    bool const dump_something =
        opt.dump_entire_index_opt ||
        opt.dump_meta_names_opt ||
        opt.dump_stop_words_opt;
    if ( !( argc || dump_something
#ifdef  SEARCH_DAEMON
         || daemon_type != "none"
#endif
    ) ) {
        cerr << usage;
        ::exit( Exit_Usage );
    }

    /////////// Load index file ///////////////////////////////////////////////

#ifdef  RLIMIT_AS                       /* SVR4 */
#if defined( SEARCH_DAEMON ) && defined( __APPLE__ )
    if ( daemon_type != "none" && !launchd_cooperation )
#endif
        max_out_limit( RLIMIT_AS );     // max-out total avail. memory
#endif  /* RLIMIT_AS */

    mmap_file const the_index( index_file_name );
    the_index.behavior( mmap_file::bt_random );
    if ( !the_index ) {
        error() << "could not read index from \"" << index_file_name
                << '"' << error_string( the_index.error() );
        ::exit( Exit_No_Read_Index );
    }
    words      .set_index_file( the_index, index_segment::isi_word      );
    stop_words .set_index_file( the_index, index_segment::isi_stop_word );
    directories.set_index_file( the_index, index_segment::isi_dir       );
    files      .set_index_file( the_index, index_segment::isi_file      );
    meta_names .set_index_file( the_index, index_segment::isi_meta_name );

#ifdef  SEARCH_DAEMON
    ////////// Become a daemon ////////////////////////////////////////////////

    if ( !dump_something && daemon_type != "none" )
        become_daemon();                // function does not return
#endif
    ////////// Perform the query //////////////////////////////////////////////

    service_request( argv, opt );
    ::exit( Exit_Success );
}

//*****************************************************************************
//
// SYNOPSIS
//
        void dump_single_word( char const *word, ostream &out )
//
// DESCRIPTION
//
//      Dump the list of files a word is in and ranks therefore to standard
//      output.
//
// PARAMETERS
//
//      word    The word to have its index dumped.
//
//      out     The ostream to dump to.
//
//*****************************************************************************
{
    auto_vec<char> const lower_word( to_lower_r( word ) );
    less< char const* > const comparator;

    if ( !is_ok_word( word ) ||
        ::binary_search(
            stop_words.begin(), stop_words.end(), lower_word, comparator
        )
    ) {
        out << "# ignored: " << word << endl;
        return;
    }
    //
    // Look up the word.
    //
    word_range const range = ::equal_range(
        words.begin(), words.end(), lower_word, comparator
    );
    if ( range.first == words.end() || comparator( lower_word, *range.first ) ){
        out << "# not found: " << word << endl;
        return;
    }
    file_list const list( range.first );
    FOR_EACH( file_list, list, file ) {
        out << file->occurrences_ << ' '
            << file->rank_ << result_separator
            << index_file_info( file->index_ ) << '\n';
        if ( !out )
            return;
    }
    out << '\n';
}

//*****************************************************************************
//
// SYNOPSIS
//
        void dump_word_window(
            char const *word, int window_size, int match, ostream &out
        )
//
// DESCRIPTION
//
//      Dump a "window" of words from the index around the given word to
//      standard output.
//
// PARAMETERS
//
//      word            The word.
//
//      window_size     The number of lines the window is to contain at most.
//
//      match           The number of characters to compare.
//
//      out             The ostream to dump to.
//
//*****************************************************************************
{
    auto_vec< char > const lower_word( to_lower_r( word ) );
    less< char const* > const comparator;

    if ( !is_ok_word( word ) ||
        ::binary_search(
            stop_words.begin(), stop_words.end(), lower_word, comparator
        )
    ) {
        out << "# ignored: " << word << endl;
        return;
    }
    //
    // Look up the word.
    //
    word_range range = ::equal_range(
        words.begin(), words.end(), lower_word, comparator
    );
    if ( range.first == words.end() || comparator( lower_word, *range.first ) ){
        out << "# not found: " << word << endl;
        return;
    }

    //
    // Dump the window by first "backing up" half the window size, then going
    // forward.
    //
    int i = window_size / 2;
    while ( range.first != words.begin() && i-- > 0 )
        --range.first;
    for ( i = 0; range.first != words.end() && i < window_size; ++range.first ){
        int const cmp = ::strncmp( *range.first, lower_word, match );
        if ( cmp < 0 )
            continue;
        if ( cmp > 0 )
            break;
        out << *range.first << '\n';
        if ( !out )
            return;
        ++i;
    }
    out << '\n';
}

//*****************************************************************************
//
// SYNOPSIS
//
        bool search( char const *query,
            int skip_results, int max_results, char const *results_format,
            ostream &out, ostream &err
        )
//
// DESCRIPTION
//
//      Parse a query, perform a search, and output the results.
//
// PARAMETERS
//
//      query           The text of the query.
//
//      skip_results    The number of initial results to skip.
//
//      max_results     The maximum number of results to output.
//
//      results_format  The results format.
//
//      out             The ostream to print the results to.
//
//      err             The ostream to print errors to.
//
//*****************************************************************************
{
    token_stream    query_stream( query );
    search_results  results;
    stop_word_set   stop_words_found;

    if ( !( parse_query( query_stream, results, stop_words_found )
         && query_stream.eof()
    ) ) {
        err << error << "malformed query\n";
#ifdef  SEARCH_DAEMON
        if ( daemon_type != "none" )
            return false;
#endif
        ::exit( Exit_Malformed_Query );
    }

    ////////// Print the results //////////////////////////////////////////////

    results_formatter const *format;
    if ( to_lower( *results_format ) == 'x' /* must be "xml" */ )
        format = new xml_formatter( out, results.size() );
    else
        format = new classic_formatter( out, results.size() );

    format->pre( stop_words_found );
    if ( !out )
        return false;
    if ( skip_results < results.size() && max_results ) {
        //
        // Copy the results to a vector to sort them by rank.
        //
#ifndef __SUNPRO_CC
        typedef vector< search_result > sorted_results_type;
        sorted_results_type sorted;
        sorted.reserve( results.size() );
        ::copy( results.begin(), results.end(), ::back_inserter( sorted ) );
#else
        // Sun's CC compiler and/or their STL implementation seems pretty
        // broken so we have to do the following things the hard way.
        //
        typedef vector_adapter< search_result > sorted_results_type;
        sorted_results_type sorted( results.size() );

        int i = 0;
        TRANSFORM_EACH( search_results, results, result )
            sorted[ i++ ] = *reinterpret_cast<search_result*>( &*result );
#endif  /* __SUNPRO_CC */

        ::sort( sorted.begin(), sorted.end(), sort_by_rank() );
        //
        // Compute the highest rank and the normalization factor.
        //
        int const highest_rank = sorted[0].second;
        double const normalize = 100.0 / highest_rank;
        //
        // Print the sorted results skipping some if requested to and not
        // exceeding the maximum.
        //
        for ( sorted_results_type::const_iterator
              r  = sorted.begin() + skip_results;
              r != sorted.end() && max_results-- > 0 && out; ++r
        ) {
            // cast gets rid of warning
            int rank = static_cast<int>( r->second * normalize );
            if ( !rank )
                rank = 1;
            format->result(
                rank,
                file_info(
                    reinterpret_cast<unsigned char const*>( files[ r->first ] )
                )
            );
            if ( !out )
                return false;
        }
    }
    format->post();
    delete format;
    return true;
}

//*****************************************************************************
//
// SYNOPSIS
//
        search_options::search_options(
            int *argc, char ***argv,
            option_stream::spec const opt_spec[],
            ostream &err
        )
//
// DESCRIPTION
//
//      Construct (initialze) a search_options by parsing options from the argv
//      vector according to the given option specification.
//
// PARAMETERS
//
//      argc        A pointer to the number of arguments.  The value is
//                  decremented by the number of options and their arguments.
//
//      argv        A pointer to the argv vector.  This pointer is incremented
//                  by the number of options and their arguments.
//
//      opt_spec    The set of options to allow and their parameters.
//
//      err         The ostream to print errors to.
//
//*****************************************************************************
    : bad_( false )
{
    config_file_name_arg    = ConfigFile_Default;
    dump_entire_index_opt   = false;
    dump_match_arg          = 0;
    dump_meta_names_opt     = false;
    dump_stop_words_opt     = false;
    dump_window_size_arg    = 0;
    dump_word_index_opt     = false;
    index_file_name_arg     = 0;
    max_results_arg         = 0;
    results_format_arg      = 0;
    result_separator_arg    = 0;
    skip_results_arg        = 0;
    stem_words_opt          = false;
    word_files_max_arg      = 0;
    word_percent_max_arg    = 0;
#ifdef  FEATURE_word_pos
    words_near_arg          = 0;
#endif
#ifdef  SEARCH_DAEMON
    daemon_type_arg         = 0;
    group_arg               = 0;
#ifdef  __APPLE__
    launchd_opt             = 0;
#endif
    max_threads_arg         = 0;
    min_threads_arg         = 0;
    pid_file_name_arg       = 0;
    search_background_opt   = false;
    socket_address_arg      = 0;
    socket_file_name_arg    = 0;
    socket_queue_size_arg   = 0;
    socket_timeout_arg      = 0;
    thread_timeout_arg      = 0;
    user_arg                = 0;
#endif  /* SEARCH_DAEMON */
    option_stream opt_in( *argc, *argv, opt_spec );
    for ( option_stream::option opt; opt_in >> opt; )
        switch ( opt ) {

#ifdef  SEARCH_DAEMON
            case 'a': // TCP socket address.
                socket_address_arg = opt.arg();
                break;

            case 'b': // Run as a daemon.
                daemon_type_arg = opt.arg();
                break;

            case 'B': // Do not run daemon in the background.
                search_background_opt = true;
                break;
#endif
            case 'c': // Config. file.
                config_file_name_arg = opt.arg();
                break;

            case 'd': // Dump query word indices.
                dump_word_index_opt = true;
                break;

            case 'D': // Dump entire word index.
                dump_entire_index_opt = true;
                break;

            case 'f': // Word/files file maximum.
                word_files_max_arg = opt.arg();
                break;

            case 'F': // Results format.
                if ( results_format.is_legal( opt.arg(), err ) )
                    results_format_arg = opt.arg();
                else
                    bad_ = true;
                break;
#ifdef  SEARCH_DAEMON
            case 'G': // Group.
                group_arg = opt.arg();
                break;
#endif
            case 'i': // Index file.
                index_file_name_arg = opt.arg();
                break;
            case 'm': // Max. number of results.
                max_results_arg = opt.arg();
                break;

            case 'M': // Dump meta-name list.
                dump_meta_names_opt = true;
                break;
#ifdef  FEATURE_word_pos
            case 'n': // Words near.
                words_near_arg = ::atoi( opt.arg() );
                break;
#endif
#ifdef  SEARCH_DAEMON
            case 'o': // Socket timeout.
                socket_timeout_arg = ::atoi( opt.arg() );
                break;

            case 'O': // Thread timeout.
                thread_timeout_arg = ::atoi( opt.arg() );
                break;
#endif
            case 'p': // Word/file percentage.
                word_percent_max_arg = opt.arg();
                break;
#ifdef  SEARCH_DAEMON
            case 'P': // PID file.
                pid_file_name_arg = opt.arg();
                break;

            case 'q': // Socket queue size.
                socket_queue_size_arg = ::atoi( opt.arg() );
                if ( socket_queue_size_arg < 1 )
                    socket_queue_size_arg = 1;
                break;
#endif
            case 'r': // Number of initial results to skip.
                skip_results_arg = ::atoi( opt.arg() );
                if ( skip_results_arg < 0 )
                    skip_results_arg = 0;
                break;

            case 'R': // Result separator.
                result_separator_arg = opt.arg();
                break;

            case 's': // Stem words.
                stem_words_opt = true;
                break;

            case 'S': // Dump stop-word list.
                dump_stop_words_opt = true;
                break;
#ifdef  SEARCH_DAEMON
            case 't': // Minimum number of concurrent threads.
                min_threads_arg = ::atoi( opt.arg() );
                break;

            case 'T': // Maximum number of concurrent threads.
                max_threads_arg = ::atoi( opt.arg() );
                break;

            case 'u': // Unix domain socket file.
                socket_file_name_arg = opt.arg();
                break;

            case 'U': // User.
                user_arg = opt.arg();
                break;
#endif
            case 'V': // Display version and exit.
                err << "SWISH++ " << version << endl;
#ifdef  SEARCH_DAEMON
                if ( daemon_type == "none" )
#endif
                    ::exit( Exit_Success );
#ifdef  SEARCH_DAEMON
                bad_ = true;
                return;
#endif
            case 'w': { // Dump words around query words.
                dump_window_size_arg = ::atoi( opt.arg() );
                if ( dump_window_size_arg < 0 )
                    dump_window_size_arg = 0;
                char const *comma = ::strchr( opt.arg(), ',' );
                if ( !comma )
                    break;
                dump_match_arg = ::atoi( comma + 1 );
                if ( dump_match_arg < 0 )
                    dump_match_arg = 0;
                break;
            }
#if defined( SEARCH_DAEMON ) && defined( __APPLE__ )
            case 'X': // Cooperate with Mac OS X's launchd.
                launchd_opt = true;
                break;
#endif
            default: // Bad option.
                err << usage;
                bad_ = true;
                return;
        }

    *argc -= opt_in.shift(), *argv += opt_in.shift();
}

//*****************************************************************************
//
// SYNOPSIS
//
        bool service_request(
            char *argv[], search_options const &opt, ostream &out, ostream &err
        )
//
// DESCRIPTION
//
//      Service a request either from the command line or from a client via a
//      socket.
//
// PARAMETERS
//
//      argv    The post-option-parsed set of command line arguments, i.e., all
//              the options have been stripped.
//
//      opt     The set of options specified for this request.
//
//      out     The ostream to send results to.
//
//      err     The ostream to send errors to.
//
//*****************************************************************************
{
    /////////// Dump stuff if requested ///////////////////////////////////////

    if ( opt.dump_window_size_arg ) {
        while ( *argv && out )
            dump_word_window( *argv++,
                opt.dump_window_size_arg, opt.dump_match_arg, out
            );
        return true;
    }
    if ( opt.dump_word_index_opt ) {
        while ( *argv && out )
            dump_single_word( *argv++, out );
        return true;
    }
    if ( opt.dump_entire_index_opt ) {
        FOR_EACH( index_segment, words, word ) {
            out << *word << '\n';
            file_list const list( word );
            FOR_EACH( file_list, list, file ) {
                out << "  " << file->occurrences_ << ' '
                    << file->rank_ << result_separator
                    << index_file_info( file->index_ )
                    << '\n';
                if ( !out )
                    return false;
            }
            out << '\n';
        }
        return true;
    }
    if ( opt.dump_stop_words_opt ) {
        FOR_EACH( index_segment, stop_words, word ) {
            out << *word << '\n';
            if ( !out )
                return false;
        }
        return true;
    }
    if ( opt.dump_meta_names_opt ) {
        FOR_EACH( index_segment, meta_names, meta_name ) {
            out << *meta_name << '\n';
            if ( !out )
                return false;
        }
        return false;
    }

    ////////// Perform the query //////////////////////////////////////////////

    //
    // Paste the rest of the command line together into a single query string.
    //
    string query = *argv++;
    while ( *argv ) {
        query += ' ';
        query += *argv++;
    }
    return search( query.c_str(),
        opt.skip_results_arg,
        opt.max_results_arg ? ::atoi( opt.max_results_arg ) : max_results,
        opt.results_format_arg ? opt.results_format_arg : results_format,
        out, err
    );
}

//*****************************************************************************
//
// SYNOPSIS
//
        ostream& write_file_info( ostream &o, char const *p )
//
// DESCRIPTION
//
//      Parse a file_info from an index file and write it to an ostream.
//
// PARAMETERS
//
//      o   The ostream to write to.
//
//      p   A pointer to the first character containing a file_info inside an
//          index file.
//
// RETURN VALUE
//
//      The passed-in ostream.
//
//*****************************************************************************
{
    file_info const fi( reinterpret_cast<unsigned char const*>( p ) );
    return o
        << directories[ fi.dir_index() ] << '/' << fi.file_name()
        << result_separator << fi.size() << result_separator
        << fi.title();
}

//*****************************************************************************
//
//  Miscellaneous function(s)
//
//*****************************************************************************

ostream& usage( ostream &err ) {
    err <<  "usage: " << me << " [options] query\n"
    "options: (unambiguous abbreviations may be used for long options)\n"
    "========\n"
    "-?   | --help             : Print this help message\n"
#ifdef SEARCH_DAEMON
    "-a a | --socket-address a : Socket address [default: *:" << SocketPort_Default << "]\n"
    "-b t | --daemon-type t    : Daemon type to run as [default: none]\n"
    "-B   | --no-background    : Don't run daemon in the background [default: do]\n"
#endif
    "-c f | --config-file f    : Name of configuration file [default: " << ConfigFile_Default << "]\n"
    "-d   | --dump-words       : Dump query word indices, exit\n"
    "-D   | --dump-index       : Dump entire word index, exit\n"
    "-f n | --word-files n     : Word/file maximum [default: infinity]\n"
    "-F f | --format f         : Results format [default: classic]\n"
#ifdef SEARCH_DAEMON
    "-G s | --group s          : Daemon group to run as [default: " << Group_Default << "]\n"
#endif
    "-i f | --index-file f     : Name of index file [default: " << IndexFile_Default << "]\n"
    "-m n | --max-results n    : Maximum number of results [default: " << ResultsMax_Default << "]\n"
    "-M   | --dump-meta        : Dump meta-name index, exit\n"
#ifdef FEATURE_word_pos
    "-n n | --near n           : Maximum number of words apart [default: " << WordsNear_Default << "]\n"
#endif
#ifdef SEARCH_DAEMON
    "-o s | --socket-timeout s : Search client request timeout [default: " << SocketTimeout_Default << "]\n"
    "-O s | --thread-timeout s : Idle spare thread timeout [default: " << ThreadTimeout_Default << "]\n"
#endif
    "-p n | --word-percent n   : Word/file percentage [default: 100]\n"
#ifdef SEARCH_DAEMON
    "-P f | --pid-file f       : Name of file to record daemon PID in [default: none]\n"
    "-q n | --queue-size n     : Maximum queued socket connections [default: " << SocketQueueSize_Default << "]\n"
#endif
    "-r n | --skip-results n   : Number of initial results to skip [default: 0]\n"
    "-R s | --separator s      : Result separator string [default: \" \"]\n"
    "-s   | --stem-words       : Stem words prior to search [default: no]\n"
    "-S   | --dump-stop        : Dump stop-word index, exit\n"
#ifdef SEARCH_DAEMON
    "-t n | --min-threads n    : Minimum number of threads [default: " << ThreadsMin_Default << "] \n"
    "-T n | --max-threads n    : Maximum number of threads [default: " << ThreadsMax_Default << "] \n"
    "-u f | --socket-file f    : Name of socket file [default: " << SocketFile_Default << "]\n"
    "-U s | --user s           : Daemon user to run as [default: " << User_Default << "]\n"
#endif
    "-V   | --version          : Print version number, exit\n"
    "-w n[,m] | --window n[,m] : Dump window of words around query words [default: 0]\n"
#if defined( SEARCH_DAEMON ) && defined( __APPLE__ )
    "-X   | --launchd          : If a daemon, cooperate with Mac OS X's launchd\n";
#endif
    return err;
}
/* vim:set et sw=4 ts=4: */