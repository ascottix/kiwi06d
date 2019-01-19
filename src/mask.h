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
#ifndef MASK_H_
#define MASK_H_

#include "bitboard.h"

struct Mask
{
    // Squares on the specified rank
    static BitBoard     Rank[8];

    // Squares from the specified rank (excluded) up to the 1st rank
    static BitBoard     RanksTo1st[8];

    // Squares from the specified rank (excluded) up to the 8th rank
    static BitBoard     RanksTo8th[8];

    // Squares on the specified file
    static BitBoard     File[8];

    // Squares not on the specified file
    static BitBoard     NotFile[8];

    // Squares on the files left and right of the specified file
    static BitBoard     FilesAround[8];

    // The eight squares (or less) surrounding the specified square
    static BitBoard     SquaresAround[64];

    // The one or two squares immediately to the left and right of the specified square
    static BitBoard     SideSquares[64];

    // Squares that go from the specified square (excluded) up to the 1st or 8th rank (on the same file)
    static BitBoard     SquaresTo1stRank[64];
    static BitBoard     SquaresTo8thRank[64];

    // Squares that go from the specified square (excluded) up to the 1st or 8th file (on the same rank)
    static BitBoard     SquaresTo1stFile[64];
    static BitBoard     SquaresTo8thFile[64];

    // Squares on diagonal towards a specific direction
    static BitBoard     SquaresToDiagA1[64];
    static BitBoard     SquaresToDiagH8[64];
    static BitBoard     SquaresToDiagA8[64];
    static BitBoard     SquaresToDiagH1[64];


    // Squares on the 2nd rank, excluding A2 and H2
    static BitBoard     Absolute2nd;

    // Squares on the 7th rank, excluding A7 and H7
    static BitBoard     Absolute7th;

    // Dark-colored squares (e.g. A1)
    static BitBoard     DarkSquares;

    // Lite-colored squares (e.g. H1)
    static BitBoard     LiteSquares;

    // Color (dark or lite) of square
    static BitBoard     Color[64];

    // Board center: D4, E4, D5, E5
    static BitBoard     SweetCenter;

    // Board wide center: 4x4 box from C3 to F6
    static BitBoard     WideCenter;

    // Half board on black and white side
    static BitBoard     BlackSide;
    static BitBoard     WhiteSide;

    // Dark squares in the center: D4, E5
    static BitBoard     SweetCenterDarkSquares;

    // Lite squares in the center: E4, D5
    static BitBoard     SweetCenterLiteSquares;

    // Center side: "D" and "E" files
    static BitBoard     CenterSide;

    // Center pawns on starting squares
    static BitBoard     D2_E2;
    static BitBoard     D7_E7;

    // King side: "F", "G" and "H" files
    static BitBoard     KingSide;

    // Queen side: "A", "B" and "C" files
    static BitBoard     QueenSide;

    // Squares that must be free (from enemy pawns) for a pawn to be passed
    static BitBoard     BlackPassed[64];
    static BitBoard     WhitePassed[64];

    // Squares that enemy king must occupy in order to be able to stop a passed pawn
    static BitBoard     WhitePassed_Square_WTM[64];
    static BitBoard     WhitePassed_Square_BTM[64];
    static BitBoard     WhitePassed_Effective[64];
    static BitBoard     WhitePassed_RelativeEffective[64];
    static BitBoard     BlackPassed_Square_WTM[64];
    static BitBoard     BlackPassed_Square_BTM[64];
    static BitBoard     BlackPassed_Effective[64];
    static BitBoard     BlackPassed_RelativeEffective[64];

    // Squares that must be free to allow castling
    static BitBoard     BlackCrossCastleKing;
    static BitBoard     BlackCrossCastleQueen;
    static BitBoard     WhiteCrossCastleKing;
    static BitBoard     WhiteCrossCastleQueen;

    // Squares that make a pawn backward if not occupied by a friendly pawn
    static BitBoard     BlackBackwardPawn[64];
    static BitBoard     WhiteBackwardPawn[64];

    // Squares that make a pawn double if occupied by a friendly pawn
    static BitBoard     BlackDoublePawn[64];
    static BitBoard     WhiteDoublePawn[64];

    // King attack patterns
    static BitBoard     KingAttack_Danger[64];

    static void initialize();
};

#endif // MASK_H_
