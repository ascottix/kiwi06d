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
#ifndef ATTACKS_H_
#define ATTACKS_H_

#include "bitboard.h"

enum {
    DirNone     = 0,

    // Direction
    DirRank     = 1,
    DirFile     = 2,
    DirA1H8     = 4,
    DirA8H1     = 8,

    // Direction (extended info)
    DirRank_A   = 1,    // Same rank, closer to the "A" file
    DirRank_H,          // Same rank, closer to the "H" file
    DirFile_1,          // Same file, closer to the "1" rank
    DirFile_8,          // Same file, closer to the "8" rank
    DirA1H8_A1,         // On A1-H8 diagonal, closer to A1
    DirA1H8_H8,         // On A1-H8 diagonal, closer to H8
    DirA8H1_A8,         // On A8-H1 diagonal, closer to A8
    DirA8H1_H1,         // On A8-H1 diagonal, closer to H1
};

struct Attacks
{
    static BitBoard     BlackPawn[64];
    static BitBoard     WhitePawn[64];
    static BitBoard     BishopA1H8[64][256];
    static BitBoard     BishopA8H1[64][256];
    static BitBoard     King[64];
    static BitBoard     Knight[64];
    static BitBoard     RookRank[64][256];
    static BitBoard     RookFile[64][256];
    static BitBoard     Rook[64];
    static BitBoard     Bishop[64];
    static BitBoard     BlackPawnCouldAttack[64];
    static BitBoard     WhitePawnCouldAttack[64];
    static BitBoard     SquaresBetween[64][64];

    // Distance between two squares
    static int          Distance[64][64];       // So called "Taxicab" or "Manhattan" distance
    static int          KingDistance[64][64];
    
    // Direction that connects two squares (see enum above)
    static char         Direction[64][64];
    static char         DirectionEx[64][64];    // Extended info
    
    // Initialization
    static void     initialize();

    static bool     isValidSquare( int file, int rank );

    static void     setBitByFileRank( BitBoard & b, int file, int rank );

private:
    static void     setSquareBetweenBit( int src, int dst, int file, int rank );
};

#endif // ATTACKS_H_
