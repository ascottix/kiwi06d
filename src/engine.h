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
#ifndef ENGINE_H_
#define ENGINE_H_

#include "adapter.h"
#include "book.h"
#include "hash.h"
#include "move.h"
#include "movelist.h"
#include "pawnhash.h"
#include "position.h"
#include "system.h"
#include "time_manager.h"

const int   NodesBetweenInputChecks = 20000;

struct MoveInfo
{
    enum {
        MaxMovesInPV    = 10        // Actually, half moves (plies)
    };

    int         score;              // Score (centipawns)
    int         depth;              // Search depth (plies)
    int         maxdepth;           // Max search depth (after extensions)
    unsigned    time;               // Time spent in search (hundredths of second)
    unsigned    nodes;              // Nodes searched
    Move        pv[MaxMovesInPV];   // Principal variation
    int         pvlen;              // Length of principal variation

    void reset() {
        pv[0] = Move::Null;
        pvlen = 0;
    }
};

struct RootMoveStat
{
    int         depth;              // Current search depth
    unsigned    time;               // Time spent in search (hundredths of second)
    unsigned    nodes;              // Nodes searched
    int         moves_remaining;    // Number of moves to examine before going to next depth level
    int         moves_total;        // Number of legal moves to examine
    Move        current_move;

    void reset() {
        current_move = Move::Null;
    }
};

struct RootMove
{
    Move        move;
    unsigned    nodes;
    unsigned    leaves;
    int         value;
};

struct RootMoveList
{
    int         count;
    RootMove    moves[ MoveList::MaxMoveCount ];
};

class Engine 
{
public:
    static const char * myName;     // Engine name
    static const char * myAuthor;   // Author name

    enum {
        // Length of a full ply: it is a multiple of 2, 3, 4, 5, 6 and 10
        // so there is a lot of freedom in choosing extension values!
        FullPlyDepth        = 60,

        // Max (half) moves per game, the theoretical limit should be 6350 or so...
        MaxMovesPerGame     = 600,

        // Max number of consecutive "out of book" full moves before book is not probed anymore
        MaxNotInBookMoves   = 3,

        // Max ply reached during a search
        MaxSearchPly        = 63,

        // Time/depth control modes
        mode_FixedTime      = 0,        // Search stops after a fixed time
        mode_FixedDepth,                // Search stops after reaching a fixed depth
        mode_TimeControl,               // Time management functions decide when to stop search

        // Engine states
        state_Observing,        // Check legality of moves, do not think
        state_Thinking,         // In game: think and play
        state_Pondering,        // In game: ponder opponent move
        state_PonderMissed,     // In game: pondered move wasn't played (intermediate state, bridge from ponder to thinking)
        state_PonderComplete,   // In game: ponder completed before receiving a user move
        state_Waiting,          // In game: wait opponent move, do not ponder
        state_Analyzing,        // Analysis: current position in progress
        state_AnalysisComplete, // Analysis: nothing more to do for current position (e.g max depth reached)
        state_Quitting          // About to quit
    };

    // Search depth parameters
    static int  maxSearchDepthFactor;

    // Null move parameters
    static int  nullMoveMinDepth;       // No null-move if closer than this to horizon (zero depth)
    static int  nullMoveMinReduction;   // Minimum null-move depth reduction
    static int  nullMoveMaxReduction;   // Maximum null-move depth reduction

    // Search extension parameters
    static int  initialExtensionBonus;  // Initial extension bonus granted when starting a new search
    static int  maxExtensionPerPly;     // Max extension per ply
    static int  extendCheck;            // Escaping from check
    static int  extendPawnOn7th;        // Pawn pushed to the 2nd or 7th rank
    static int  extendThreat;           // Null-moving results in getting mated
    static int  extendRecapture;        // Recapture on last capture square that restores material
    static int  extendSingleReply;      // Position has only one valid move
    static int  extendPawnEndgame;

    // Pruning parameters
    static int  pruneMarginAtFrontier;      // Futility pruning (depth < 2)
    static int  pruneMarginAtPreFrontier;   // Futility pruning (depth < 3)

    // Hash table
    static int  sizeOfHashTable;        // Number of entries (must be a power of two)
    static int  sizeOfPawnHashTable;    // Number of entries (must be a power of two)

    // Resign threshold
    static int  resignThreshold;

    // Search mode parameters
    static unsigned fixedSearchDepth;   // For "fixed depth" searches
    static int      searchMode;         // Search mode, see mode_Xyz constants above
    static bool     ponderingEnabled;

