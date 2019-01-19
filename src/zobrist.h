/*
    Kiwi
    64-bit random number tables for Zobrish hash

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
#ifndef ZOBRIST_H_
#define ZOBRIST_H_

#include "bitboard.h"

struct Zobrist
{
    static BitBoard BlackPawn[64];
    static BitBoard BlackKnight[64];
    static BitBoard BlackBishop[64];
    static BitBoard BlackRook[64];
    static BitBoard BlackQueen[64];
    static BitBoard BlackKing[64];
    static BitBoard BlackCastleKing;
    static BitBoard BlackCastleQueen;
    static BitBoard WhitePawn[64];
    static BitBoard WhiteKnight[64];
    static BitBoard WhiteBishop[64];
    static BitBoard WhiteRook[64];
    static BitBoard WhiteQueen[64];
    static BitBoard WhiteKing[64];
    static BitBoard WhiteCastleKing;
    static BitBoard WhiteCastleQueen;
    static BitBoard EnPassant[64];

    static void initialize();
};

#define ZobSideToPlay Zobrist::EnPassant[0]

#endif // ZOBRIST_H_
