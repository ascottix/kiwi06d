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
#include <string.h>

#include "counters.h"
#include "movehandler.h"
#include "position.h"
#include "score.h"

/*
    Move ordering based on the "Inside Rebel" paper by Ed Schroeder
    (author of Rebel and Pro Deo).
*/
const int   BonusMultiplier = 1000*1000;    

const int   BonusForWinningCapture      = 200 * BonusMultiplier;
const int   BonusForPromotionCapture    = 190 * BonusMultiplier;
const int   BonusForMajorPromotion      = 180 * BonusMultiplier;
const int   BonusForGoodCapture         = 170 * BonusMultiplier;
const int   BonusForKiller1             = 100 * BonusMultiplier;
const int   BonusForKiller2             =  90 * BonusMultiplier;
const int   BonusForMinorPromotion      =  80 * BonusMultiplier;
const int   BonusForCastling            =  70 * BonusMultiplier;

int  MoveHandler::tableHistoryBlack[64*64];
int  MoveHandler::tableHistoryWhite[64*64];
Move MoveHandler::tableKiller1[MaxKiller];
Move MoveHandler::tableKiller2[MaxKiller];

MoveHandler::MoveHandler( const Position & pos, int ply, int mode, Move hashMove )
    : pos_( pos ) 
{
    ply_ = ply;
    mode_ = mode;
    hashMove_ = hashMove;
    state_ = StateTryHashMove;
    historyTable_ = pos.sideToPlay == Black ? tableHistoryBlack : tableHistoryWhite;

    // Other variables are uninitialized for now... they will be set in the "Generate" state
}

void MoveHandler::restart( Move hashMove )
{
    moveIndex_ = 0;

    if( hashMove != Move::Null ) {
        move_[ moveCount_ ] = hashMove;
        moveWeight_[ moveCount_ ] = BonusForWinningCapture + 100000;
        moveCount_++;
    }

    state_ = StateReadNextMove;
}

int MoveHandler::getNextMove( Move & result )
{
    switch( state_ ) {

    case StateDone:
        // No more moves to try
        return 1;

    case StateTryHashMove:
        if( hashMove_ != Move::Null ) {
            // Try this move first
            result = hashMove_;
            state_ = StateGenerateMoves;
            return 0;
        }

        /* ...else fall thru generation code... */
    case StateGenerateMoves:
        {
            moveIndex_ = 0;
            moveCount_ = 0;

            MoveList tmp;

            bool selectForQuiesce = (mode_ == GenerateForQuiesce) && ! pos_.isSideToMoveInCheck();

            if( pos_.isSideToMoveInCheck() ) {
                pos_.generateCheckEscapes( tmp );
            }
            else if( mode_ == GenerateForQuiesce ) {
                pos_.generateTactical( tmp ); 
            }
            else {
                pos_.generateMoves( tmp );
            }

            // Assign a weight to each move and copy it in the main array
            for( int i=0; i<tmp.count(); i++ ) {
                Move m = tmp.get( i );

                if( m != hashMove_ ) {
                    int moved = pos_.board.piece[ m.getFrom() ];
                    int captured = pos_.board.piece[ m.getTo() ];
                    int promoted = m.getPromoted();
                    int weight = 0;

                    if( captured != None ) {
                        // Move is a capture
                        weight = Score::PieceAbs[ captured ] - Score::PieceAbs[ moved ];

                        if( weight < 0 ) {
                            weight = pos_.evaluateExchange( m.getFrom(), m.getTo() );

                            // Prune losing captures if generating moves for the quiesce search
                            if( selectForQuiesce && (weight < 0) ) {
                                discardedMoves_.add( m );
                                continue;
                            }
                        }

                        if( weight > 0 ) {
                            weight += BonusForWinningCapture;
                        }
                        else {
                            if( PieceType(promoted) == Queen ) {
                                weight += BonusForPromotionCapture;
                            }
                            else if( weight == 0 ) {
                                weight += BonusForGoodCapture;
                            }
                        }

                    }
                    else if( promoted != None ) {
                        // Move is a promotion
                        if( PieceType(promoted) == Queen ) {
                            weight += BonusForMajorPromotion;
                        }
                        else {
                            if( selectForQuiesce && (PieceType(m.getPromoted()) != Knight) ) {
                                discardedMoves_.add( m );
                                continue;
                            }

                            weight += BonusForMinorPromotion + Score::PieceAbs[ promoted ];
                        }
                    }
                    else {
                        if( m == tableKiller1[ ply_ ] ) {
                            weight += BonusForKiller1;
                        }
                        else if( m == tableKiller2[ ply_ ] ) {
                            weight += BonusForKiller2;
                        }
                        else {
                            weight += historyTable_[ m.toUint12() ] >> 16;
                        }
                    }

                    // Adjust weight with piece/square information too
                    const char * psq = Score::ByPiece_Opening[ moved ];

                    if( psq != 0 ) {
                        weight += psq[ m.getTo() ];
                        weight -= psq[ m.getFrom() ];
                    }

                    // Add move to the list
                    move_[ moveCount_ ] = m;
                    moveWeight_[ moveCount_ ] = weight;
                    moveCount_++;
                }
                // ...else skip, as hash move was already considered
            }
        }

        state_ = StateReadNextMove;

        /* ...fall thru to get a capture move... */
    case StateReadNextMove:
        if( moveIndex_ < moveCount_ ) {
            // Search the best move in the list
            int i = moveIndex_;

            for( int j=i+1; j<moveCount_; j++ ) {
                if( moveWeight_[ j ] > moveWeight_[ i ] ) {
                    i = j;
                }
            }

            result = move_[ i ];
            
            if( i != moveIndex_ ) {
                move_[ i ] = move_[ moveIndex_ ];
                moveWeight_[ i ] = moveWeight_[ moveIndex_ ];
            }

            moveIndex_++;

            return 0;
        }
        else {
            state_ = StateDone;
            return 1;
        }
    }

    return 1;
}

void MoveHandler::resetHistoryTable()
{
    memset( tableHistoryBlack, 0, sizeof(tableHistoryBlack) );
    memset( tableHistoryWhite, 0, sizeof(tableHistoryBlack) );
}

void MoveHandler::updateHistoryTable( int side, const Move & m, int delta )
{
    int index = m.toUint12();
    int * tableHistory = side == Black ? tableHistoryBlack : tableHistoryWhite;

    tableHistory[ index ] += delta*delta;

    // Scale a move down if it's growing too much, to give other moves a chance!
    if( tableHistory[ index ] & 0x40000000 ) {
        tableHistory[ index ] >>= 1;
    }
}

void MoveHandler::resetKillerTable()
{
    for( int i=0; i<MaxKiller; i++ ) {
        tableKiller1[i] = Move::Null;
        tableKiller2[i] = Move::Null;
    }
}

void MoveHandler::addToKillerTable( const Move & m, int ply )
{
    Move    killer = tableKiller1[ply];

    if( killer != m ) {
        if( killer != Move::Null ) {
            // Move this entry into the next slot
            tableKiller2[ply] = killer;
        }
        tableKiller1[ply] = m;
    }
}
