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
#include <stdlib.h>

#include "book.h"
#include "engine.h"
#include "log.h"
#include "mask.h"
#include "pawnhash.h"
#include "pgn_lex.h"
#include "pgn.h"
#include "random.h"
#include "recognizer.h"
#include "san.h"
#include "score.h"
#include "undoinfo.h"

void testRecognizers(); // Defined in recognizer_test.cxx

static void dumpPosition( const Position & pos )
{
    pos.dump();

    char fen[200];

    pos.getBoard( fen );

    Log::write( "  FEN : %s\n", fen );

    int eval = pos.getEvaluation();

    Log::write( "  Static evaluation: %d\n", eval );
}

/*
    Check whether a game is ended in the current position.
*/
bool Engine::isGameEnded()
{
    bool result = false;

    // Check for end of game by forced draw or checkmate
    if( gamePosition.getHalfMoveClock() >= 100 ) {
        Log::write( "playMove: draw by 50 moves rule\n" );

        interfaceAdapter->sendResult( 0, "draw by 50 moves rule" );

        result = true;
    }
    else if( rep3History[ gameHistoryIdx ].repCount == 2 ) {
        Log::write( "playMove: draw by 3-fold repetition\n" );

        interfaceAdapter->sendResult( 0, "draw by repetition" );

        result = true;
    }
    else {
        /*
            Check for draw by insufficient material by FIDE rule (9.6):
            
            "The game is drawn when a position is reached from which a checkmate cannot
            occur by any possible series of legal moves, even with the most unskilled play."

            We handle the following cases:
        
            1. K+n*M vs. K+m*M  where M is a minor piece (knight or bishop) and 0 <= n, m <= 1
            2. K+n*B vs. K+m*B  where all bishops stand on the same color (n > 0, m >= 0)

            Note that although K+N+N can't force mate it's still not draw by the FIDE rules,
            because a mate is technically possible.
        */
        if( (gamePosition.numOfBlackPawns() + gamePosition.numOfWhitePawns()) == 0 ) {
            int wp = gamePosition.numOfWhitePieces();
            int bp = gamePosition.numOfBlackPieces();

            if( ( wp <= 1 )&&( bp <= 1 ) ) {
                // Either side has at most one piece: if it's minor, it's draw
                result = gamePosition.blackQueensRooks.isEmpty() && gamePosition.whiteQueensRooks.isEmpty();
            }
            else {
                // At least one side has more than one piece, it's draw only in case 2. above
                BitBoard whiteQueens = gamePosition.whiteQueensBishops & gamePosition.whiteQueensRooks;
                BitBoard whiteBishops= gamePosition.whiteQueensBishops ^ whiteQueens;

                BitBoard blackQueens = gamePosition.blackQueensBishops & gamePosition.blackQueensRooks;
                BitBoard blackBishops= gamePosition.blackQueensBishops ^ blackQueens;

                BitBoard allBishops         = whiteBishops | blackBishops;

                BitBoard allBishopsAndKings = allBishops |
                    BitBoard::Set[ gamePosition.whiteKingSquare ] |
                    BitBoard::Set[ gamePosition.blackKingSquare ];

                if( allBishopsAndKings == gamePosition.allPieces ) {
                    // There are only bishops in this position... are they all on the same color?
                    BitBoard darkBishops = allBishops & Mask::DarkSquares;
                    BitBoard liteBishops = allBishops & Mask::LiteSquares;

                    if( (darkBishops == allBishops) || (liteBishops == allBishops) ) {
                        result = true;
                    }
                }
            }
        }

        if( result ) {
            Log::write( "playMove: draw by insufficient material\n" );

            interfaceAdapter->sendResult( 0, "draw by insufficient material" );
        }
        else {
            // Verify if it's a checkmate or stalemate position
            MoveList all;

            gamePosition.generateValidMoves( all );

            if( all.count() == 0 ) {
                // There are no valid moves... is it checkmate or stalemate?
                if( gamePosition.boardFlags & Position::SideToPlayInCheck ) {
                    // Checkmate
                    Log::write( "playMove: checkmate, result=%s\n", gamePosition.sideToPlay == Black ? "1-0" : "0-1" );
                
                    interfaceAdapter->sendResult( gamePosition.sideToPlay == Black ? +1 : -1, "checkmate" );

                    result = true;
                }
                else {
                    // Stalemate
                    Log::write( "playMove: stalemate\n" );

                    interfaceAdapter->sendResult( 0, "stalemate" );

                    result = true;
                }
            }
        }
    }

    return result;
}

