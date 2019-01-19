/*
    Kiwi
    Searching

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
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "counters.h"
#include "engine.h"
#include "hash.h"
#include "log.h"
#include "mask.h"
#include "metrics.h"
#include "move.h"
#include "movelist.h"
#include "movehandler.h"
#include "position.h"
#include "recognizer.h"
#include "san.h"
#include "score.h"
#include "undoinfo.h"

const bool  isNullMoveEnabled       = true;
const bool  isFutilityEnabled       = true;
const bool  haveHistoryPruning      = true;
const bool  haveRecognizersInSearch = true;
const bool  haveRootMoveOrdering    = true;

int nodesUntilInputCheck     = NodesBetweenInputChecks;
int maxSearchPly;            // Max search depth for current iteration (plies)
int maxDepthReached;

// Define FULL_NODE_EVAL to get static evaluation at each node, which costs
// time but improves null move and futility
#define FULL_NODE_EVAL

/*
    Test: r1b1kb1r/pp1n1ppp/2p2n2/q7/P1BpP3/2N2N2/1P1B1PPP/R2QK2R w KQkq -
    ...not good with fail history (best move is Bxf7+)!

    Test: 8/1QB3k1/1P2p3/1P2PqP1/6K1/5P2/8/8 w - - 0 66
    ...there's a draw by rep, but Kiwi can't see it!

    Test: 8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - bm Rxb2; id "WAC.002";
*/

struct HistoryInfo
{
    int count;
    int fail_high;
};

HistoryInfo histTable[12*64];

void clearHistTable()
{
    memset( histTable, 0, sizeof(histTable) );
}

bool Engine::isSearchOver()
{
    if( isTimeOut() ) {
        searchMustBeInterrupted = true;
    }
    else {
        nodesUntilInputCheck = NodesBetweenInputChecks;

        if( inputCheckWhileSearching ) {
            if( System::isInputAvailable() ) {
                handleInput();
            }
        }
    }

    return searchMustBeInterrupted;
}

