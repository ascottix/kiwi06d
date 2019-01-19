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

#include "bitboard.h"
#include "board.h"
#include "log.h"

CACHE_ALIGN const unsigned int lsz64_tbl[64] =
{
    63, 30,  3, 32, 59, 14, 11, 33,
    60, 24, 50,  9, 55, 19, 21, 34,
    61, 29,  2, 53, 51, 23, 41, 18,
    56, 28,  1, 43, 46, 27,  0, 35,
    62, 31, 58,  4,  5, 49, 54,  6,
    15, 52, 12, 40,  7, 42, 45, 16,
    25, 57, 48, 13, 10, 39,  8, 44,
    20, 47, 38, 22, 17, 37, 36, 26,
};

const BitBoard BitBoard::Set[64] =
{
   (((Uint64)1) <<  0),
   (((Uint64)1) <<  1),
   (((Uint64)1) <<  2),
   (((Uint64)1) <<  3),
   (((Uint64)1) <<  4),
   (((Uint64)1) <<  5),
   (((Uint64)1) <<  6),
   (((Uint64)1) <<  7),
   (((Uint64)1) <<  8),
   (((Uint64)1) <<  9),
   (((Uint64)1) << 10),
   (((Uint64)1) << 11),
   (((Uint64)1) << 12),
   (((Uint64)1) << 13),
   (((Uint64)1) << 14),
   (((Uint64)1) << 15),
   (((Uint64)1) << 16),
   (((Uint64)1) << 17),
   (((Uint64)1) << 18),
   (((Uint64)1) << 19),
   (((Uint64)1) << 20),
   (((Uint64)1) << 21),
   (((Uint64)1) << 22),
   (((Uint64)1) << 23),
   (((Uint64)1) << 24),
   (((Uint64)1) << 25),
   (((Uint64)1) << 26),
   (((Uint64)1) << 27),
   (((Uint64)1) << 28),
   (((Uint64)1) << 29),
   (((Uint64)1) << 30),
   (((Uint64)1) << 31),
   (((Uint64)1) << 32),
   (((Uint64)1) << 33),
   (((Uint64)1) << 34),
   (((Uint64)1) << 35),
   (((Uint64)1) << 36),
   (((Uint64)1) << 37),
   (((Uint64)1) << 38),
   (((Uint64)1) << 39),
   (((Uint64)1) << 40),
   (((Uint64)1) << 41),
   (((Uint64)1) << 42),
   (((Uint64)1) << 43),
   (((Uint64)1) << 44),
   (((Uint64)1) << 45),
   (((Uint64)1) << 46),
   (((Uint64)1) << 47),
   (((Uint64)1) << 48),
   (((Uint64)1) << 49),
   (((Uint64)1) << 50),
   (((Uint64)1) << 51),
   (((Uint64)1) << 52),
   (((Uint64)1) << 53),
   (((Uint64)1) << 54),
   (((Uint64)1) << 55),
   (((Uint64)1) << 56),
   (((Uint64)1) << 57),
   (((Uint64)1) << 58),
   (((Uint64)1) << 59),
   (((Uint64)1) << 60),
   (((Uint64)1) << 61),
   (((Uint64)1) << 62),
   (((Uint64)1) << 63)
};

const BitBoard BitBoard::Clr[64] =
{
   ~(((Uint64)1) <<  0),
   ~(((Uint64)1) <<  1),
   ~(((Uint64)1) <<  2),
   ~(((Uint64)1) <<  3),
   ~(((Uint64)1) <<  4),
   ~(((Uint64)1) <<  5),
   ~(((Uint64)1) <<  6),
   ~(((Uint64)1) <<  7),
   ~(((Uint64)1) <<  8),
   ~(((Uint64)1) <<  9),
   ~(((Uint64)1) << 10),
   ~(((Uint64)1) << 11),
   ~(((Uint64)1) << 12),
   ~(((Uint64)1) << 13),
   ~(((Uint64)1) << 14),
   ~(((Uint64)1) << 15),
   ~(((Uint64)1) << 16),
   ~(((Uint64)1) << 17),
   ~(((Uint64)1) << 18),
   ~(((Uint64)1) << 19),
   ~(((Uint64)1) << 20),
   ~(((Uint64)1) << 21),
   ~(((Uint64)1) << 22),
   ~(((Uint64)1) << 23),
   ~(((Uint64)1) << 24),
   ~(((Uint64)1) << 25),
   ~(((Uint64)1) << 26),
   ~(((Uint64)1) << 27),
   ~(((Uint64)1) << 28),
   ~(((Uint64)1) << 29),
   ~(((Uint64)1) << 30),
   ~(((Uint64)1) << 31),
   ~(((Uint64)1) << 32),
   ~(((Uint64)1) << 33),
   ~(((Uint64)1) << 34),
   ~(((Uint64)1) << 35),
   ~(((Uint64)1) << 36),
   ~(((Uint64)1) << 37),
   ~(((Uint64)1) << 38),
   ~(((Uint64)1) << 39),
   ~(((Uint64)1) << 40),
   ~(((Uint64)1) << 41),
   ~(((Uint64)1) << 42),
   ~(((Uint64)1) << 43),
   ~(((Uint64)1) << 44),
   ~(((Uint64)1) << 45),
   ~(((Uint64)1) << 46),
   ~(((Uint64)1) << 47),
   ~(((Uint64)1) << 48),
   ~(((Uint64)1) << 49),
   ~(((Uint64)1) << 50),
   ~(((Uint64)1) << 51),
   ~(((Uint64)1) << 52),
   ~(((Uint64)1) << 53),
   ~(((Uint64)1) << 54),
   ~(((Uint64)1) << 55),
   ~(((Uint64)1) << 56),
   ~(((Uint64)1) << 57),
   ~(((Uint64)1) << 58),
   ~(((Uint64)1) << 59),
   ~(((Uint64)1) << 60),
   ~(((Uint64)1) << 61),
   ~(((Uint64)1) << 62),
   ~(((Uint64)1) << 63)
};

void BitBoard::dump( const char * header ) const
{
    FILE * f = Log::file();

    static int  map[8][8] = 
    {
        { A8, B8, C8, D8, E8, F8, G8, H8 },
        { A7, B7, C7, D7, E7, F7, G7, H7 },
        { A6, B6, C6, D6, E6, F6, G6, H6 },
        { A5, B5, C5, D5, E5, F5, G5, H5 },
        { A4, B4, C4, D4, E4, F4, G4, H4 },
        { A3, B3, C3, D3, E3, F3, G3, H3 },
        { A2, B2, C2, D2, E2, F2, G2, H2 },
        { A1, B1, C1, D1, E1, F1, G1, H1 }
    };

    if( header != 0 ) {
        fprintf( f, "[bitboard] %s\n", header );
    }

    for( int rank=0; rank<8; rank++ ) {
        fprintf( f, "  " );

        for( int file=0; file<8; file++ ) {
            fprintf( f, "%c ", getBit( map[rank][file] ) ? '*' : '.' );
        }

        fprintf( f, "\n" );
    }
}