// Note: returns a static buffer (not good for multithreading!)
static const char * moveToText( const Position & pos, Move move )
{
    static char buf[9];

    SAN::moveToText( buf, pos, move );

    return buf;
}

int Engine::playMove( Move move, MoveInfo * info )
{
    Log::write( "playMove: %s\n", moveToText( gamePosition, move ) );

    int result = gamePosition.doMove( move );

    if( result == 0 ) {
        // Update game history
        gameHistoryIdx++;
        gameHistory[ gameHistoryIdx ] = gamePosition;
        
        if( info != 0 ) {
            moveHistory[ gameHistoryIdx ] = *info;
        }
        else {
            moveHistory[ gameHistoryIdx ].reset();
            moveHistory[ gameHistoryIdx ].pvlen = 1;
            moveHistory[ gameHistoryIdx ].pv[0] = move;
        }

        // Update 3-position repetition history
        rep3History[ gameHistoryIdx ].hashCode = gamePosition.hashCode;
        rep3History[ gameHistoryIdx ].repCount = 0;

        int n = gameHistoryIdx - 4;
        int e = gameHistoryIdx - gamePosition.getHalfMoveClock();

        while( n >= e ) {
            if( rep3History[ gameHistoryIdx ].hashCode == rep3History[ n ].hashCode ) {
                rep3History[ gameHistoryIdx ].repCount++;
            }

            n -= 2;
        }
    }

    return result;
}

int Engine::undoMove( int numOfHalfMoves )
{
    while( numOfHalfMoves > 0 ) {
        if( gameHistoryIdx > 0 ) {
            gameHistoryIdx--;
            gamePosition = gameHistory[ gameHistoryIdx ];
        }

        numOfHalfMoves--;
    }

    gamePosition.dump();

    ponderMove = Move::Null;

    hashTable->reset();

    return 0;
}

int Engine::resetBoard( const char * fen )
{
    // Cleanup hash tables
    hashTable->reset();
    pawnHashTable->reset();

    Position pos;

    if( fen != 0 ) {
        pos.setBoard( fen );
    }
    else {
        pos.setBoard( Position::startPosition );
    }

    pos.getEvaluation(); // Only useful for debugging

    gamePosition = pos;
    gameHistoryIdx = 0;
    gameHistory[ 0 ] = pos;
    moveHistory[ 0 ].reset();
    rep3History[ 0 ].hashCode = pos.hashCode;
    rep3History[ 0 ].repCount = 0;

    numOfMovesNotInBook = 0;

    ponderMove = Move::Null;

    return 0;
}

int Engine::handleOpponentMove( const char * moveAsText )
{
    Log::write( "handleOpponentMove: %s\n", moveAsText );

    Move move;

    Position pos( gamePosition );

    if( ponderMove != Move::Null ) {
        // Since we already guessed a reply, this move refers to the previous position
        pos = gameHistory[ gameHistoryIdx - 1 ];
    }

    int res = SAN::textToMove( &move, pos, moveAsText );

    if( (res != 0) || (move == Move::Null) ) {
        // Illegal move
        interfaceAdapter->rejectMove( moveAsText, 0 );

        Log::write( "*** handleOpponentMove: cannot parse move: %s -> %d\n", moveAsText, res );

        return -1;
    }
    else {
        if( move == Move::Null ) {
            Log::write( "*** handleOpponentMove: serious trouble... move is NULL!!! %s\n", res );

            return -1;
        }

        if( state == state_PonderComplete || state == state_Waiting ) {
            if( ponderMove != Move::Null ) {
                // If we are here, then the ponder search has terminated already
                // (e.g. a forced mate was found)
               Log::write( "*** HANDLING PONDER COMPLETE: PONDER = %s\n", move == ponderMove ? "HIT" : "MISS" );

               undoMove( 1 );
            }

            playMove( move );

            state = state_Thinking;
        }
        else if( state == state_Pondering ) {
            // Usually next state is thinking, except when missing a ponder move
            state = state_Thinking;

            if( move == ponderMove ) {
                Log::write( "*** PONDER HIT, entering thinking state (search time = %d)\n", timeSpentInSearch() );

                setTimeTargetForMove();

                if( isTimeOut() ) {
                    searchMustBeInterrupted = true;
                }

                dumpPosition( gamePosition );
            }
            else {
                if( ponderMove != Move::Null ) {
                    Log::write( "*** PONDER MISS, taking back move and entering ponder missed state\n" );

                    undoMove( 1 );

                    state = state_PonderMissed;

                    searchMustBeInterrupted = true;
                }

                playMove( move );
            }
        }
        else if( state == state_Analyzing || state == state_AnalysisComplete ) {
            // Play the move and stop the current search, but stay in analyzing mode
            state = state_Analyzing;

            searchMustBeInterrupted = true;

            playMove( move );
        }
        else if( state == state_Observing ) {
            // Just play the move
            playMove( move );
        }
        else {
            // Command is unexpected in this state
            interfaceAdapter->rejectMove( moveAsText, "user move unexpected at this time" );
        }
    }

    ponderMove = Move::Null;

    return 0;
}

