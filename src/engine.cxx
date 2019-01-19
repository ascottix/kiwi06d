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
#include <stdlib.h>
#include <string.h>

#include "engine.h"

#include "attacks.h"
#include "bitbase.h"
#include "board.h"
#include "log.h"
#include "mask.h"
#include "pgn.h"
#include "pgn_lex.h"
#include "recognizer.h"
#include "undoinfo.h"
#include "san.h"
#include "score.h"
#include "zobrist.h"

void testRecognizers(); // Defined in recognizer_test

#ifndef EOF_AS_INPUT
#error Alessandro, remember to define EOF_AS_INPUT!
#endif

// Name
const char * Engine::myName = "Kiwi 0.6d";
const char * Engine::myAuthor = "Alessandro Scotti";

const char * nameOfOpeningBook          = "kiwi.obk";
const char * nameOfConfigurationFile    = "kiwi.ini";

// Search parameters
int Engine::maxSearchDepthFactor    = 3;

// Null move parameters
int Engine::nullMoveMinDepth        = FullPlyDepth * 2;     // No null move if depth below this
int Engine::nullMoveMinReduction    = FullPlyDepth * 2;     // Min depth reduction
int Engine::nullMoveMaxReduction    = FullPlyDepth * 4;     // Max depth reduction

// Search extension parameters
int Engine::initialExtensionBonus   = 0;
int Engine::maxExtensionPerPly      = FullPlyDepth;
int Engine::extendCheck             = FullPlyDepth;
int Engine::extendPawnOn7th         = FullPlyDepth;
int Engine::extendThreat            = FullPlyDepth * 2 / 4;
int Engine::extendRecapture         = FullPlyDepth * 2 / 4;
int Engine::extendSingleReply       = FullPlyDepth;
int Engine::extendPawnEndgame       = 2*FullPlyDepth; // From 0.6a... not tested much

// Pruning parameters
int Engine::pruneMarginAtFrontier       = 200;
int Engine::pruneMarginAtPreFrontier    = 500;

int Engine::sizeOfHashTable             = 64 * 1024 * 1024; // Number of entries (must be a power of two)
int Engine::sizeOfPawnHashTable         =  2 * 1024 * 1024; // Number of entries

int Engine::scoreMarginAt1stCheck   =   0;  // At  50% time, score margin can be negative here!
int Engine::scoreMarginAt2ndCheck   =  25;  // At 100% time
int Engine::scoreMarginAt3rdCheck   = 100;  // At 200% time
int Engine::safetyTimePerMove       = 250;  // In milliseconds

// Resign threshold: if the threshold is an "impossibly low" value,
// say for example -50000, then the engine will never resign
int Engine::resignThreshold         = -700;

//
HashTable *     Engine::hashTable       = 0;
PawnHashTable * Engine::pawnHashTable   = 0;

unsigned    Engine::fixedSearchDepth;
int         Engine::searchMode;
bool        Engine::ponderingEnabled;

int         Engine::timeLeft;
int         Engine::opponentTimeLeft;
int         Engine::movesPerInterval;
int         Engine::sizeOfTimeInterval;     // In milliseconds
int         Engine::sizeOfTimeIncrement;    // In milliseconds
int         Engine::movesUntilTimeCheck;

int         Engine::state;

Engine::Rep3Info    Engine::rep3History[ MaxMovesPerGame + MaxSearchPly ];
MoveInfo            Engine::moveHistory[ MaxMovesPerGame ];
Position            Engine::gameHistory[ MaxMovesPerGame + MaxSearchPly ];
int                 Engine::gameHistoryIdx;
Position            Engine::gamePosition;
MoveInfo            Engine::gameMoveToPlay;

TimeManager Engine::timeManager;
unsigned    Engine::timeOnClock;
unsigned    Engine::timeOnOpponentClock;
unsigned    Engine::timeDefaultTargetForMove;
unsigned    Engine::timeCurrentTarget;
unsigned    Engine::timeCurrentStage;

bool        Engine::inputCheckWhileSearching = true;

Move            Engine::ponderMove;

RootMoveStat    Engine::rootMoveStat;

bool            Engine::opponentIsComputer = false;

volatile bool   Engine::searchMustBeInterrupted;
unsigned        Engine::searchStartTime;
Adapter *       Engine::interfaceAdapter = 0;
bool            Engine::showThinking;
unsigned        Engine::showThinkingLastUpdate = 0;

Book *          Engine::openingBook;
int             Engine::numOfMovesNotInBook;
BookTree        Engine::bookTree;

typedef bool (* OptionHandler) ( const char * name, const char * value, void * extra );

struct OptionInfo
{
    const char * name;      // Option name
    OptionHandler handler;  // Option handler routine
    void * extra;           // Extra parameter for handler routine (may be null)
};