/*
    Because MTD(f) works with null-windows only, there is no need to specify both
    alpha and beta: here alpha=gamma-1 and beta=gamma.

    Note:
    1) x > alpha <=> x > gamma-1 <=> x >= gamma
    2) x < beta  <=> x < gamma

    Note: this function must be called with ply >= 1, otherwise the rep3History
    array is not updated correctly.
*/
int Engine::negaMaxMT( Position & pos, int gamma, int depth, int ply )
{
    // Check for input every now and then
    if( --nodesUntilInputCheck <= 0 ) {
        isSearchOver();
    }

    // Exit now if search must be interrupted
    if( searchMustBeInterrupted ) {
        return 0;
    }

    // Update counter
    Counters::posSearched++;

    // If no depth remaining or going to deep, return
    if( depth < FullPlyDepth || ply >= maxSearchPly ) {
        if( ply > maxDepthReached ) {
            maxDepthReached = ply;
        }

        unsigned checks_depth = 1;

        int result = negaMaxQuiesceMT( pos, gamma, ply, checks_depth );

        return result;
    }

    // Check for draw by 3-fold repetition or 50 moves rule
    int repCount = 0;

    if( pos.boardFlags & Position::PositionRepeatPossible ) {
        int hmc = pos.getHalfMoveClock();

        // Check the 50 move rule
        if( hmc >= 100 ) {
            return 0;
        }

        // It is possible that this position appeared before... check it
        int n = gameHistoryIdx + ply - 4;
        int e = gameHistoryIdx + ply - hmc;

        while( n >= e ) {
            if( pos.hashCode == rep3History[ n ].hashCode ) {
                // Found, update repetition count and exit from the loop
                repCount = 1 + rep3History[ n ].repCount;

                // Use 2-fold repetition during search
                return 0;

                // ...still leave the "break" here for safety!
                break;
            }

            n -= 2;
        }
    }

    if( ! pos.hasBlackPawns() && ! pos.hasWhitePawns() && (pos.numOfBlackPieces() + pos.numOfWhitePieces()) <= 1 ) {
        if( (pos.numOfBlackMajorPieces() + pos.numOfWhiteMajorPieces()) == 0 ) {
            return 0;
        }
    }

    // Update search path info
    gameHistory[ gameHistoryIdx + ply ].materialScore = pos.materialScore;

    rep3History[ gameHistoryIdx + ply ].hashCode = pos.hashCode;
    rep3History[ gameHistoryIdx + ply ].repCount = repCount;

    // Initialize variables
    HashTable::Entry *  hashEntry;
    Move                hashMove    = Move::Null;

    bool    hasSingleReply = false;
    bool    hasMateThreat = false;

    // Lookup the current position in the transposition table
    hashEntry = hashTable->probe( pos );

    if( hashEntry != 0 ) {
        Move m = hashEntry->getMove();

        if( m != Move::Null && ! pos.isValidMove(m) ) {
            // Bad hash entry
            hashEntry = 0;
        }
    }

    // If the position was found in the transposition table,
    // check if we can get a quick exit
    if( hashEntry != 0 ) {
        // Position found in the hash table
        int value = hashEntry->getValue();

        // If score is mate, we must convert it from relative to absolute (for the current ply)
        if( value < Score::MateLo ) {
            value += ply;
        }
        else if( value > Score::MateHi ) {
            value -= ply;
        }

        if( hashEntry->getDepth() >= depth ) {
            // The position in the hash table has already been searched deeper
            // than we're going to do, so it's quite safe to reuse that value
            if( hashEntry->isUpperBound() ) {
                if( value < gamma ) {
                    return value;
                }
            }
            else {
                if( value >= gamma ) {
                    return value;
                }
            }
        }

        // Save hash move for later
        hashMove = hashEntry->getMove();

        // Set extension flags
        hasSingleReply = hashEntry->hasSingleReply() != 0;
        hasMateThreat = hashEntry->hasMateThreat() != 0;
    }

    // Probe recognizers
    if( haveRecognizersInSearch ) {
        RecognizerInfo  recognizerInfo;

        if( Recognizer::probe( pos, recognizerInfo ) ) {
            if( recognizerInfo.type() == rtExact ) {
                return recognizerInfo.adjust( ply );
            }
            else if( recognizerInfo.type() == rtLowerBound && recognizerInfo.value() >= gamma ) {
                return recognizerInfo.value();
            }
            else if( recognizerInfo.type() == rtUpperBound && recognizerInfo.value()  < gamma ) {
                return recognizerInfo.value();
            }
        }
    }

    // Keep searching the node, trying the null move first
    UndoInfo    undoinfo( pos );
    int         side = pos.sideToPlay;
    unsigned    inCheck = pos.boardFlags & Position::SideToPlayInCheck;
    bool        sideHasFewPieces = (side == Black ? pos.numOfBlackPieces() : pos.numOfWhitePieces()) < 2;

#ifdef FULL_NODE_EVAL
    int eval = side == Black ? -pos.getEvaluation() : +pos.getEvaluation();
#endif

    /*
        Null move pruning.

        Test: 4n1k1/2pr2pp/8/BN1Pp3/P3P3/5qPb/2Q4P/2R3K1 w - - 0 2
        ...wants to play Nc3 for a while, but that loses instantly to Qe3+

        Test: r2qr1k1/1bp2pbp/p2p1np1/1p4B1/1P1PP1N1/5N1P/P1Q2PP1/R3R1K1 b - - 0 23 
        ...plays Bc8 without seeing that it loses a piece to e5!
    */
    if( isNullMoveEnabled &&
#ifdef FULL_NODE_EVAL
        eval >= gamma &&
#endif
        (! Score::isMate(gamma)) &&
        (! inCheck) &&
        (depth >= nullMoveMinDepth) &&
        ! hasMateThreat &&
        ! sideHasFewPieces )
    {
        // Try the null move
        Counters::nullMoveAttempts++;

        // To compute the he null move reduced depth we start at nullMoveMinReduction
        // and increase the depth by one quarter for each ply above that, but
        // without exceeding the nullMoveMaxReduction cap
        int nullDepth = nullMoveMinReduction + (depth - nullMoveMinDepth) / 3;

        if( nullDepth > nullMoveMaxReduction ) {
            nullDepth = nullMoveMaxReduction;
        }

        nullDepth = depth - nullDepth;

        if( nullDepth < 0 ) {
            nullDepth = 0;
        }

        // Try to skip the search if we can somewhat foresee that
        // it won't produce a cutoff
        bool skip = false;

        if( (hashMove != Move::Null) && (hashEntry->getDepth() > nullDepth) ) {
            int value = hashEntry->getValue();

            if( (value + 25) < gamma ) {
                skip = true;
            }
        }

        if( ! skip ) {
            pos.doNullMove();

            int res;

            // Null-moving right into quiesce doesn't seem to work very well for me,
            // but code must still support it...
            if( nullDepth < FullPlyDepth ) {
                res = -negaMaxQuiesceMT( pos, 1-gamma, ply+1, 1 );
            }
            else {
                res = -negaMaxMT( pos, 1-gamma, nullDepth, ply+1 );
            }

            pos.undoNullMove( undoinfo );

            // Exit now if search must be interrupted
            if( searchMustBeInterrupted ) {
                return 0;
            }

            if( res >= gamma /*&& ((res > eval) || (res >= (gamma + 4)))*/ ) {
                Counters::nullMoveCutOffs++;

                hashTable->store( pos,
                    hashMove,
                    res,
                    HashTable::Entry::LowerBound,
                    depth );

                return res;
            }

            // Extend for threat if null moving leads to mate in the next ply;
            hasMateThreat = (res == ply + 2 - Score::Mate);
        }
    }

    // Internal iterative deepening: if we don't have a move to try
    // first perform a shallow search to get one
    if( hashMove == Move::Null && depth >= (4*FullPlyDepth) )
    {
        negaMaxMT( pos, gamma, depth-(2*FullPlyDepth), ply );

        hashEntry = hashTable->probe( pos );

        if( hashEntry != 0 ) {
            Move m = hashEntry->getMove();

            if( pos.isValidMove( m ) ) {
                hashMove = m;
            }
            else {
                hashEntry = 0;
            }
        }
    }

    // Pre-compute default extensions that apply to all moves
    int baseExtension = 0;

    if( inCheck ) {
        baseExtension += extendCheck;
    }

    if( hasMateThreat ) {
        baseExtension += extendThreat;
    }

    if( hasSingleReply ) {
        baseExtension += extendSingleReply;
    }

    // Generate and search all moves at this node
    MoveHandler moveHandler( pos, ply, GenerateForSearch, hashMove );
    Move        curr;
    Move        bestMove    = Move::Null;
    int         validMoves  = 0;
    bool        failedHigh  = false;
    int         result      = Score::Min;

    while( ! moveHandler.getNextMove( curr ) )
    {
        if( pos.doMove( curr ) != 0 ) {
            // Move not valid, skip
            Counters::posInvalid++;
            pos.undoMove( curr, undoinfo );
            continue;
        }

        validMoves++;

        // Compute extensions
        int depthExtension = baseExtension;

        int to = curr.getTo();

        if( (RankOfSquare(to) == 1 || RankOfSquare(to) == 6) && PieceType( pos.board.piece[ to ] ) == Pawn ) {
            depthExtension += extendPawnOn7th;
        }

        if( ply >= 2 && curr.isCapture() ) {
            int trade = gameHistory[ gameHistoryIdx ].materialScore - pos.materialScore;

            if( trade >= -20 && trade <= +20 ) {
                depthExtension += extendRecapture;
            }

            // Extend greatly if entering into a pawn endgame
            if( pos.numOfWhitePieces() == 0 && pos.numOfBlackPieces() == 0 ) {
                int mat = iabs( gameHistory[ gameHistoryIdx + ply - 1 ].materialScore - pos.materialScore );

                if( mat > Score::Pawn ) {
                    depth += 2*FullPlyDepth;
                }
            }
        }

        if( depthExtension > maxExtensionPerPly ) {
            depthExtension = maxExtensionPerPly;
        }

        unsigned givesCheck = pos.boardFlags & Position::SideToPlayInCheck;

        /*
            Futility pruning.

            If we are close to the horizon and the move does not appear to be interesting enough,
            skip it.
-
            The following checks and margins should be quite safe. I have run a test on the WAC suite where
            pruned moves were actually flagged and searched: of those less than 0.01% failed high, usually
            with a value that was only a little higher than gamma.

            The moves for which the above doesn't work at all are the tactical moves such as forks and skewers
            that result in gaining a material advantage. However, it seems they are quite rare and only a handful
            of them was found in the test.
        */
        if( isFutilityEnabled &&
            depthExtension == 0 &&
            depth < 3*FullPlyDepth &&
            result != Score::Min &&     // Before pruning, we must have a valid score to return
            ! sideHasFewPieces &&
            ! givesCheck &&
            ! curr.isCaptureOrPromotion() &&
            true )
        {
#ifdef FULL_NODE_EVAL
            int approximateValue = eval + ((depth < 2*FullPlyDepth) ? 100 : 300);
#else
            int materialScore = pos.blackToMove() ? -pos.materialScore : +pos.materialScore;
            int approximateValue = materialScore + ((depth < 2*FullPlyDepth) ? pruneMarginAtFrontier : pruneMarginAtPreFrontier);
#endif

            if( approximateValue < gamma ) {
                pos.undoMove( curr, undoinfo );
                continue;
            }
        }

        int hpiece = (pos.board.piece[ curr.getTo() ] >> 1) - 1;

        int hindex = hpiece * 64 + curr.getTo();

        if( haveHistoryPruning &&
            depthExtension == 0 &&
            validMoves >= 4 &&
            ! curr.isCaptureOrPromotion() &&
            ! givesCheck &&
            depth >= (4*FullPlyDepth) &&
            true ) 
        {
            int n = histTable[ hindex ].count;
            int f = histTable[ hindex ].fail_high;

            if( f < (n / 8) ) {
                depthExtension = -FullPlyDepth;
            }
        }

        unsigned nodes = Counters::callsToEvaluation + Counters::posSearched;

re_search:
        int temp = -negaMaxMT( pos, 1-gamma, depth+depthExtension-FullPlyDepth, ply+1 );

        // Exit now if search must be interrupted
        if( searchMustBeInterrupted ) {
            pos.undoMove( curr, undoinfo );
            return 0;
        }

        if( temp >= gamma ) {
            if( depthExtension < 0 ) {
                depthExtension = 0;
                goto re_search;
            }

            histTable[ hindex ].fail_high++;
        }

        histTable[ hindex ].count++;

        nodes = Counters::callsToEvaluation + Counters::posSearched - nodes;

        if( temp > result || bestMove == Move::Null ) {
            bestMove = curr;

            // Set result
            result = temp;

            // Check if result is good enough to produce a cutoff
            if( result >= gamma ) {
                failedHigh = true;

                ++Counters::anyFailedHigh;
                if( validMoves == 1 ) ++Counters::firstFailedHigh;
                if( validMoves == 2 ) ++Counters::secondFailedHigh;

                pos.undoMove( curr, undoinfo );

                break;
            }
        }

        pos.undoMove( curr, undoinfo );
    }

    if( validMoves == 0 ) {
        // Couldn't find a valid move so it's mate or stalemate
        return inCheck ? ply - Score::Mate : 0;
    }

    /*
        Update history and killer tables: this seems to work
        better if we only reward moves that caused a fail high.
    */
    if( failedHigh ) {
        if( ! bestMove.isCaptureOrPromotion() ) {
            MoveHandler::addToKillerTable( bestMove, ply );
            MoveHandler::updateHistoryTable( side, bestMove, depth );
        }
    }

    /*
        Store the move in the hash table.
    */
    // Draw by rep: 8/1QB3k1/1P2p3/1P2PqP1/6K1/5P2/8/8 w - - 0 66
    //if( 1 || failedHigh || (hashMove == Move::Null) ) {
    if( failedHigh || (! Score::isMate(gamma)) ) {
        // Adjust score and depth for hash table (if needed)
        int hashResult = result;

        // If mate, adjust score so that it's relative to the current position
        // (rather than to the root)
        if( hashResult > Score::MateHi ) {
            hashResult += ply;
        }
        else if( hashResult < Score::MateLo ) {
            hashResult -= ply;
        }

        unsigned flags = result >= gamma ?
            HashTable::Entry::LowerBound :
            HashTable::Entry::UpperBound;

        if( (! failedHigh && validMoves == 1) || hasSingleReply ) {
            flags |= HashTable::Entry::SingleReply;
        }

        if( hasMateThreat ) {
            flags |= HashTable::Entry::MateThreat;
        }

        // Update hash table
        hashTable->store( pos,
            bestMove,
            hashResult,
            flags,
            depth );

    }

    // Note: depth could have be modified at this point and must not be used anymore
    return result;
}