int Engine::handleThinkingComplete()
{
    Log::write( "think: complete, handling result...\n" );

    if( state == state_Thinking ) {
        // Output the engine move
        Move m = gameMoveToPlay.pv[0];

        if( m == Move::Null ) {
            Log::write( "think: no best move to play!!!\n" );

            state = state_Observing;

            // At least be nice to the other player: resign game
            interfaceAdapter->resignGame( gamePosition, "engine doesn't have a move to play" );
        }
        else {
            Log::write( "*** Time spent in search: %d\n", timeSpentInSearch() );

            // Make a copy of the current board
            Position    pos( gamePosition );

            // Play the move and enter the pondering state
            state = ponderingEnabled ? state_Pondering : state_Waiting;

            if( showThinking ) {
                interfaceAdapter->showThinking( gamePosition, gameMoveToPlay );
            }

            playMove( m, &gameMoveToPlay );

            // Update clock and time manager
            timeManager.goNextMove();

            // Check for draw by repetition and by 50-moves rule: those must be claimed
            // before making the move
            if( gamePosition.getHalfMoveClock() >= 100 ) {
                // TODO: claim draw by 50 moves rule
            }
            else if( rep3History[ gameHistoryIdx ].repCount == 2 ) {
                // TODO: claim draw by 3-fold repetition
            }

            // Tell the move to the opponent
            interfaceAdapter->playMove( pos, m );

            // If game ended, enter observing state
            if( isGameEnded() ) {
                Log::write( "think: game ended, entering observing state!\n" );
                state = state_Observing;
            }
            else {
                // Resign a lost game or announce mate if winning
                int score = gameMoveToPlay.score;

                // Resign if score was very low and not getting better for the last few moves
                if( (gameHistoryIdx >= 5) && (score <= resignThreshold) ) {
                    int s0 = score;
                    int s1 = moveHistory[ gameHistoryIdx-2 ].score;
                    int s2 = moveHistory[ gameHistoryIdx-4 ].score;

                    if( (s2 <= resignThreshold) && (s1 <= s2) && (s0 <= s1) ) {
                        // Resign
                        Log::write( "think: resigning with scores s0=%d, s1=%d, s2=%d\n",
                            score,
                            s1,
                            s2 );

                        interfaceAdapter->resignGame( gamePosition, 0 );

                        state = state_Observing;
                    }
                }

                // Announce "mate in n" if found
                if( score > Score::MateHi ) {
                    int n = 1 + (Score::Mate - score) / 2;

                    if( n > 1 ) {
                        Log::write( "think: mate in %d\n", n );
                    }

                    // For now, this avoids strange situations where a
                    // move from the hash table brings us farther from the mate:
                    // if that happens that the path to mate is usually completely
                    // lost!!!
                    // hashTable->reset(); // TODO: this from an ancient version, is it still true?!?!
                }
            }
        }

        Recognizer::dumpStats();
    }
    else {
        // ...else we changed state because of some external input, ignore
        Log::write( "think: not in thinking state, ignoring result\n" );
    }

    return 0;
}

void Engine::initializeSearch()
{
    setTimeTargetForMove();

    searchStartTime = System::getTickCount();

    searchMustBeInterrupted = false;

    gameMoveToPlay.reset();

    ponderMove = Move::Null;

    Recognizer::clearStats();
}