const char *    HashSizeOption      = "ttable.size";
const char *    PawnHashSizeOption  = "pawntable.size";

static bool handleIntegerOption( const char * name, const char * value, void * extra )
{
    bool result = false;

    int * variableAddress = (int *) extra;  // Extra parameter is address of destination variable

    bool negate = false;

    while( *value == '-' || *value == '+' ) {
        negate ^= (*value == '-');
        value++;
    }

    if( isdigit(*value) ) {
        int n = 0;

        while( isdigit(*value) ) {
            n = n*10 + *value - '0';
            value++;
        }

        if( *value == '\0' ) {
            *variableAddress = negate ? -n : +n;

            result = true;
        }
    }
    else {
        printf( "*** Error: cannot parse number\n" );
    }

    return result;
}

static bool handleSizeInMegabytes( const char * name, const char * value, void * extra )
{
    bool result = false;

    if( isdigit(*value) ) {
        int n = 0;

        while( isdigit(*value) ) {
            n = n*10 + *value - '0';
            value++;
        }

        if( *value == 'm' || *value == 'M' ) {
            // Value is specified in megabytes
            n *= 1024*1024;
            value++;
        }

        if( *value == '\0' ) {
            if( (n & (n-1)) != 0 ) {
                printf( "*** Error: size must be a power of two\n" );
            }
            else {
                if( n < 1024*1024 ) {
                    printf( "*** Warning: minimum size is 1M\n" );
                    n = 1024*1024;
                }

                if( strcmp(name,HashSizeOption) == 0 ) {
                    Engine::sizeOfHashTable = n;
                }
                else if( strcmp(name,PawnHashSizeOption) == 0 ) {
                    Engine::sizeOfPawnHashTable = n;
                }

                result = true;
            }
        }
    }
    else {
        printf( "*** Error: cannot parse number\n" );
    }

    return result;
}

OptionInfo optionInfo[] = {
    "score.pawn",           handleIntegerOption,    &Score::Pawn,
    "score.knight",         handleIntegerOption,    &Score::Knight,
    "score.bishop",         handleIntegerOption,    &Score::Bishop,
    "score.rook",           handleIntegerOption,    &Score::Rook,
    "score.queen",          handleIntegerOption,    &Score::Queen,

    HashSizeOption,         handleSizeInMegabytes,  0,
    PawnHashSizeOption,     handleSizeInMegabytes,  0,

    "search.maxfactor",     handleIntegerOption,    &Engine::maxSearchDepthFactor,

    "prune.frontier",       handleIntegerOption,    &Engine::pruneMarginAtFrontier,
    "prune.pre-frontier",   handleIntegerOption,    &Engine::pruneMarginAtPreFrontier,

    "nullmove.mindepth",    handleIntegerOption,    &Engine::nullMoveMinDepth,
    "nullmove.rmin",        handleIntegerOption,    &Engine::nullMoveMinReduction,
    "nullmove.rmax",        handleIntegerOption,    &Engine::nullMoveMaxReduction,

    "extend.init",          handleIntegerOption,    &Engine::initialExtensionBonus,
    "extend.plymax",        handleIntegerOption,    &Engine::maxExtensionPerPly,
    "extend.check",         handleIntegerOption,    &Engine::extendCheck,
    "extend.pawn7th",       handleIntegerOption,    &Engine::extendPawnOn7th,
    "extend.threat",        handleIntegerOption,    &Engine::extendThreat,
    "extend.recapture",     handleIntegerOption,    &Engine::extendRecapture,
    "extend.singlereply",   handleIntegerOption,    &Engine::extendSingleReply,
    "extend.pawnendgame",   handleIntegerOption,    &Engine::extendPawnEndgame,

    "resign.threshold",     handleIntegerOption,    &Engine::resignThreshold,

    "time.score.control0",  handleIntegerOption,    &Engine::scoreMarginAt1stCheck,
    "time.score.control1",  handleIntegerOption,    &Engine::scoreMarginAt2ndCheck,
    "time.score.control2",  handleIntegerOption,    &Engine::scoreMarginAt3rdCheck,
    "time.safety",          handleIntegerOption,    &Engine::safetyTimePerMove,

    0, 0, 0,
};

int Engine::handleSetOptionCommand( const char * key, const char * value )
{
    int result = -1;

    if( state == state_Observing ) {
        OptionInfo * option = optionInfo;

        while( option->name != 0 ) {
            if( strcmp( option->name, key ) == 0 ) {
                if( option->handler( key, value, option->extra ) ) {
                    result = 0;
                }
                break;
            }

            option++;
        }
    }
    else {
        printf( "*** Error: set option command received in state: %d\n", state );
    }

    // Reinitialize all the stuff that was cached and that may depend on
    // the parameter that was modified
    if( result == 0 ) {
        Score::initialize();

        if( sizeOfHashTable != (int) hashTable->getSize() ) {
            delete hashTable;
            hashTable = new HashTable( sizeOfHashTable / sizeof(HashTable::Entry) );
        }

        if( sizeOfPawnHashTable != (int) pawnHashTable->getSize() ) {
            delete pawnHashTable;
            pawnHashTable = new PawnHashTable( sizeOfPawnHashTable / sizeof(HashTable::Entry) );
        }
    }

    return result;
}

