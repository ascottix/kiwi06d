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
#include <assert.h>

#include "metrics.h"
#include "position_enum.h"

PositionEnumerator::PositionEnumerator()
{
    pieceCount_ = 0;

    addPiece( WhiteKing, A1, H8 );
    addPiece( BlackKing, A1, H8 );
}

int PositionEnumerator::addPiece( int piece, int min_pos, int max_pos )
{
    int result = -1;

    if( (pieceCount_ < MaxPieceCount) && 
        (((piece != BlackKing) && (piece != WhiteKing)) || (pieceCount_ < 2)) && 
        (min_pos <= max_pos) && 
        (min_pos >= A1) && (min_pos <= H8) &&
        (max_pos >= A1) && (max_pos <= H8) ) 
    {
        piece_[ pieceCount_ ].piece_ = piece;
        piece_[ pieceCount_ ].min_pos_ = min_pos;
        piece_[ pieceCount_ ].max_pos_ = max_pos;
        piece_[ pieceCount_ ].cur_pos_ = min_pos;

        pieceCount_++;

        result = 0;
    }
    else {
        assert( 0 );
    }

    return result;
}

int PositionEnumerator::addPiece( int piece )
{
    int pmin = A1;
    int pmax = H8;

    if( PieceType(piece) == Pawn ) {
        pmin = A2;
        pmax = H7;
    }

    return addPiece( piece, pmin, pmax );
}

void PositionEnumerator::gotoFirstPosition()
{
    for( int i=0; i<pieceCount_; i++ ) {
        piece_[ i ].cur_pos_ = piece_[ i ].min_pos_;
    }
}

void PositionEnumerator::clear()
{
    pieceCount_ = 2;    // Remove anything but the kings

    gotoFirstPosition();
}

bool PositionEnumerator::isValidPosition( bool wtm )
{
    // Check king distance
    int wk = piece_[0].cur_pos_;
    int bk = piece_[1].cur_pos_;

    if( distance( wk, bk ) <= 1 ) {
        return false;
    }

    // Make sure that pieces do not overlap (special case for common 3 and 4 pieces)
    if( pieceCount_ >= 3 ) {
        if( (piece_[2].cur_pos_ == wk) || (piece_[2].cur_pos_ == bk) ) {
            return false;
        }

        if( pieceCount_ >= 4 ) {
            if( (piece_[3].cur_pos_ == wk) || (piece_[3].cur_pos_ == bk) || (piece_[3].cur_pos_ == piece_[2].cur_pos_) ) {
                return false;
            }

            if( pieceCount_ >= 5 ) {
                for( int i=2; i<pieceCount_; i++ ) {
                    for( int j=0; j<pieceCount_; j++ ) {
                        if( (i != j) && (piece_[i].cur_pos_ == piece_[j].cur_pos_) ) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    int target = wtm ? bk : wk;
    int attacker_type = wtm ? White : Black;

    bool unknown = false;

    // Is any king in check?
    for( int i=2; i<pieceCount_; i++ ) {
        if( PieceSide( piece_[i].piece_ ) == attacker_type ) {
            int sq = piece_[i].cur_pos_;

            switch( piece_[i].piece_ ) {
            case BlackPawn:
                if( Attacks::BlackPawn[sq].getBit(target) ) {
                    return false;
                }
                break;
            case WhitePawn:
                if( Attacks::WhitePawn[sq].getBit(target) ) {
                    return false;
                }
                break;
            case BlackKnight:
            case WhiteKnight:
                if( Attacks::Knight[sq].getBit(target) ) {
                    return false;
                }
                break;
            case BlackBishop:
            case WhiteBishop:
                if( Attacks::Bishop[sq].getBit(target) == 0 ) {
                    return true;
                }
                unknown = true;
                break;
            case BlackRook:
            case WhiteRook:
                if( Attacks::Rook[sq].getBit(target) == 0 ) {
                    return true;
                }
                unknown = true;
                break;
            case BlackQueen:
            case WhiteQueen:
                if( Attacks::Bishop[sq].getBit(target) == 0 && Attacks::Rook[sq].getBit(target) == 0 ) {
                    return true;
                }
                unknown = true;
                break;
            }
        }
    }

    return true;
}

bool PositionEnumerator::getCurrentPosition( Position & pos, bool wtm )
{
    bool result = false;

    if( hasMorePositions() ) {
        // Ok to try this position, check that kings do not touch first
        if( distance( piece_[0].cur_pos_, piece_[1].cur_pos_ ) > 1 ) {
            // Setup board
            Board   board;
            bool    invalid = false;

            board.clear();

            for( int i=0; i<pieceCount_; i++ ) {
                // Check that pieces do not overlap
                if( board.piece[ piece_[i].cur_pos_ ] != None ) {
                    invalid = true;
                    break;
                }

                board.piece[ piece_[i].cur_pos_ ] = piece_[i].piece_;
            }

            if( ! invalid ) {
                // Assign board to position and make sure the side *not* on move is not in check
                pos.setBoard( board, wtm );

                result = wtm ? ! pos.isSideInCheck(Black) : ! pos.isSideInCheck(White);
            }
        }
    }

    return result;
}

bool PositionEnumerator::gotoNextPosition()
{
   // Advance to next position, this is similar to adding with carry...
    int i = 0;

    while( i < pieceCount_ ) {
        piece_[ i ].cur_pos_++;

        if( piece_[ i ].cur_pos_ <= piece_[ i ].max_pos_ ) {
            break;
        }

        piece_[ i ].cur_pos_ = piece_[ i ].min_pos_;

        i++;
    }

    // If we have done all positions, set a special flag to mean we're done
    if( i >= pieceCount_ ) {
        piece_[0].cur_pos_ = -1;
    }

    return hasMorePositions();
}

int PositionEnumerator::getPiecePos( int index ) const
{
    int result = -1;

    if( index >= 0 && index < pieceCount_ ) {
        result = piece_[ index ].cur_pos_;
    }

    assert( result >= 0 );

    return result;
}

int PositionEnumerator::getPiece( int index ) const
{
    int result = -1;

    if( index >= 0 && index < pieceCount_ ) {
        result = piece_[ index ].piece_;
    }

    assert( result >= 0 );

    return result;
}