    // Time management: if current score is less than the score of the last 
    // move played minus the margin below, search time is extended (also, other
    // conditions must be met before time is reduced at the first checkpoint)
    static int      scoreMarginAt1stCheck;  // At  50% time, score margin can be negative here!
    static int      scoreMarginAt2ndCheck;  // At 100% time
    static int      scoreMarginAt3rdCheck;  // At 200% time
    static int      safetyTimePerMove;      // Extra time the engine keeps aside for each move (for lag and so on)

    // Time controls
    static int  timeLeft;
    static int  opponentTimeLeft;
    static int  movesPerInterval;
    static int  sizeOfTimeInterval;     // In milliseconds
    static int  sizeOfTimeIncrement;    // In milliseconds
    static int  movesUntilTimeCheck;

    static int  state;

public:
    // Initialization
    static void initialize();

    static HashTable * getHashTable() {
        return hashTable;
    }

    static PawnHashTable * getPawnHashTable() {
        return pawnHashTable;
    }

    static int main( Adapter * adapter );

    static Adapter * getInterfaceAdapter() {
        return interfaceAdapter;
    }

    static int resetBoard( const char * fen );
    
private:
    struct Rep3Info
    {
        BitBoard    hashCode;
        unsigned    repCount;
    };

    static void think();
    static void ponder();
    static void analyze();

    static bool isTimeOut();  

    // Test
    static int test();
    static int perft( const char * fen, int max_depth );
    static int perftRunSuite();
    static int runTestSuiteEPD( const char * name, int secondsPerMove, int maxDepth );
    static int runEvalSuiteEPD( const char * name );

    // Game
    static int handleThinkingComplete();
    static int handleInput();
    static int handleSetOptionCommand( const char * key, const char * value );
    static int handleOpponentMove( const char * moveAsText );
    static int playMove( Move move, MoveInfo * info = 0 );
    static int undoMove( int numOfHalfMoves = 1 );
    static bool isGameEnded();
    static bool hasPonderMoveOnBoard();
    static void setTimeTargetForMove();
    static unsigned timeSpentInSearch();
    static int updatePrincipalVariation( const Position & position, Move * pv, int pvofs, int pvlen );
    static void updateThinkingDisplay();
    static void setMoveToPlay( Move m, int score, int depth, int maxdepth, int nodes );
    static void initializeSearch();
    static int getFullMovesPlayedFor( int side );

    // Book
    static Move getBookMove( Book & book, const Position & pos );
    static unsigned getBookMoves( Book & book, const Position & pos, MoveList & moves, unsigned * weights );
    static void displayBookInfo();

    static Adapter *    interfaceAdapter;
    static bool         showThinking;
    static unsigned     showThinkingLastUpdate;
    static HashTable *  hashTable;          // Main hashtable (for search)
    static PawnHashTable * pawnHashTable;   // Pawn hashtable (for evaluation)
    static volatile bool searchMustBeInterrupted;
    static unsigned     searchStartTime;
    static Rep3Info     rep3History[MaxMovesPerGame+MaxSearchPly];
    static MoveInfo     moveHistory[MaxMovesPerGame];
    static Position     gameHistory[MaxMovesPerGame+MaxSearchPly];
    static int          gameHistoryIdx; // Same as number of half moves played since start position
    static Position     gamePosition;
    static MoveInfo     gameMoveToPlay;
    static TimeManager  timeManager;
    static unsigned     timeOnClock;
    static unsigned     timeOnOpponentClock;
    static unsigned     timeDefaultTargetForMove;   // Default time allocated for move
    static unsigned     timeCurrentTarget;          // Current target for move: can be from 50% to 400% of default
    static unsigned     timeCurrentStage;           // Current stage for checking if move is good enough for time target (from 0 to 3)
    static bool         inputCheckWhileSearching;   // If false, input is not checked during a search
    static Move         ponderMove;                 // Move that is being pondered
    static RootMoveStat rootMoveStat;
    static bool         opponentIsComputer;

    static Book *       openingBook;
    static int          numOfMovesNotInBook;
    static BookTree     bookTree;   // For creating and exporting books

    static bool isSearchOver();

    static int negaMaxQuiesceMT( Position & pos, int gamma, int ply, int checks_depth = 0 );
    static int negaMaxMT( Position & pos, int gamma, int ply, int depth );
    static int negaMaxMT_AtRoot( Position & pos, int gamma, int depth, RootMoveList & moves );
    static int initializeSearch( const Position & pos, int initial_score, RootMoveList & moves );
    static int searchMTDf( Position & pos, int f, int depth, RootMoveList & moves );
    static int searchPosition( Position & pos, int f, int depth );
};

#endif // ENGINE_H_