void Engine::initialize()
{
    Log::write( "%s starting...\n\n", myName );

    // Now load the configuration file (if any)
    FILE * f = fopen( nameOfConfigurationFile, "r" );
    int lineNumber = 0;
    
    if( f != 0 ) {
        Log::write( "Configuration file found, loading options...\n" );

        char line[512];

        while( 1 ) {
            if( fgets( line, sizeof(line), f ) == 0 ) {
                break;
            }

            lineNumber++;

            line[ sizeof(line) - 1 ] = '\0';

            // Strip leading blanks
            char * str = line;

            while( (*str != '\0') && isspace(*str) ) {
                str++;
            }

            // Skip if comment or blank
            if( str[0] == ';' || str[0] == '\0' ) {
                continue;
            }

            // Get key/value separator
            char * sep = strchr( str, '=' );

            if( sep == 0 ) {
                Log::write( "ini: no separator found at line %d\n", lineNumber );
                continue;
            }

            *sep = 0;

            // Get key and value
            String  key( str );
            String  val( sep+1 );

            key.trim();
            val.trim();

            // Search and set option
            OptionInfo * option = optionInfo;

            while( option->name != 0 ) {
                if( key == option->name ) {
                    if( ! option->handler( option->name, val.cstr(), option->extra ) ) {
                        Log::write( "ini: error setting option %s\n", option->name );
                    }
                    break;
                }

                option++;
            }

            if( option->name == 0 ) {
                Log::write( "ini: unknown option %s (line %d)\n", key.cstr(), lineNumber );
                continue;
            }
        }

        fclose( f );
    }

    // Initialize bitboards and other stuff
    Attacks::initialize();
	Board::initialize();
    Mask::initialize();
    Score::initialize();
    Zobrist::initialize();
    Recognizer::initialize();

    // Dump some configuration data into the configuration file
    LOG(( "maxSearchDepthFactor   = %d\n", maxSearchDepthFactor ));
    LOG(( "nullMoveMinDepth       = %d\n", nullMoveMinDepth ));
    LOG(( "nullMoveMinReduction   = %d\n", nullMoveMinReduction ));
    LOG(( "nullMoveMaxReduction   = %d\n", nullMoveMaxReduction ));
    LOG(( "extendInit             = %d\n", initialExtensionBonus ));
    LOG(( "extendPlyMax           = %d\n", maxExtensionPerPly ));
    LOG(( "extendCheck            = %d\n", extendCheck ));
    LOG(( "extendPawnOn7th        = %d\n", extendPawnOn7th ));
    LOG(( "extendThreat           = %d\n", extendThreat ));
    LOG(( "extendRecapture        = %d\n", extendRecapture ));
    LOG(( "extendSingleReply      = %d\n", extendSingleReply ));
    LOG(( "pruneAtFrontier        = %d\n", pruneMarginAtFrontier ));
    LOG(( "pruneAtPreFrontier     = %d\n", pruneMarginAtPreFrontier ));
    LOG(( "safetyTimePerMove      = %d\n", safetyTimePerMove ));
    LOG(( "sizeOfHashTable        = %dM\n", sizeOfHashTable / (1024*1024) ));
    LOG(( "sizeOfPawnHashTable    = %dM\n", sizeOfPawnHashTable / (1024*1024) ));
    LOG(( "\n" ));

    // Initialize hash tables
    hashTable = new HashTable( sizeOfHashTable / sizeof(HashTable::Entry) );
    pawnHashTable = new PawnHashTable( sizeOfPawnHashTable / sizeof(HashTable::Entry) );

    // Load opening book
    openingBook = new Book;
    openingBook->loadFromFile( nameOfOpeningBook );

    // Reset game history
    resetBoard( 0 );

    // Initialize variables
    showThinking = true;
    ponderingEnabled = false;
    searchMode = mode_FixedTime;
    timeCurrentTarget = 3*1000;     // 3 seconds
    state = state_Observing;

    srand( System::getTickCount() );
}

int Engine::updatePrincipalVariation( const Position & position, Move * pv, int pvofs, int pvlen  )
{
    HashTable::Entry *  entry;
    Position            pos( position );

    // Play moves already in the PV
    while( pvofs > 0 ) {
        if( pos.doMove( *pv++ ) != 0 ) {
            // Error playing a PV move, exit now!
            return 0;
        }

        pvofs--;
    }

    int result = 0;

    // Try to get the rest of the PV from the hash table
    while( pvlen > 0 ) {
        entry = hashTable->probe(pos);

        if( entry == 0 )
            break;

        Move m = entry->getMove();

        if( m == Move::Null )
            break;

        if( ! pos.isValidMove( m ) )
            break;
    
        if( pos.doMove( m ) != 0 )
            break;

        *pv++ = m;

        pvlen--;

        result++;
    }

    return result;
}

