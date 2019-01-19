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
#include "board.h"
#include "log.h"
#include "score.h"

/*
    Basic piece values. These can be redefined at will, however most
    other values and tables assume that the value of a pawn is 100.

*/
int Score::Pawn         = 100;
int Score::Knight       = 325;
int Score::Bishop       = 330;
int Score::Rook         = 500;
int Score::Queen        = 975;
int Score::King         = 0;

int Score::Piece[16];
int Score::PieceAbs[16];

const char * Score::ByPiece_Opening[16];
const char * Score::ByPiece_Endgame[16];

/*
    Adjustment is a ratio expressed in 1/1000th units and is added or subtracted from
    the corresponding piece/square values. For example, a +100 adjustment causes
    all values to be incremented by 10 percent. Use 0 for no adjustment.
*/
const int   Knight_Adjustment = 0;
const int   Bishop_Adjustment = 0;
const int   Rook_Adjustment = 0;
const int   Queen_Adjustment = 0;

static int adjustScore( int score, int adjustment )
{
    return score + (score * adjustment) / 1000;
}

/*
    Piece/square tables must be visualized like playing for white,
    even if in practice they refer to black because the A1 square
    is defined as zero, which happens to be in the top left corner
    when declaring arrays.

    This is especially true for tables that explicitly state "Black"
    in their name, such as for example BlackKnightOutpost. Again,
    the table is for black but it is much easier to visualize it
    from the white side, with the first rank in the bottom.
*/
char Score::BlackKnight_Opening[64] =  {
     -95,  -17,  -11,   -7,   -7,  -11,  -17,  -95, // Trapped knight idea from Fruit
     -14,   -6,    0,    3,    3,    0,   -6,  -16,
      -3,    4,   12,   16,   16,   12,    4,   -3,
      -3,    4,   11,   15,   15,   11,    4,   -3,
      -7,    0,    7,   11,   11,    7,    0,   -7,
     -14,   -6,    1,    3,    3,    1,   -6,  -14,
     -26,  -17,  -10,   -6,   -6,  -10,  -17,  -26,
     -38,  -29,  -21,  -17,  -17,  -21,  -29,  -38,
};

char Score::BlackKnight_Endgame[64] = {
     -31,  -23,  -14,  -11,  -11,  -14,  -23,  -31,
     -23,  -14,   -6,   -3,   -3,   -6,  -14,  -23,
     -14,   -6,    0,    4,    4,    0,   -6,  -14,
     -11,   -3,    4,    8,    8,    4,   -3,  -11,
     -11,   -3,    4,    8,    8,    4,   -3,  -11,
     -14,   -6,    0,    4,    4,    0,   -6,  -14,
     -23,  -14,   -6,   -3,   -3,   -6,  -14,  -23,
     -31,  -23,  -14,  -11,  -11,  -14,  -23,  -31,
};

char Score::BlackBishop_Opening[64] =  {
      -9,   -8,   -5,   -3,   -3,   -5,   -8,   -9,
      -8,   -2,   -1,    0,    0,   -1,   -2,   -8,
      -7,   -1,    2,    1,    1,    2,   -1,   -7,
      -3,    0,    1,    5,    5,    1,    0,   -3,
      -3,    0,    1,    5,    5,    1,    0,   -3,
      -5,   -1,    2,    1,    1,    2,   -1,   -5,
      -7,    0,   -1,    0,    0,   -1,    0,   -7,
     -15,  -14,  -11,  -10,  -10,  -11,  -14,  -15,
};

char Score::BlackBishop_Endgame[64] = {
     -14,   -9,   -7,   -5,   -5,   -7,   -9,  -14,
      -9,   -5,   -2,    0,    0,   -2,   -5,   -9,
      -7,   -2,    0,    2,    2,    0,   -2,   -7,
      -5,    0,    2,    5,    5,    2,    0,   -5,
      -5,    0,    2,    5,    5,    2,    0,   -5,
      -7,   -2,    0,    2,    2,    0,   -2,   -7,
      -9,   -5,   -2,    0,    0,   -2,   -5,   -9,
     -14,   -9,   -7,   -5,   -5,   -7,   -9,  -14,
};

char Score::BlackRook_Opening[64] =  {
      -7,   -2,    0,    2,    2,    0,   -2,   -7,
      -7,   -2,    0,    2,    2,    0,   -2,   -7,
      -6,   -2,    0,    2,    2,    0,   -2,   -6,
      -6,   -2,    0,    2,    2,    0,   -2,   -6,
      -5,   -2,    0,    2,    2,    0,   -2,   -5,
      -5,   -2,    0,    3,    3,    0,   -2,   -5,
      -5,   -2,    0,    3,    3,    0,   -2,   -5,
      -5,   -2,    1,    4,    4,    1,   -2,   -5,
};

