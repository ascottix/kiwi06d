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
#ifndef POSITION_ENUM_H_
#define POSITION_ENUM_H_

#include "board.h"
#include "position.h"

/**
    Note: adding a piece when the enumeration has already started is not supported.
*/
class PositionEnumerator
{
public:
    PositionEnumerator();

    /** Adds a piece to the enumeration, with position limits. Returns 0 on success, otherwise an error code. */
    int addPiece( int piece, int min_pos, int max_pos );

    /** Adds a piece to the enumeration. Returns 0 on success, otherwise an error code. */
    int addPiece( int piece );

    void clear();

    void gotoFirstPosition();

    /** Returns true if there are more positions to be enumerated, false otherwise. */
    bool hasMorePositions() const {
        return piece_[0].cur_pos_ >= 0;
    }

    bool isValidPosition( bool wtm );

    /** Gets the current position. Returns true if the position is valid, false otherwise. */
    bool getCurrentPosition( Position & pos, bool wtm );

    /** Moves to the next position. Returns false if there are no more positions. */
    bool gotoNextPosition();

    /** Returns the number of pieces in this enumeration. */
    int getPieceCount() const {
        return pieceCount_;
    }

    /** 
        Returns the position of the specified piece, where:
            0 = white king
            1 = black king
        and user added pieces get index numbers 2 and greater.

        Returns a negative number on error (e.g. index out of bounds).
    */
    int getPiecePos( int index ) const;

    /** Returns the specified piece. */
    int getPiece( int index ) const;

    /** Returns the position of the white king. */
    int getWhiteKingPos() const {
        return piece_[0].cur_pos_;
    }

    /** Returns the position of the black king. */
    int getBlackKingPos() const {
        return piece_[1].cur_pos_;
    }

private:
    enum {
        MaxPieceCount = 7
    };

    struct PieceData {
        int piece_;
        int min_pos_;
        int max_pos_;
        int cur_pos_;
    };

    int pieceCount_;
    PieceData piece_[ MaxPieceCount ];
};

#endif // POSITION_ENUM_H_