int Engine::getFullMovesPlayedFor( int side )
{
    int result = gameHistoryIdx / 2;

    // If the number of half moves played is odd and this is the
    // side that played first, add one more move
    if( (gameHistoryIdx & 1) && (side == gameHistory[0].sideToPlay) ) {
        result++;
    }

    return result;
}

void Engine::updateThinkingDisplay()
{
    showThinkingLastUpdate = timeSpentInSearch();

    interfaceAdapter->showThinking( gamePosition, gameMoveToPlay );
}

void Engine::setMoveToPlay( Move move, int score, int depth, int maxdepth, int nodes )
{
    unsigned searchTime = timeSpentInSearch();

    bool show = showThinking &&
        ( 
            (gameMoveToPlay.pv[0] != move)  ||
            (gameMoveToPlay.depth != depth) ||
            ((searchTime - showThinkingLastUpdate) >= 3000)
        );

    gameMoveToPlay.score = score;
    gameMoveToPlay.depth = depth;
    gameMoveToPlay.maxdepth = maxdepth;
    gameMoveToPlay.nodes = nodes;
    gameMoveToPlay.time = searchTime;
    gameMoveToPlay.pv[0] = move;
    gameMoveToPlay.pvlen = 1 + updatePrincipalVariation( gamePosition, gameMoveToPlay.pv, 1, MoveInfo::MaxMovesInPV-1 );

    // Show thinking if enabled and needed
    if( show ) {
        updateThinkingDisplay();
    }
}

void Engine::setTimeTargetForMove()
{
    if( searchMode == mode_TimeControl && state == state_Thinking ) {
        int movesToGo = timeManager.movesLeftInControl();

        // If sudden death, assume a default value
        if( movesToGo <= 0 ) {
            movesToGo = 30;
        }

        int increment = timeManager.currControl().increment * 1000;

        Log::write( "Time control: %d time left to play %d moves\n", timeOnClock, movesToGo );

        timeDefaultTargetForMove = (timeOnClock - 1000 + increment*(movesToGo-1)) / movesToGo;

        if( timeDefaultTargetForMove < 0 || timeDefaultTargetForMove >= timeOnClock ) {
            Log::write( "*** Time control: bad time %d, resetting!\n", timeDefaultTargetForMove );
            timeDefaultTargetForMove = 0;
        }

        Log::write( "Time control: target for this move is %d\n", timeDefaultTargetForMove );

        timeCurrentTarget = timeDefaultTargetForMove / 2;
        timeCurrentStage = 0;
    }
}

unsigned Engine::timeSpentInSearch()
{
    return System::getTickCount() - searchStartTime;
}

bool Engine::isTimeOut()
{
    bool result = false;

    if( state == state_Thinking ) {
        unsigned timeSpent = timeSpentInSearch();

        if( searchMode == mode_FixedTime ) {
            // Check against fixed time, including a small safety margin
            result = timeSpent >= (timeCurrentTarget - safetyTimePerMove);
        }
        else if( searchMode == mode_TimeControl ) {
            while( timeSpent >= timeCurrentTarget && ! result ) {
                // Exceeding current target, check the current situation
                int cur_score = gameMoveToPlay.score;
                int old_score = gameHistoryIdx >= 1 ? moveHistory[ gameHistoryIdx-1 ].score : 0;

                if( timeCurrentStage == 0 ) {
                    // Exit now if the move is more or less forced (e.g. a recapture that restores material)
                    if( gameHistoryIdx >= 2 ) {
                        if( gameMoveToPlay.pv[0].isCapture() &&
                            moveHistory[ gameHistoryIdx ].pv[0].isCapture() &&
                            (cur_score >= 0) &&
                            (cur_score >= (old_score - scoreMarginAt1stCheck)) )
                        {
                            Log::write( "Time control: early exit\n" );
                            result = true;
                        }
                    }

                    timeCurrentStage++;
                    timeCurrentTarget = timeDefaultTargetForMove;
                }
                else if( timeCurrentStage == 1 ) {
                    if( cur_score >= (old_score - scoreMarginAt2ndCheck) ) {
                        result = true;
                    }

                    timeCurrentStage++;
                    timeCurrentTarget = timeDefaultTargetForMove * 2;
                }
                else if( timeCurrentStage == 2 ) {
                    if( cur_score >= (old_score - scoreMarginAt3rdCheck) ) {
                        result = true;
                    }

                    timeCurrentStage++;
                    timeCurrentTarget = timeDefaultTargetForMove * 4;
                }
                else {
                    result = true;
                }

                // Force move if next target is unreachable
                // (note: include a safety margin here!)
                if( (timeCurrentTarget + 1000) >= timeOnClock ) {
                    result = true;
                }
            }

            // If a timeout occurred, reset the time target so we are sure that
            // all subsequent calls to this function will return timeout as well
            if( result ) {
                timeCurrentTarget = 0;
            }
        }
    }

    return result;
}