int Engine::searchMTDf( Position & pos, int f, int depth, RootMoveList & moveList )
{
    int result  = f;
    int g       = f;
    int lower   = Score::Min;
    int upper   = Score::Max;
    int gamma   = (g == lower) ? g+1 : g;

    // Do not search deeper than this
    maxSearchPly = maxSearchDepthFactor*depth;

    if( maxSearchPly > MaxSearchPly ) {
        maxSearchPly = MaxSearchPly;
    }

    maxDepthReached = 0;

    // Test: r1b1r1k1/p1q2p1p/nppb2N1/3p4/3P3Q/3B4/PPPN1PPP/R3R1K1 b - - 0 15 
    // ...fails low too slowly

    // Convert depth in the "fractional" format used by negaMaxMT()
    int fractDepth = depth * FullPlyDepth + initialExtensionBonus;

    do {
        g = negaMaxMT_AtRoot( pos, gamma, fractDepth, moveList );

        if( searchMustBeInterrupted ) {
            break;
        }

        // Update bounds
        if( g < gamma ) {
            upper = g;
            gamma = g;
        }
        else {
            lower = g;
            gamma = g + 1;
        }

        // Save the move found
        setMoveToPlay( moveList.moves[0].move, g, depth, maxDepthReached, Counters::posSearched );

        // Keep the score
        result = g;
    } while( lower < upper );

    return result;
}