unsigned Engine::getBookMoves( Book & book, const Position & p, MoveList & moves, unsigned * weights )
{
    unsigned    totalWeight = 0;

    moves.reset();

    Position pos( p ); // Make a work copy of the original position

    Book::Entry * root = book.lookup( pos );

    if( root != 0 ) {
        int i;
        int count = 0;

        // Get the list of book moves
        MoveList    moveList;

        pos.generateValidMoves( moveList );

        UndoInfo undoInfo( pos );

        for( i=0; i<moveList.count(); i++ ) {
            pos.doMove( moveList.move[i] );

            Book::Entry * e = book.lookup( pos );

            pos.undoMove( moveList.move[i], undoInfo );

            if( e != 0 ) {
                // Position found... make sure the move didn't occur in the past history though
                bool valid = true;

                for( int idx=0; idx<=gameHistoryIdx; idx++ ) {
                    Move m = moveHistory[idx].pv[0];

                    if( m.isCaptureOrPromotion() ) {
                        break;
                    }

                    Move x( m.getTo(), m.getFrom() );

                    if( x == moveList.move[i] ) {
                        Log::write( "book: %s skipped (move reversal)\n", moveList.move[i].toString() );
                        valid = false;
                        break;
                    }
                }
                
                // Save move and info
                if( valid ) {
                    moves.add( moveList.move[i] );

                    if( weights != 0 ) {
                        weights[ count ] = e->count;
                    }

                    count++;

                    totalWeight += e->count;
                }
            }
        }

        // Sort the move list
        if( weights != 0 ) {
            for( i=0; i<count-1; i++ ) {
                int m = i;

                for( int j=i+1; j<count; j++ ) {
                    if( weights[ j ] > weights[ m ] ) {
                        m = j;
                    }
                }

                if( m != i ) {
                    unsigned x = weights[i];
                    weights[i] = weights[m];
                    weights[m] = x;

                    Move y = moves.move[i];
                    moves.move[i] = moves.move[m];
                    moves.move[m] = y;
                }
            }
        }
    }

    return totalWeight;
}

Move Engine::getBookMove( Book & book, const Position & p )
{
    Move result = Move::Null;

    // Get the book moves
    MoveList    bookMoves;
    unsigned    bookMovesWeight[ MoveList::MaxMoveCount ];
    
    unsigned    totalWeight = getBookMoves( book, p, bookMoves, bookMovesWeight );

    int         bookMovesCount = bookMoves.count();

    // If move list is not empty, select a random move according to the move weights
    if( bookMovesCount > 0 ) {
        unsigned bitMask = 1;

        while( bitMask < totalWeight ) {
            bitMask = (bitMask << 1) | 1;
        }

        unsigned weight;

        do {
            weight = rand() & bitMask;
        } while( weight > totalWeight );

        for( int i=0; i<bookMovesCount; i++ ) {
            if( weight <= bookMovesWeight[i] ) {
                result = bookMoves.get( i );
            }

            weight -= bookMovesWeight[i];
        }
    }

    return result;
}

void Engine::displayBookInfo()
{
    // Get the book moves
    MoveList    bookMoves;
    unsigned    bookMovesWeight[ MoveList::MaxMoveCount ];
    
    unsigned    totalWeight = getBookMoves( *openingBook, gamePosition, bookMoves, bookMovesWeight );

    int         bookMovesCount = bookMoves.count();

    // Display the move list
    if( bookMovesCount > 0 ) {
        for( int i=0; i<bookMovesCount; i++ ) {
            double moveFrequency = 100.0 * (double) bookMovesWeight[ i ] / totalWeight;

            printf( " %s\tcount=%5u, frequency=%5.2f%%\n", 
                SAN::moveToText( gamePosition, bookMoves.get(i) ), 
                bookMovesWeight[ i ],
                moveFrequency );
        }
    }
    else {
        printf( " Book has no moves for the current position.\n" );
    }

    printf( "\n" );
}