bool Engine::hasPonderMoveOnBoard()
{
    return ((state == state_PonderComplete) || (state == state_Pondering)) && (ponderMove != Move::Null);
}

#ifdef HAVE_EGTB
int EGTBProbe( const Position &, int * );
#endif

static void getBestMove( const Position & pos )
{
    Position p( pos );
    MoveList ml;
    UndoInfo ui( p );

    Log::assign(0);

    pos.dump();

    p.generateValidMoves( ml );

    printf( "\nStatic eval = %d, %d moves:\n", p.getEvaluation(), ml.count() );

    int bestEgtbScore = Score::Max;
    MoveList bestEgtb;
    int bestRecScore = Score::Max;
    MoveList bestRec;

    for( int i=0; i<ml.count(); i++ ) {
        printf( "    %-7s = ", SAN::moveToText( p, ml.move[i] ) );

        p.doMove( ml.move[i] );

        printf( "%7d", p.getEvaluation() );

#ifdef HAVE_EGTB
        int egtbScore;

        if( EGTBProbe( p, &egtbScore ) ) {
            if( egtbScore < bestEgtbScore ) {
                bestEgtbScore = egtbScore;
                bestEgtb.reset();
            }
            
            if( egtbScore <= bestEgtbScore ) {
                bestEgtb.add( ml.move[i] );
            }

            printf( ", EGTB=%7d", egtbScore );
        }
#endif

        RecognizerInfo recScore;

        if( Recognizer::probe( p, recScore ) ) {
            // Note: this is not entirely correct because it ignores bound
            // information, however I automatically compensate for it when testing! :-)
            if( recScore.value() < bestRecScore ) {
                bestRecScore = recScore.value();
                bestRec.reset();
            }

            if( recScore.value() <= bestRecScore ) {
                bestRec.add( ml.move[i] );
            }

            printf( ", recog=%7d", recScore.value() );
        }

        p.undoMove( ml.move[i], ui );

        printf( "\n" );
    }

    if( bestEgtb.count() > 0 ) {
        printf( "Best for EGTB: " );
        for( int i=0; i<bestEgtb.count(); i++ ) {
            if( i > 0 ) printf( ", " );
            printf( "%s", SAN::moveToText( p, bestEgtb.move[i] ) );
        }
        printf( "\n" );
    }

    if( bestRec.count() > 0 ) {
        printf( "Best for recognizer: " );
        for( int i=0; i<bestRec.count(); i++ ) {
            if( i > 0 ) printf( ", " );
            printf( "%s", SAN::moveToText( p, bestRec.move[i] ) );
        }
        printf( "\n" );
    }
}

