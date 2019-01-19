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
#ifndef SAN_H_
#define SAN_H_

#include "move.h"

class Position;

enum {
    sanOk = 0,
    sanMalformedMoveText,
    sanAmbiguousMove,
    sanInvalidMove,
};

/**
    Converts a move to and from Standard Algebraic Notation text format.
*/
class SAN
{
public:
    /** Returns a piece given its name. */
    static int  getPieceByName( char n );

    /** Returns the name of the specified piece. */
    static char getNameOfPiece( int piece );

    /** Sets the name of a piece, e.g. to work with a different language. */
    static void setNameOfPiece( char n, int piece );

    /**
        Converts a SAN move text into a Move object (useable by the engine).

        Note: if files are expressed with lowercase letters then this function
        will also read long algebraic notation moves, for example "e2e4" or 
        "h7h8q" should parse ok, but "B1C2" does not.

        @param move pointer to destination move object (may be null)
        @param pos position where the move is played
        @param text move text

        @return 0 on success, otherwise an error code
    */
    static int textToMove( Move * move, const Position & pos, const char * text );

    /**
        Converts a move into a SAN text string.

        Note: the destination buffer must be long enough to accomodate for the 
        move text. The maximum length of a string produced by this function 
        is 7 characters (e.g. "Bc3xe5+") excluding the terminating null.

        @param move pointer to destination text buffer
        @param pos position where the move is played
        @param move move to convert

        @return 0 on success, otherwise an error code
    */
    static int moveToText( char * text, const Position & pos, Move move );

    /**
        Converts a move into a SAN text string.

        This function is not thread-safe as it uses a static buffer for
        storing the result. It's quite handy though.
    */
    static const char * moveToText( const Position & pos, Move move );

    /**
        Converts a move into a LAN (Long Algebraic Notation) text string.

        Note: the destination buffer must be long enough to accomodate for the 
        move text. The maximum length of a string produced by this function 
        is 5 characters (e.g. "h2h1q") excluding the terminating null.

        @param move pointer to destination text buffer
        @param pos position where the move is played
        @param move move to convert

        @return 0 on success, otherwise an error code
    */
    static int moveToTextLong( char * text, const Position & pos, Move move );

private:
    // Piece names: they default to the English names but can
    // be changed if needed to parse different languages
    static char nameOfKnight;
    static char nameOfBishop;
    static char nameOfRook;
    static char nameOfQueen;
    static char nameOfKing;
};

#endif // SAN_H_