int Engine::initializeSearch( const Position & pos, int initial_score, RootMoveList & moveList )
{
    int i;

    // Reset search tables
    MoveHandler::resetKillerTable();
    MoveHandler::resetHistoryTable();

    clearHistTable();

    // Reset counters
    Counters::reset();

    // Lookup the position in the hash table: that will suggest which move to consider first
    Move hashMove = Move::Null;
    HashTable::Entry * entry = getHashTable()->probe( pos );

    if( entry !=  0 ) {
        Move m = entry->getMove();

        Position p( pos );

        if( p.isValidMove(m) && (p.doMove(m) == 0) ) {
            LOG(( "Found hash move: %s\n", SAN::moveToText(pos,m) ));

            hashMove = m;
        }
    }

    // Generate the root move list
    MoveList    rootMoves;

    pos.generateValidMoves( rootMoves );

    moveList.count = rootMoves.count();

    for( i=0; i<moveList.count; i++ ) {
        moveList.moves[i].move = rootMoves.get( i );
        moveList.moves[i].value = 0;
        moveList.moves[i].nodes = 0;
        moveList.moves[i].leaves = 0;
    }

    // Assign a score to each move
    for( i=0; i<moveList.count; i++ ) {
        if( moveList.moves[i].move == hashMove ) {
            // Move from the hash table always gets the max score
            moveList.moves[i].value = Score::Max;
        }
        else {
            // Use quiesce search to quickly evaluate the move
            Position p( pos );

            p.doMove( moveList.moves[i].move );

            int score = negaMaxQuiesceMT( p, Score::Max, 0 );

            moveList.moves[i].value = pos.sideToPlay == Black ? -score : score;
        }
    }

    // Sort the move list so that better moves are examined first
    for( i=0; i<moveList.count-1; i++ ) {
        int m = i;

        for( int j=i+1; j<moveList.count; j++ ) {
            if( moveList.moves[j].value > moveList.moves[i].value ) {
                m = j;
            }
        }

        if( m != i ) {
            RootMove temp = moveList.moves[i];

            moveList.moves[i] = moveList.moves[m];

            moveList.moves[m] = temp;
        }
    }

    // No move to play yet!
    gameMoveToPlay.reset();

    return moveList.count;
}

