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
#ifndef SCORE_H
#define SCORE_H

const int Stage_QueenValue  = 4;
const int Stage_RookValue   = 2;
const int Stage_MinorValue  = 1;
const int Stage_Cap         = 12; // Larger values are automatically converted to this

const int Stage_Max             = Stage_Cap;
const int Stage_Min             = 0;
const int Stage_Opening         = Stage_Max;
const int Stage_EarlyEndgame    = 3;
const int Stage_Endgame         = 0;
const int Stage_Count           = Stage_Max - Stage_Min + 1;

struct Score
{
    // Piece values
    static int  Pawn;
    static int  Knight;
    static int  Bishop;
    static int  Rook;
    static int  Queen;
    static int  King;

    static int  Piece   [16];
    static int  PieceAbs[16];

    static const char * ByPiece_Opening[16];
    static const char * ByPiece_Endgame[16];

    // Piece/square table
    static char BlackKnight_Opening[64];
    static char BlackKnight_Endgame[64];
    static char BlackBishop_Opening[64];
    static char BlackBishop_Endgame[64];
    static char BlackRook_Opening[64];
    static char BlackRook_Endgame[64];
    static char BlackQueen_Opening[64];
    static char BlackQueen_Endgame[64];
    static char BlackKing_Opening[64];
    static char BlackKing_Endgame[64];

    static char WhiteKnight_Opening[64];
    static char WhiteKnight_Endgame[64];
    static char WhiteBishop_Opening[64];
    static char WhiteBishop_Endgame[64];
    static char WhiteRook_Opening[64];
    static char WhiteRook_Endgame[64];
    static char WhiteQueen_Opening[64];
    static char WhiteQueen_Endgame[64];
    static char WhiteKing_Opening[64];
    static char WhiteKing_Endgame[64];

    static int  BlackKnightOutpost[64];
    static int  WhiteKnightOutpost[64];

    static int  RookOnOpenFile      [8];

    // Positional advantages/disadvantages
    enum {
        Min                     = -32000,
        Max                     = +32000,
        Mate                    = +20000,
        MateLo                  = 500 - Mate,
        MateHi                  = Mate - 500,

        // Bishops
        BishopPair                  = 35,
        BishopPair_OwnPawnPenalty   =  2,   // Each own pawn gets penalized
        EndgameBishopWithPassers    = 25,   // Endgame with pawns on both sides of the board

        // Rooks
        RookOn7thRank_Opening   = 15,
        RookOn7thRank_Endgame   = 30,
        RookOn7thRankAbsolute   = 15,   // Extra for 7th rank
        RookOnHalfOpenFile      = 10,
        RookBehindPassedPawn    = 20,

        // Development
        BlockedCenterPawn       = 25,

        BadTrade                = 100,
    };

    static void initialize();

    static bool isMate( int score ) {
        return (score < Score::MateLo) || (score > Score::MateHi);
    }

    static inline int getByStage( int stage, int smax, int smin ) {
        return smin + (smax - smin)*stage / Stage_Max;
        // TODO!!!
        return smin + (smax - smin + 1)*stage / Stage_Count;
    }

    static void getSymmetricSquareValueTable( int * dst, const int * src );
};

#endif // SCORE_H
