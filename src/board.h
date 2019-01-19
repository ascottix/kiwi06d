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
#ifndef BOARD_H_
#define BOARD_H_

#include <stdio.h>

#include "bitboard.h"

extern BitBoard SetTA1H8[64];
extern BitBoard SetDA1H8[64];
extern BitBoard SetDA8H1[64];

extern int boardTA1H8[64];
extern int boardDA1H8[64];
extern int shiftDA1H8[64];
extern int boardDA8H1[64];
extern int shiftDA8H1[64];

enum Side
{
    Black = 0,
    White = 1
};

enum
{
    PieceTypeMask   = 0x0E,
    PieceSideMask   = 0x01
};

#define PieceType( p )      ((p) & PieceTypeMask)
#define PieceSide( p )      ((p) & PieceSideMask)
#define OppositeSide( p )   ((p) ^ PieceSideMask)

enum Piece
{
    None    = 0 << 1,
    Pawn    = 1 << 1,
    Knight  = 2 << 1,
    Bishop  = 3 << 1,
    Rook    = 4 << 1,
    Queen   = 5 << 1,
    King    = 6 << 1,
    //
    BlackPawn   = Black | Pawn,
    BlackKnight = Black | Knight,
    BlackBishop = Black | Bishop,
    BlackRook   = Black | Rook,
    BlackQueen  = Black | Queen,
    BlackKing   = Black | King,
    //
    WhitePawn   = White | Pawn,
    WhiteKnight = White | Knight,
    WhiteBishop = White | Bishop,
    WhiteRook   = White | Rook,
    WhiteQueen  = White | Queen,
    WhiteKing   = White | King
};

enum Square
{
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8
};

#define FileOfSquare( sq )                  ((sq) &  7)
#define RankOfSquare( sq )                  ((sq) >> 3)
#define FileRankToSquare( file, rank )      ((file) + ((rank) << 3))

/**
    This structure is simply an array of 64 integers that
    represent the standard 8x8 board, arranged as per the
    Square enumeration above.
*/
struct Board
{
    signed char piece[64];

    // Removes all pieces from the board
    void clear() { 
        for( int i=0; i<64; i++ ) piece[i] = None; 
    }
    
    // Checks two boards for equality
    int operator == ( const Board & ) const;

    // Dumps the board to the log file
    void dump( const char * header = 0 ) const;

    static char * getSquareName( char * name, int square );

    static const char * squareName( int square );

    // Initialization
	static void initialize();
};

#endif // BOARD_H_