int Engine::searchPosition( Position & pos, int initial_score, int maxdepth )
{
    // Initialize search
    RootMoveList moveList;

    initializeSearch( pos, initial_score, moveList );

    // Look for a book move
    if( (state != state_Analyzing) && (numOfMovesNotInBook < MaxNotInBookMoves) ) {
        Move m = getBookMove( *openingBook, pos );

        if( m != Move::Null ) {
            numOfMovesNotInBook = 0;

            LOG(( "  playing from book!\n" ));

            setMoveToPlay( m, 0, 0, 0, 0 );

            return 0;
        }
        else {
            numOfMovesNotInBook++;
        }
    }

    // Return immediately if only one move is possible!
    if( moveList.count == 1 ) {
        LOG(( "  playing only move!\n" ));

        setMoveToPlay( moveList.moves[0].move, 0, 0, 0, 0 );

        return 0;
    }

    int f = initial_score;

    // Start searching...
    for( int depth=3; depth <= maxdepth; depth++ ) {
        int i;

        // Clear node count for all moves in the list, as usually the node
        // count is not relevant across different search depths
        for( i=0; i<moveList.count; i++ ) {
            moveList.moves[i].nodes = 0;
        }

        int s_score = searchMTDf( pos, f, depth, moveList );

        if( searchMustBeInterrupted ) {
            Log::write( "  s: interrupted!\n" );

#if 1
            /*
                Perform a cleanup of the hash table, removing all PV's related
                to the current position and forcing into the table the last known good PV.
                
                If the search gets interrupted with a "bad" PV for the root position,
                it is possible that we are lead down that bad path when searching for the
                next move, because we examine the move from the hash table first, and
                the hash table is now "dirty" from the previous search.
                Sometimes the engine is able to refute this move quickly, but every now
                and then it will eventually play the hash move, which can be decent (although 
                not the best) or a downright blunder.
                To avoid this, we remove from the hash table all PV's for the current position,
                and then put back the PV for the best move, which was saved during the search.
            */
            for( i=0; i<moveList.count; i++ ) {
                LOG(( "Hash cleanup (%2d/%2d): %s... ", i+1, moveList.count, SAN::moveToText(pos, moveList.moves[i].move) ));

                Position p( pos );

                p.doMove( moveList.moves[i].move );

                while( true ) {
                    HashTable::Entry * entry = hashTable->probe(p);

                    if( entry == 0 )
                        break;

                    Move m = entry->getMove();

                    if( m == Move::Null )
                        break;

                    if( ! p.isValidMove( m ) )
                        break;

                    LOG(( "%s[%d/%d] ", SAN::moveToText( p, m ), entry->getValue(), entry->getDepth() ));

                    hashTable->clean( p );

                    if( p.doMove( m ) != 0 )
                        break;
                }

                LOG(( "done.\n" ));
            }

            hashTable->clean( pos );

            // Repopulate the hash table with the last known good PV
            LOG(( "Hash update... " ));

            Position p( pos );

            int moveScore = gameMoveToPlay.score;

            for( i=0; i<gameMoveToPlay.pvlen; i++ ) {
                Move m = gameMoveToPlay.pv[i];

                LOG(( "%s ", SAN::moveToText( p, m ) ));

                hashTable->store( p,
                    m,
                    moveScore,
                    0,
                    0 );

                moveScore = -moveScore; // Negate score as the side on move changes

                p.doMove( m );
            }

            LOG(( "done.\n" ));
#endif

            // Exit from the search
            break;
        }

        // Assign score
        f = s_score;

        // If a mate is found, exit now unless we must continue a mate variation
        // (if so we must keep searching until we get closer to the mate, otherwise
        // we run the risk of always finding a mate but never actually giving it!)
        if( state != state_Analyzing ) {
            if( f >= Score::MateHi ) {
                if( (initial_score < Score::MateHi) || (f > initial_score) )
                    break;
            }
            else if( f <= Score::MateLo ) {
                break;
            }
        }

        // Check for timeout
        if( isSearchOver() )
            break;
    }

    return f;
}