void Engine::think()
{
    Log::write( "Entering think()\n" );

    dumpPosition( gamePosition );

    // Initialize search parameters
    initializeSearch();

    hashTable->bumpSearchId();

    int depth = (searchMode == mode_FixedDepth) ? fixedSearchDepth : MaxSearchPly;
    int score = 0;

    if( (gameHistoryIdx >= 2) && (moveHistory[ gameHistoryIdx-1 ].pvlen > 0) ) {
        score = moveHistory[ gameHistoryIdx-1 ].score;

        Log::write( "think: starting from last known score: %d\n", score );
    }

    // Make a copy of the root position, as it is passed by reference to all search
    // functions and there may be problems if we must process an asynchronous command
    // that requires changing the position while searching
    Position pos( gamePosition );

    // Search 
    searchPosition( pos, score, depth );

    // Handle end of search
    handleThinkingComplete();
}

void Engine::ponder()
{
    Log::write( "entering: ponder\n" );

    // Initialize
    initializeSearch();

    // Find a move to ponder
    if( moveHistory[ gameHistoryIdx ].pvlen > 1 ) {
        Log::write( "ponder: fetching move from PV\n" );

        ponderMove = moveHistory[ gameHistoryIdx ].pv[ 1 ];
    }
    else {
        Log::write( "ponder: performing shallow search\n" );

        Position pos( gamePosition );
    
        // Disable input checking in this part, as during this short search
        // the board won't be in a correct state
        inputCheckWhileSearching = false;

        searchPosition( pos, 0, 4 );

        inputCheckWhileSearching = true;

        ponderMove = gameMoveToPlay.pv[ 0 ];
    }

    if( ponderMove != Move::Null ) {
        Log::write( "ponder: move is %s\n", moveToText( gamePosition, ponderMove ) );

        if( showThinking ) {
            interfaceAdapter->sendHint( gamePosition, ponderMove );
        }

        playMove( ponderMove );

        // Now that we have executed the ponder move, start thinking for a reply...
        Position pos( gamePosition );

        int depth = (searchMode == mode_FixedDepth) ? fixedSearchDepth : MaxSearchPly;

        int score = searchPosition( pos, 0, depth );

        if( state == state_Pondering ) {
            Log::write( "ponder: done, reply=%s, score=%d\n", moveToText(gamePosition,gameMoveToPlay.pv[0]), score );

            state = state_PonderComplete;
        }
        else {
            Log::write( "ponder: search terminated, but no longer in pondering state...\n" );

            handleThinkingComplete();
        }
    }
    else {
        Log::write( "ponder: can't find a move to ponder!\n" );

        state = state_Waiting;
    }
}

void Engine::analyze()
{
    Log::write( "analyze: starting...\n" );

    dumpPosition( gamePosition );

    initializeSearch();

    int depth = (searchMode == mode_FixedDepth) ? fixedSearchDepth : MaxSearchPly;
    int score = 0;

    Position pos( gamePosition );

    searchPosition( pos, score, depth );

    if( state == state_Analyzing && ! searchMustBeInterrupted ) {
        state = state_AnalysisComplete;
    }

    Recognizer::dumpStats();

    // Cleanup hash tables on exit, as by default neither think() nor ponder() do it
    hashTable->reset();
    pawnHashTable->reset();
}

static void printBB( const char * name, const BitBoard & bb )
{
    printf( "%s=0x%016" PRIx64 "\n", name != 0 ? name : "", bb.data );
}

int Engine::main( Adapter * adapter )
{
    bool done = false;

    // Initialize variables
    Log::assign( "kiwi_log.txt" );

    Engine::initialize();

    // testRecognizers();

    // TODO: the adapter desing is only half-baked because the control
    // loop below should be part of it, not of the engine
    interfaceAdapter = adapter;

    state = state_Observing;

    // Main loop
    Log::write( "\nEngine started: entering main loop...\n\n" );

    while( ! done ) {
        // Think if necessary
        switch( state ) {
            case state_Thinking:
                think();
                break;
            case state_Pondering:
                ponder();
                break;
            case state_PonderMissed:
                // Go directly to the thinking state
                state = state_Thinking;
                break;
            case state_Analyzing:
                analyze();
                break;
            case state_Quitting:
                done = true;
                break;
            default:
                System::yield();
                break;
        }

        // Handle input
        while( System::isInputAvailable() ) {
            int r = handleInput();

            if( r != 0 ) {
                done = true;
                break;
            }
        }
    }

    return 0;
}