int Engine::handleInput()
{
    if( System::isInputStreamClosed() ) {
        searchMustBeInterrupted = true;
        return 1;
    }

    bool yield = false; // If true we exit from the command loop (and back to the main loop, where supposedly there is work to do)
    Command command;

    while( ! yield && interfaceAdapter->getNextCommand( command ) ) {
        if( command.code() != cmd_Null ) {
            switch( command.code() ) {
            // Display search status
            case cmd_DisplaySearchStatus:
                if( state == state_Analyzing || state == state_Pondering || state == state_Thinking ) {
                    rootMoveStat.time = timeSpentInSearch();
                    rootMoveStat.nodes = Counters::posSearched;
                    interfaceAdapter->showCurrentSearchMove( gamePosition, rootMoveStat );
                }
                break;
            // Enter analysis mode
            case cmd_EnterAnalyzeMode:
                state = state_Analyzing;
                break;
            // Enter force (observing) mode
            case cmd_Force:
                state = state_Observing;
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Go
            case cmd_Go:
                timeManager.skipMultipleMoves( getFullMovesPlayedFor( gamePosition.sideToPlay ) );
                state = state_Thinking;
                break;
            // Play other
            case cmd_GoPlayOther:
                timeManager.skipMultipleMoves( getFullMovesPlayedFor( OppositeSide(gamePosition.sideToPlay) ) );
                state = state_Pondering;
                break;
            // Hide thinking
            case cmd_HideThinking:
                showThinking = false;
                break;
            // Leave analysus mode
            case cmd_LeaveAnalyzeMode:
                state = state_Observing;
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Move now
            case cmd_MoveNow:
                if( state == state_Thinking ) {
                    searchMustBeInterrupted = true;
                }
                break;
            // Prepare for new game
            case cmd_New:
                resetBoard( 0 );
                if( state != state_Analyzing && state != state_AnalysisComplete ) {
                    state = state_Waiting;
                }
                else {
                    state = state_Analyzing;
                }
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Opponent moves
            case cmd_OpponentMoves:
                handleOpponentMove( command.strParam(0) );
                break;
            // Opponent is offering a draw
            case cmd_OpponentOffersDraw:
                break;
            // Ping
            case cmd_Ping:
                interfaceAdapter->answerToPing( command );
                break;
            // Quit immediately
            case cmd_Quit:
                searchMustBeInterrupted = true;
                state = state_Quitting;
                yield = true;
                break;
            // Result
            case cmd_Result:
                state = state_Observing;
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Set board to specified FEN position
            case cmd_SetBoard:
                resetBoard( command.strParam(0) );
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Set clock
            case cmd_SetClock:
                timeOnClock = command.intParam(0) * 10;
                break;
            // Set fixed depth
            case cmd_SetFixedDepth:
                searchMode = mode_FixedDepth;
                fixedSearchDepth = command.intParam(0);
                break;
            // Set fixed time
            case cmd_SetFixedTime:
                searchMode = mode_FixedTime;
                timeCurrentTarget = command.intParam(0) * 1000;
                break;
            // Set level
            case cmd_SetLevel:
                searchMode = mode_TimeControl;
                timeManager.reset();
                timeManager.addControl(
                    command.intParam(0),
                    command.intParam(1),
                    command.intParam(2) );
                break;
            // Set opponent time
            case cmd_SetOpponentClock:
                timeOnOpponentClock = command.intParam(0) * 10;
                break;
            // Opponent is computer
            case cmd_SetOpponentIsComputer:
                opponentIsComputer = true;
                break;
            // Set pondering off
            case cmd_SetPonderingOff:
                ponderingEnabled = false;
                break;
            // Set pondering on
            case cmd_SetPonderingOn:
                ponderingEnabled = true;
                break;
            // Show book
            case cmd_ShowBook:
                displayBookInfo();
                break;
            // Show hint
            case cmd_ShowHint:
                {
                    Move hintMove = Move::Null;

                    if( state == state_Analyzing || state == state_AnalysisComplete ) {
                        hintMove = gameMoveToPlay.pv[0];
                    }
                    else if( state == state_Pondering ) {
                        hintMove = ponderMove;
                    }

                    if( hintMove != Move::Null ) {
                        interfaceAdapter->sendHint( gamePosition, gameMoveToPlay.pv[0] );
                    }
                }
                break;
            // Show thinking
            case cmd_ShowThinking:
                showThinking = true;
                break;
            // Undo last full move (i.e. two half moves)
            case cmd_UndoLastFullMove:
                if( hasPonderMoveOnBoard() ) {
                    // Undo the ponder move first
                    undoMove( 1 );
                }
                undoMove( 2 );
                timeManager.goPrevMove();
                searchMustBeInterrupted = true;
                yield = true;
                break;
            // Undo
            case cmd_UndoLastHalfMove:
                if( state == state_Observing ) {
                    undoMove( 1 );
                }
                else if( state == state_Analyzing || state == state_AnalysisComplete ) {
                    undoMove( 1 );
                    searchMustBeInterrupted = true;
                    state = state_Analyzing;
                }
                break;
            // Add to book tree
            case cmd_KiwiAddToBookTree:
                bookTree.addGameCollection( 
                    command.strParam(0),
                    command.intParam(0),
                    command.intParam(1) );
                break;
            // Get best move info according to EGTB and recognizer
            case cmd_KiwiBestMove:
                getBestMove( gamePosition );
                break;
            // Run evaluation test on suite
            case cmd_KiwiEvaluateSuite:
                runEvalSuiteEPD( command.strParam(0) );
                break;
            // Export book tree
            case cmd_KiwiExportBookTree:
                bookTree.exportToBookFile( 
                    command.strParam(0),
                    command.intParam(0) );
                break;
            // Generate bitbases
            case cmd_KiwiGenBB:
                generateBitbases();
                break;
            // Display help
            case cmd_KiwiHelp:
                printf( "Welcome to %s by %s.\n\n", myName, myAuthor );
                printf( "Please use the standard WinBoard/XBoard command set to talk with the engine,\n" );
                printf( "or one of the following commands:\n\n" );
                printf( "bookadd    [filename] [min moves per game] [max plies to consider]\n" );
                printf( "bookload   [filename]\n" );
                printf( "booksave   [filename] [min occurences of a book position]\n" );
                printf( "perft      [depth]\n" );
                printf( "suite      [filename] [seconds per move] [optional: max depth]\n" );
                break;
            // Load book
            case cmd_KiwiLoadBook:
                {
                    Book * book = new Book;

                    if( book->loadFromFile( command.strParam(0) ) == 0 ) {
                        delete openingBook;
                        openingBook = book;
                        printf( "Book loaded successfully (%u positions)\n", book->entryCount() );
                    }
                    else {
                        delete book;
                        printf( "Error: unable to load book, current book is unchanged.\n" );
                    }
                }
                break;
            // Run perft() on current position
            case cmd_KiwiPerft:
                {
                    char fen[200];

                    gamePosition.getBoard( fen );

                    perft( fen, command.intParam(0) );
                }
                break;
            // Run test suite
            case cmd_KiwiRunSuite:
                runTestSuiteEPD( command.strParam(0), 
                    command.intParam(0),
                    command.intParamCount() > 1 ? command.intParam(1) : MaxSearchPly );
                break;
            // Set engine option
            case cmd_KiwiSetOption:
                handleSetOptionCommand( command.strParam(0), command.strParam(1) );
                break;
            // Run whatever test is currently set up
            case cmd_KiwiTest:
                test();
                break;
            // Set position and analyze
            case cmd_KiwiAnalyze:
                resetBoard( command.strParam(0) );
                searchMustBeInterrupted = true;
                yield = true;
                state = state_Analyzing;
                break;
            // Unknown command... shouldn't happen!
            default:
                Log::write( "Engine... received an unknown command!\n" );
                break;
            }
        }
    }

    return 0;
}