int Engine::negaMaxMT_AtRoot( Position & pos, int gamma, int depth, RootMoveList & moveList )
{
    // Exit now if no moves to play
    if( moveList.count == 0 ) {
        return pos.boardFlags & Position::SideToPlayInCheck ? -Score::Mate : 0;
    }

    // Initialize variables
    bool        failedHigh  = false;
    int         result      = Score::Min;
    Move        bestMove    = Move::Null;
    unsigned    totalNodes  = 0;
    UndoInfo    undoInfo( pos );

    // Search all valid moves at the current depth
    rootMoveStat.reset();

    rootMoveStat.moves_total = moveList.count;
    rootMoveStat.depth = depth / FullPlyDepth;

    for( int j=0; j<moveList.count; j++ ) {
        totalNodes += moveList.moves[j].nodes;
    }

    for( int i=0; i<moveList.count; i++ ) {
        Move move = moveList.moves[i].move;

        rootMoveStat.moves_remaining = moveList.count - 1 - i;
        rootMoveStat.current_move = move;

        // Play move and search it
        pos.doMove( move );

        unsigned nodes = Counters::callsToEvaluation + Counters::posSearched;

        int s = -negaMaxMT( pos, 1-gamma, depth-FullPlyDepth, 1 );

        // Take the move back to restore the start position
        pos.undoMove( move, undoInfo );

        if( searchMustBeInterrupted ) {
            break;
        }

        nodes = Counters::callsToEvaluation + Counters::posSearched - nodes;

        moveList.moves[i].nodes += nodes;
        moveList.moves[i].value = s;

        totalNodes += nodes;

        // Note: since result is initialized to "minus infinity", the test
        // below is always true for the first move
        if( s > result ) {
            bestMove = move;

            // Set result
            result = s;

            // Check if result is good enough to produce a cutoff
            if( result >= gamma ) {
                failedHigh = true;

                ++Counters::anyFailedHigh;

                if( i == 0 ) ++Counters::firstFailedHigh;
                if( i == 1 ) ++Counters::secondFailedHigh;

                break;
            }
        }
    }

    // If a move caused a fail high (or we don't have any move), bring it to the top
    if( failedHigh || (gameMoveToPlay.pvlen == 0) ) {
        int bestIndex = 0;

        while( moveList.moves[ bestIndex ].move != bestMove ) {
            bestIndex++;
        }

        if( bestIndex > 0 ) {
            RootMove temp = moveList.moves[ bestIndex ];

            while( bestIndex > 0 ) {
                moveList.moves[bestIndex] = moveList.moves[bestIndex-1];
                bestIndex--;
            }

            moveList.moves[0] = temp;
        }
    }

    /* 
        Sort moves according to their node count. 
        
        Usually, good moves are harder to refute and take a lot more nodes than 
        weak moves, so by sorting the list we're actually trying to bring good moves 
        closer to the top, where they'll be considered first should the current
        best move fail low.
    */
    if( haveRootMoveOrdering ) {
        // Note that caller expects best move to be on top of the list... 
        // don't touch that or it will be lost!
        for( int i=1; i<moveList.count-1; i++ ) {
            int k = i;

            for( int j=i+1; j<moveList.count; j++ ) {
                if( moveList.moves[j].nodes > moveList.moves[k].nodes ) {
                    k = j;
                }
            }

            if( k != i ) {
                RootMove temp = moveList.moves[k];

                while( k > i ) {
                    moveList.moves[k] = moveList.moves[k-1];
                    k--;
                }

                moveList.moves[i] = temp;
            }

        }
    }

    return result;
}