char Score::BlackRook_Endgame[64] = {
      -1,    0,    0,    0,    0,    0,    0,   -1,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    1,    1,    0,    0,    0,
       0,    0,    1,    1,    1,    1,    0,    0,
       0,    0,    1,    1,    1,    1,    0,    0,
       0,    0,    0,    1,    1,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
      -1,    0,    0,    0,    0,    0,    0,   -1,
};

char Score::BlackQueen_Opening[64] =  {
      -3,   -3,   -3,   -3,   -3,   -3,   -3,   -3,
      -1,   -1,    0,    0,    0,    0,   -1,   -1,
      -2,   -1,    0,    0,    0,    0,   -1,   -2,
      -3,    0,    0,    0,    0,    0,    0,   -3,
      -3,    0,    0,    0,    0,    0,    0,   -3,
      -3,    0,    0,    0,    0,    0,    0,   -3,
      -3,    0,    1,    1,    1,    1,    0,   -3,
      -5,   -3,   -3,   -4,   -4,   -3,   -3,   -5,
};

char Score::BlackQueen_Endgame[64] = {
     -18,  -11,   -9,   -6,   -6,   -9,  -11,  -18,
     -11,   -5,   -2,    0,    0,   -2,   -5,  -11,
      -9,   -2,    1,    3,    3,    1,   -2,   -9,
      -6,    0,    3,    7,    7,    3,    0,   -6,
      -6,    0,    3,    7,    7,    3,    0,   -6,
      -9,   -2,    1,    3,    3,    1,   -2,   -9,
     -11,   -5,   -2,    0,    0,   -2,   -5,  -11,
     -18,  -11,   -9,   -6,   -6,   -9,  -11,  -18,
};

char Score::BlackKing_Opening[64] =  {
     -30,  -30,  -40,  -50,  -50,  -40,  -30,  -30,
     -30,  -30,  -30,  -40,  -40,  -30,  -30,  -30,
     -20,  -20,  -20,  -30,  -30,  -20,  -20,  -20,
     -20,  -20,  -20,  -25,  -25,  -20,  -20,  -20,
     -20,  -20,  -20,  -20,  -20,  -20,  -20,  -20,
     -15,  -15,  -15,  -15,  -15,  -15,  -15,  -15,
       5,    5,    0,   -5,   -5,    0,    5,    5,
      20,   25,   15,    5,    5,   15,   25,   20
};

char Score::BlackKing_Endgame[64] = {
     -50,  -33,  -25,  -17,  -17,  -25,  -33,  -50,
     -33,  -17,   -8,    0,    0,   -8,  -17,  -33,
     -25,   -8,    1,    9,    9,    1,   -8,  -25,
     -17,    0,    9,   18,   18,    9,    0,  -17,
     -17,    0,    9,   18,   18,    9,    0,  -17,
     -25,   -8,    1,    9,    9,    1,   -8,  -25,
     -33,  -17,   -8,    0,    0,   -8,  -17,  -33,
     -50,  -33,  -25,  -17,  -17,  -25,  -33,  -50,
};

// Knight outpost
int Score::BlackKnightOutpost[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  5,  5,  0,  0,  0,
    0,  0,  5, 10, 10,  5,  0,  0,
    0,  0,  5, 10, 10,  5,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};

char Score::WhiteKnight_Opening[64];
char Score::WhiteKnight_Endgame[64];
char Score::WhiteBishop_Opening[64];
char Score::WhiteBishop_Endgame[64];
char Score::WhiteRook_Opening[64];
char Score::WhiteRook_Endgame[64];
char Score::WhiteQueen_Opening[64];
char Score::WhiteQueen_Endgame[64];
char Score::WhiteKing_Opening[64];
char Score::WhiteKing_Endgame[64];

int Score::WhiteKnightOutpost[64];

int Score::RookOnOpenFile[8]        = { 15, 15, 17, 20, 20, 17, 15, 15 };

static void getSymmetricSquareValueTableCh( char * dst, const char * src )
{
    for( int square=0; square<64; square++ ) {
        int file = FileOfSquare( square );
        int rank = RankOfSquare( square );

        int symmetric_square = FileRankToSquare( file, 7-rank );

        dst[symmetric_square] = src[square];
    }
}

