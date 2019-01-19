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
#include <stdio.h>
#include "random.h"
#include "zobrist.h"

BitBoard    Zobrist::BlackPawn[64];
BitBoard    Zobrist::BlackKnight[64];
BitBoard    Zobrist::BlackBishop[64];
BitBoard    Zobrist::BlackRook[64];
BitBoard    Zobrist::BlackQueen[64];
BitBoard    Zobrist::BlackKing[64];
BitBoard    Zobrist::BlackCastleKing;
BitBoard    Zobrist::BlackCastleQueen;
BitBoard    Zobrist::WhitePawn[64];
BitBoard    Zobrist::WhiteKnight[64];
BitBoard    Zobrist::WhiteBishop[64];
BitBoard    Zobrist::WhiteRook[64];
BitBoard    Zobrist::WhiteQueen[64];
BitBoard    Zobrist::WhiteKing[64];
BitBoard    Zobrist::WhiteCastleKing;
BitBoard    Zobrist::WhiteCastleQueen;
BitBoard    Zobrist::EnPassant[64];

// Note: we need a total of 13*64 + 4 = 836 random numbers for all tables

static void initZobrist( BitBoard * p, int len )
{
    static Random   random;

    while( len > 0 ) {
        *p = random.getBitBoard();

        p++;
        len--;
    }
}       

void Zobrist::initialize()
{
    initZobrist( BlackPawn,     64 );
    initZobrist( BlackKnight,   64 );
    initZobrist( BlackBishop,   64 );
    initZobrist( BlackRook,     64 );
    initZobrist( BlackQueen,    64 );
    initZobrist( BlackKing,     64 );
    initZobrist( WhitePawn,     64 );
    initZobrist( WhiteKnight,   64 );
    initZobrist( WhiteBishop,   64 );
    initZobrist( WhiteRook,     64 );
    initZobrist( WhiteQueen,    64 );
    initZobrist( WhiteKing,     64 );
    initZobrist( EnPassant,     64 );
    initZobrist( &BlackCastleKing,  1 );
    initZobrist( &BlackCastleQueen, 1 );
    initZobrist( &WhiteCastleKing,  1 );
    initZobrist( &WhiteCastleQueen, 1 );
}