int Engine::runTestSuiteEPD( const char * name, int secondsPerMove, int maxDepth )
{
    FILE *  f;
    char    b[1024];

    // Open file
    f = fopen( name, "r" );

    if( f == NULL ) {
        printf( "*** Error: cannot open file!\n" );
        return -1;
    }

    Log::write( "Test suite '%s', %d seconds per move, max depth=%d\n\n", name, secondsPerMove, maxDepth );

    // Setup search parameters
    int timeBonusForLogging = 50;   // Compensate the time spent in logging (milliseconds)
    int currentState = state;

    searchMode = mode_FixedTime;
    timeCurrentTarget = secondsPerMove * 1000 + timeBonusForLogging;

    // Force thinking state
    state = state_Thinking;

    // 0.6b: this allowed to run multiple test using redirected input,
    // but it is in fact most annoying!
    // inputCheckWhileSearching = false;

    int num = 0;        // Positions searched
    int solved = 0;     // Positions solved

    while( true ) {
        int l;
        char * s;

        if( fgets( b, sizeof(b), f ) == NULL ) 
            break;

        b[ sizeof(b)-1 ] = '\0';

        l = (int) strlen(b);

        if( l <= 5 ) 
            continue;

        b[l-1] = 0;

        Log::write( "%s\n", b );

        // Look for the best move tag
        s = strstr( b, "bm " );

        if( s == 0 ) {
            Log::write( "suite: best move tag not found, quitting...\n" );
            break;
        }

        // Save the best move list
        char bestmove[100];

        strcpy( bestmove, s+3 );

        // Remove the best move tag and setup the position
        strcpy( s, "0 0" );

        // Fix the best move list
        s = strchr( bestmove, ';' );

        if( s != 0 ) {
            *s = 0;
        }

        strcat( bestmove, " " );

        // Update the position count
        num++;

        // Let the engine think about it
        resetBoard( b );

        Counters::reset();

        gameMoveToPlay.reset();

        Position pos( gamePosition );

        searchStartTime = System::getTickCount();
        searchMustBeInterrupted = false;

        searchPosition( pos, 0, maxDepth );

        unsigned timeSpent = System::getTickCount() - searchStartTime;
        
        // Show principal variation and update statistics
        updateThinkingDisplay();

        Counters::dump();

        char kiwimove[10];

        SAN::moveToText( kiwimove, gamePosition, gameMoveToPlay.pv[0] );

        if( strstr( bestmove, kiwimove ) ) {
            solved++;

            Log::write( "=> Ok [%d/%d]\n\n", solved, num );
        }
        else {
            Log::write( "=> Not solved [%d/%d]\n\n", solved, num );
        }

        if( timeSpent > 0 ) {
            Log::write( "NPS = %u K\n", Counters::posSearched / timeSpent );
        }

        printf( "%d/%d\n", solved, num );
    }

    if( num > 0 ) {
        int pct = (solved * 1000) / num;
        Log::write( "\nPositions: %d, solved: %d (%02d.%d%%)\n", num, solved, pct / 10, pct % 10 );
    }
    else {
        Log::write( "No positions found!\n" );
    }

    // Restore previous state
    state = currentState;

    inputCheckWhileSearching = true;
    
    return 0;
}