void Score::initialize()
{
    int i;

    getSymmetricSquareValueTableCh( WhiteKnight_Opening, BlackKnight_Opening );
    getSymmetricSquareValueTableCh( WhiteKnight_Endgame, BlackKnight_Endgame );
    getSymmetricSquareValueTableCh( WhiteBishop_Opening, BlackBishop_Opening );
    getSymmetricSquareValueTableCh( WhiteBishop_Endgame, BlackBishop_Endgame );
    getSymmetricSquareValueTableCh( WhiteRook_Opening, BlackRook_Opening );
    getSymmetricSquareValueTableCh( WhiteRook_Endgame, BlackRook_Endgame );
    getSymmetricSquareValueTableCh( WhiteQueen_Opening, BlackQueen_Opening );
    getSymmetricSquareValueTableCh( WhiteQueen_Endgame, BlackQueen_Endgame );
    getSymmetricSquareValueTableCh( WhiteKing_Opening, BlackKing_Opening );
    getSymmetricSquareValueTableCh( WhiteKing_Endgame, BlackKing_Endgame );

    getSymmetricSquareValueTable( WhiteKnightOutpost, BlackKnightOutpost );

    // Piece values: these tables must always be constructed dynamically,
    // because we must keep available the freedom of changing the internal
    // bit representation of a piece (i.e. which bits identify the piece
    // and the side... see the definitions of PieceSide and PieceType)
    for( i=0; i<16; i++ ) {
        int value = 0;

        // Basic idea for evaluation: no single piece can increment
        // the defect count alone, but Q+P or R+R or say B+B+N are
        // all worth one defect.

        switch( i ) {
        case ::BlackPawn:
        case ::WhitePawn:
            value = Score::Pawn;    
            break;
        case ::BlackKnight: 
        case ::WhiteKnight: 
            value = Score::Knight;  
            break;
        case ::BlackBishop: 
        case ::WhiteBishop: 
            value = Score::Bishop;  
            break;
        case ::BlackRook:   
        case ::WhiteRook:   
            value = Score::Rook;    
            break;
        case ::BlackQueen:  
        case ::WhiteQueen:  
            value = Score::Queen;
            break;
        case ::BlackKing:   
        case ::WhiteKing:   
            value = Score::King;    
            break;
        }

        PieceAbs[i] = value;
        Piece[i] = PieceSide(i) == Black ? -value : +value;
    }

    // Prepare some tables with pointers to the piece/square tables that might
    // be useful elsewhere
    for( i=0; i<16; i++ ) {
        ByPiece_Opening[i] = 0;
        ByPiece_Endgame[i] = 0;
    }

    ByPiece_Opening[ ::BlackKnight ]  = BlackKnight_Opening;
    ByPiece_Opening[ ::BlackBishop ]  = BlackBishop_Opening;
    ByPiece_Opening[ ::BlackRook ]  = BlackRook_Opening;
    ByPiece_Opening[ ::BlackQueen ]  = BlackQueen_Opening;
    ByPiece_Opening[ ::BlackKing ]  = BlackKing_Opening;
    ByPiece_Opening[ ::WhiteKnight ]  = WhiteKnight_Opening;
    ByPiece_Opening[ ::WhiteBishop ]  = WhiteBishop_Opening;
    ByPiece_Opening[ ::WhiteRook ]  = WhiteRook_Opening;
    ByPiece_Opening[ ::WhiteQueen ]  = WhiteQueen_Opening;
    ByPiece_Opening[ ::WhiteKing ]  = WhiteKing_Opening;

    ByPiece_Endgame[ ::BlackKnight ]  = BlackKnight_Endgame;
    ByPiece_Endgame[ ::BlackBishop ]  = BlackBishop_Endgame;
    ByPiece_Endgame[ ::BlackRook ]  = BlackRook_Endgame;
    ByPiece_Endgame[ ::BlackQueen ]  = BlackQueen_Endgame;
    ByPiece_Endgame[ ::BlackKing ]  = BlackKing_Endgame;
    ByPiece_Endgame[ ::WhiteKnight ]  = WhiteKnight_Endgame;
    ByPiece_Endgame[ ::WhiteBishop ]  = WhiteBishop_Endgame;
    ByPiece_Endgame[ ::WhiteRook ]  = WhiteRook_Endgame;
    ByPiece_Endgame[ ::WhiteQueen ]  = WhiteQueen_Endgame;
    ByPiece_Endgame[ ::WhiteKing ]  = WhiteKing_Endgame;
}

void Score::getSymmetricSquareValueTable( int * dst, const int * src )
{
    for( int square=0; square<64; square++ ) {
        int file = FileOfSquare( square );
        int rank = RankOfSquare( square );

        int symmetric_square = FileRankToSquare( file, 7-rank );

        dst[symmetric_square] = src[square];
    }
}
