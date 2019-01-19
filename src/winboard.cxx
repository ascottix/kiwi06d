/*
    Kiwi
    Copyright (c) 1999-2004,2005 Alessandro Scotti
    http://www.ascotti.org/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "log.h"
#include "san.h"
#include "score.h"
#include "string_tokenizer.h"
#include "winboard.h"

typedef bool (* CommandHandler) (StringTokenizer & args, Command & command);

struct CommandInfo
{
    const char * name;
    int code;
    CommandHandler handler;
};

bool handleInteger( StringTokenizer & args, Command & command )
{
    bool result = false;

    String arg;

    if( args.getNextToken( arg ) ) {
        const char * buf = arg.cstr();

        if( isdigit( *buf ) ) {
            int number = 0;

            while( isdigit(*buf) ) {
                number = number * 10 + (*buf - '0');
                buf++;
            }

            if( *buf == '\0' ) {
                // Number parsed ok
                command.addIntParam( number );
                result = true;
            }
        }
    }

    return result;
}

bool handleString( StringTokenizer & args, Command & command )
{
    bool result = false;

    String arg;

    if( args.getNextToken( arg ) ) {
        // For a string, any parameter is ok
        command.addStrParam( arg );
        result = true;
    }

    return result;
}

bool handleKiwiBookAdd( StringTokenizer & args, Command & command )
{
    bool result = true;

    result &= handleString( args, command );
    result &= handleInteger( args, command );
    result &= handleInteger( args, command );

    return result;
}

bool handleKiwiBookSave( StringTokenizer & args, Command & command )
{
    bool result = true;

    result &= handleString( args, command );
    result &= handleInteger( args, command );

    return result;
}

bool handleKiwiRunSuite( StringTokenizer & args, Command & command )
{
    bool result = true;

    result &= handleString( args, command );    // Suite filename
    result &= handleInteger( args, command );   // Seconds per move

    if( args.hasMoreTokens() ) {
        result &= handleInteger( args, command );   // Optional: max search depth
    }

    return result;
}

bool handleKiwiSetOption( StringTokenizer & args, Command & command )
{
    bool result = true;

    result &= handleString( args, command );
    result &= handleString( args, command );

    return result;
}

bool handleXBoardLevel( StringTokenizer & args, Command & command )
{
    bool result = handleInteger( args, command );

    if( result && args.hasMoreTokens() ) {
        int periodLength = 0;
        bool periodInSeconds = false;
        String time;

        args.getNextToken( time );

        const char * buf = time.cstr();

        result = isdigit(*buf) ? true : false; // Must start with a digit

        while( result && (*buf != '\0') ) {
            switch( *buf ) {
            case ':':
                // We parsed minutes so far, convert them to seconds
                periodLength *= 60;
                periodInSeconds = true;
                result = isdigit(buf[1]) ? true : false; // Next char must be a digit
                break;
            default:
                if( isdigit(*buf) ) {
                    periodLength = 10*periodLength + (*buf - '0');
                }
                else {
                    result = false; // Unexpected character
                }
            }

            buf++;
        }

        if( ! periodInSeconds ) {
            periodLength *= 60;
        }

        command.addIntParam( periodLength );
    }

    result &= handleInteger( args, command );

    return result;
}

bool handleXBoardProtover( StringTokenizer & args, Command & command )
{
    bool result = handleInteger( args, command );

    if( result ) {
        int protocolVersion = command.intParam(0);

        if( protocolVersion >= 2 ) {
            printf( "feature myname=\"%s\"\n", Engine::myName );
            printf( "feature colors=0 ping=1 playother=1 setboard=1 sigint=0 sigterm=0 usermove=1\n" );
            printf( "feature done=1\n" );
        }
    }

    return result;
}

bool handleXBoardSetBoard( StringTokenizer & args, Command & command )
{
    String fen;

    bool result = args.getRemainingTokens( fen );

    if( result ) {
        command.addStrParam( fen );
    }

    return result;
}

bool handleXBoardResult( StringTokenizer & args, Command & command )
{
    bool result = handleString( args, command );

    if( result ) {
        const char * r = command.strParam(0);

        if( strcmp( r, "1/2-1/2" ) == 0 )
            command.addIntParam( res_Draw );
        else if( strcmp( r, "1-0" ) == 0 )
            command.addIntParam( res_WhiteWins );
        else if( strcmp( r, "0-1" ) == 0 )
            command.addIntParam( res_BlackWins );
        else if( strcmp( r, "*" ) == 0 )
            command.addIntParam( res_Unknown );
        else {
            Log::write( "*** Error: invalid result string '%s'\n", r );
            result = false;
        }
    }

    return result;
}

CommandInfo commandInfo[] = {
    ".",            cmd_DisplaySearchStatus,    0,
    "?",            cmd_MoveNow,                0,
    "accepted",     cmd_Null,                   0,  // Ignore
    "an",           cmd_KiwiAnalyze,            handleXBoardSetBoard,
    "analyze",      cmd_EnterAnalyzeMode,       0,
    "bestm",        cmd_KiwiBestMove,           0,
    "bk",           cmd_ShowBook,               0,
    "bookadd",      cmd_KiwiAddToBookTree,      handleKiwiBookAdd,
    "bookload",     cmd_KiwiLoadBook,           handleString,
    "booksave",     cmd_KiwiExportBookTree,     handleKiwiBookSave,
    "computer",     cmd_SetOpponentIsComputer,  0,
    "draw",         cmd_OpponentOffersDraw,     0,
    "easy",         cmd_SetPonderingOff,        0,
    "evalt",        cmd_KiwiEvaluateSuite,      handleString,
    "exit",         cmd_LeaveAnalyzeMode,       0,
    "force",        cmd_Force,                  0,
    "genbb",        cmd_KiwiGenBB,              0,
    "go",           cmd_Go,                     0,
    "hard",         cmd_SetPonderingOn,         0,
    "help",         cmd_KiwiHelp,               0,
    "hint",         cmd_ShowHint,               0,
    "level",        cmd_SetLevel,               handleXBoardLevel,
    "new",          cmd_New,                    0,
    "nopost",       cmd_HideThinking,           0,
    "otim",         cmd_SetOpponentClock,       handleInteger,
    "perft",        cmd_KiwiPerft,              handleInteger,
    "ping",         cmd_Ping,                   handleInteger,
    "playother",    cmd_GoPlayOther,            0,
    "post",         cmd_ShowThinking,           0,
    "protover",     cmd_Null,                   handleXBoardProtover,
    "quit",         cmd_Quit,                   0,
    "random",       cmd_Null,                   0,  // Ignore
    "rejected",     cmd_Null,                   0,
    "remove",       cmd_UndoLastFullMove,       0,
    "result",       cmd_Result,                 handleXBoardResult,
    "sd",           cmd_SetFixedDepth,          handleInteger,
    "set",          cmd_KiwiSetOption,          handleKiwiSetOption,
    "setboard",     cmd_SetBoard,               handleXBoardSetBoard,
    "st",           cmd_SetFixedTime,           handleInteger,
    "suite",        cmd_KiwiRunSuite,           handleKiwiRunSuite,
    "test",         cmd_KiwiTest,               0,
    "time",         cmd_SetClock,               handleInteger,
    "undo",         cmd_UndoLastHalfMove,       0,
    "usermove",     cmd_OpponentMoves,          handleString,
    "xboard",       cmd_Null,                   0,
    0, 0, 0
};

WinBoardAdapter::WinBoardAdapter()
{
    // Disable buffering on output
    setbuf( stdout, NULL );
}

WinBoardAdapter::~WinBoardAdapter()
{
}

bool WinBoardAdapter::getNextCommand( Command & command )
{
    bool result = false;

    const char * s = System::readInputLine();

    if( s != 0 ) {
        parseCommand( s, command );

        // Let caller know this is a valid command
        if( command.code() != cmd_Null ) {
            result = true;
        }

        System::acknowledgeInput();
    }

    return result;
}

int WinBoardAdapter::parseCommand( const char * line, Command & command )
{
    LOG(( "Received command: %s\n", line ));

    // Runtime initialization: number of entries in the command table
    static int commandCount = 0;

    StringTokenizer commandLine( line );

    int result = -1;

    command.clear();

    // Initialize if needed
    if( commandCount <= 0 ) {
        CommandInfo * last = 0;
        CommandInfo * curr = commandInfo;

        while( curr->name != 0 ) {
            // Make sure items are sorted (safety net)
            if( (last != 0) && (strcmp(last->name, curr->name) >= 0) ) {
                Log::write( "*** Fatal: command set not sorted: '%s'\n", curr->name );
                exit(1);
            }

            // Update count
            commandCount++;

            // Go to next item
            last = curr;
            curr++;
        }
    }

    // Get the command name
    String commandName;

    commandLine.getNextToken( commandName );

    if( ! commandName.isEmpty() ) {
        const char * name = commandName.cstr();

        // Perform a binary search: using the first character first, then the whole string
        int lo = 0;
        int hi = commandCount - 1;

        while( lo <= hi ) {
            int index = (lo + hi) / 2;

            int diff = name[0] - commandInfo[index].name[0];

            if( diff == 0 ) {
                diff = strcmp( name, commandInfo[index].name );
            }

            if( diff == 0 ) {
                // Command found, handle it
                if( commandInfo[index].handler != 0 ) {
                    if( commandInfo[index].handler( commandLine, command ) ) {
                        result = 0;
                    }
                }
                else {
                    result = 0;
                }

                if( result == 0 ) {
                    command.setCode( commandInfo[index].code );
                }

                break;
            }
            else if( diff < 0 ) {
                hi = index - 1;
            }
            else {
                lo = index + 1;
            }
        }

        if( lo > hi ) {
            // Command not found, default to "usermove"
            command.setCode( cmd_OpponentMoves );
            command.addStrParam( commandName );
            result = 0;
        }
    }

    if( result != 0 ) {
        command.clear();
    }

    return result;
}

void WinBoardAdapter::rejectMove( const char * move, const char * why )
{
    const char * what = (move != 0) ? move : "null";

    if( why != 0 ) {
        printf( "Illegal move (%s): %s\n", why, what );
    }
    else {
        printf( "Illegal move: %s\n", what );
    }
}

void WinBoardAdapter::sendError( const char * what, const char * why )
{
    if( what != 0 ) {
        if( why != 0 ) {
            printf( "Error (%s): %s\n", why, what );
        }
        else {
            printf( "Error: %s\n", what );
        }
    }
}

void WinBoardAdapter::sendResult( int result, const char * why )
{
    const char * what = "1/2-1/2";

    if( result < 0 ) {
        what = "0-1";
    }
    else if( result > 0 ) {
        what = "1-0";
    }

    printf( "%s {%s}\n", what, why );
}

void WinBoardAdapter::resignGame( const Position & pos, const char * why )
{
    if( why == 0 ) {
        printf( "resign\n" );
    }
    else {
        // Use the result command to display the engine message
        const char * text = pos.sideToPlay == White ?
            "0-1 {white resigns: %s}\n" :
            "1-0 {black resigns: %s}\n";

        printf( text, why );
    }
}

void WinBoardAdapter::sendHint( const Position & pos, Move move )
{
    char what[32];

    move.toString( what );

    printf( "Hint: %s\n", what );
}

void WinBoardAdapter::playMove( const Position & pos, Move move )
{
    char what[32];

    SAN::moveToTextLong( what, pos, move );

    printf( "move %s\n", what );
}

void WinBoardAdapter::showThinking( const Position & pos, const MoveInfo & move )
{
    char buffer[512];

    sprintf( buffer, "%d %d %u %u", move.depth, move.score, move.time / 10, move.nodes );

    char * pv = buffer + strlen( buffer );

    // Dump principal variation
    Position    p( pos );

    for( int i=0; i<move.pvlen; i++ ) {
        Move m = move.pv[i];

        // Convert the move in SAN text and append it to the buffer
        char text[10];

        text[0] = ' ';

        if( SAN::moveToText( text+1, p, m ) != 0 ) {
            // Shouldn't happen!
            Log::write( "showThinking: failed to convert PV move!\n" );

            p.dump();

            m.dump();

            Log::write( "\n" );
            break;
        }

        strcat( pv, text );

        // Play the move and update the position
        p.doMove( m );
    }
        
    printf( "%s\n", buffer );

    // Write in the log file too
    char score[16];

    sprintf( score, "%+.2f", (double) move.score / 100.0 );

    if( move.score > Score::MateHi ) {
        int n = 1 + (Score::Mate - move.score) / 2;

        if( n > 1 ) {
            sprintf( score, "M%d", n );
        }
    }

    Log::write( "%2d/%2d %6s  %02u:%02u %9u %s\n", 
        move.depth, 
        move.maxdepth,
        score,
        move.time / 60000, // Minutes
        (move.time % 60000) / 1000, // Seconds
        move.nodes,
        pv );
}

void WinBoardAdapter::showCurrentSearchMove( const Position & pos, const RootMoveStat & info )
{
    printf( "stat01: %u %u %d %d %d %s\n",
        info.time / 10,
        info.nodes,
        info.depth,
        info.moves_remaining,
        info.moves_total,
        SAN::moveToText( pos, info.current_move ) );
}

void WinBoardAdapter::answerToPing( const Command & ping )
{
    printf( "pong %d\n", ping.intParam(0) );
}