int Engine::runEvalSuiteEPD( const char * name )
{
    FILE *  f;
    char    b[1024];

    // Open file
    f = fopen( name, "r" );

    if( f == NULL ) {
        printf( "*** Error: cannot open file!\n" );
        return -1;
    }

    Log::write( "Evaluation test for suite '%s'\n\n", name );

    int num = 0;    // Positions searched
    int err = 0;

    while( true ) {
        int     l;
        char *  s;

        if( fgets( b, sizeof(b), f ) == NULL ) 
            break;

        b[ sizeof(b)-1 ] = '\0';

        l = (int) strlen(b);

        if( l <= 5 ) 
            continue;

        b[l-1] = 0;

        Log::write( "%s\n", b );

        // Look for the best move tag and remove it if found
        s = strstr( b, "bm " );

        if( s != 0 ) {
            strcpy( s, "0 0" );
        }

        // Update the position count
        num++;

        // Evaluate the position
        Position pos;

        pos.setBoard( b );

        int eval = pos.getEvaluation();

        // Reverse the position and evaluate it again
        char * fen = b;

        for( int rank=7; rank>=0; rank-- ) {
            int empty = 0;

            for( int file=0; file<8; file++ ) {
                int p = pos.board.piece[ FileRankToSquare(file,7-rank) ];

                if( p == None ) {
                    empty++;
                }
                else {
                    if( empty > 0 ) {
                        *fen++ = (char) (empty + '0');
                        empty = 0;
                    }

                    switch( p ) {
                    case BlackPawn:   *fen = 'P'; break;
                    case BlackKnight: *fen = 'N'; break;
                    case BlackBishop: *fen = 'B'; break;
                    case BlackRook:   *fen = 'R'; break;
                    case BlackQueen:  *fen = 'Q'; break;
                    case BlackKing:   *fen = 'K'; break;
                    case WhitePawn:   *fen = 'p'; break;
                    case WhiteKnight: *fen = 'n'; break;
                    case WhiteBishop: *fen = 'b'; break;
                    case WhiteRook:   *fen = 'r'; break;
                    case WhiteQueen:  *fen = 'q'; break;
                    case WhiteKing:   *fen = 'k'; break;
                    }

                    fen++;
                }
            }

            if( empty > 0 ) {
                *fen++ = (char) (empty + '0');
            }

            *fen++ = (rank == 0) ? ' ' : '/';
        }

        // Side to play (reversed!)
        *fen++ = (pos.sideToPlay == Black) ? 'w' : 'b';
        *fen++ = ' ';

        // Casting availability (reversed!)
        if( pos.boardFlags & Position::AnyCastle ) {
            if( pos.boardFlags & Position::WhiteCastleKing ) *fen++ = 'k';
            if( pos.boardFlags & Position::WhiteCastleQueen ) *fen++ = 'q';
            if( pos.boardFlags & Position::BlackCastleKing ) *fen++ = 'K';
            if( pos.boardFlags & Position::BlackCastleQueen ) *fen++ = 'Q';
        }
        else {
            *fen++ = '-';
        }

        // En-passant availability, half-move clock, move number
        strcpy( fen, " - - -" );

        pos.dump();

        if( pos.setBoard( b ) != 0 ) {
            Log::write( "*** Warning: error setting reverse position!\n" );
        }

        pos.dump();

        int reverse_eval = -pos.getEvaluation();

        if( eval != reverse_eval ) {
            err++;
            Log::write( "Error! Eval=%d, reverse eval=%d\n\n", eval, reverse_eval );
        }
        else {
            Log::write( "= %d\n\n", eval );
        }
    }

    if( num > 0 ) {
        Log::write( "\nPositions: %d, errors: %d\n", num, err );
    }
    else {
        Log::write( "No positions found!\n" );
    }

    printf( "Positions examined: %d, errors: %d\n", num, err );

    return 0;
}

int Engine::test()
{
    /*
        Bizarre positions (from the Valentin Albillo website)
    */
    /*
    static char * bizarre1 = "4k3/nnnnnnnn/8/8/8/8/RRRRRRRR/4K3/ w - -"; // Rxc7 mate in 7
    static char * bizarre2 = "4k3/bbbbbbbb/8/8/8/8/RRRRRRRR/4K3/ w - -"; // Rxa7 mate in 8
    static char * bizarre3 = "nnn5/nkn5/nnn5/8/8/5QQQ/5QKQ/5QQQ/ w - -"; // Qg7 or Qxc8+ mate in 9 (tough!)
    static char * bizarre4 = "RnBqkBnR/PPppppPP/8/8/8/8/ppPPPPpp/rNbQKbNr/ w - -"; // hxg8=N mate in 8 (a wonderful underpromotion... incredibly enough Kiwi finds it almost immediately!)
    static char * bizarre5 = "8/2pPpP2/1P1qk1p1/1p4P1/1P4p1/1p1QK1P1/2PpPp2/8/ w - -"; // f8=Q mate in 12 (very tough!)
    */

    return 0;
}
