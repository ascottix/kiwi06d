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

const char LiteSquareChar   = ' ';
const char DarkSquareChar   = '.';

BitBoard    SetTA1H8[64];
BitBoard    SetDA1H8[64];
BitBoard    SetDA8H1[64];

/*
    Table to transpose the board along the A1-H8 diagonal
*/
int boardTA1H8[64] = 
{
    A1, A2, A3, A4, A5, A6, A7, A8,
    B1, B2, B3, B4, B5, B6, B7, B8,
    C1, C2, C3, C4, C5, C6, C7, C8,
    D1, D2, D3, D4, D5, D6, D7, D8,
    E1, E2, E3, E4, E5, E6, E7, E8,
    F1, F2, F3, F4, F5, F6, F7, F8,
    G1, G2, G3, G4, G5, G6, G7, G8,
    H1, H2, H3, H4, H5, H6, H7, H8
};

/*
 0 a8
 1 a7 b8
 3 a6 b7 c8
 6 a5 b6 c7 d8
10 a4 b5 c6 d7 e8
15 a3 b4 c5 d6 e7 f8
21 a2 b3 c4 d5 e6 f7 g8
28 a1 b2 c3 d4 e5 f6 g7 h8
36 b1 c2 d3 e4 f5 g6 h7
43 c1 d2 e3 f4 g5 h6
49 d1 e2 f3 g4 h5
54 e1 f2 g3 h4
58 f1 g2 h3
61 g1 h2
63 h1
*/

/*
    Table to rearrange the board so that squares on diagonals parallel to A1-H8
    are placed in contiguous bits (in a bitboard)
*/
int boardDA1H8[64] =
{
    28, 36, 43, 49, 54, 58, 61, 63,
    21, 29, 37, 44, 50, 55, 59, 62,
    15, 22, 30, 38, 45, 51, 56, 60,
    10, 16, 23, 31, 39, 46, 52, 57,
     6, 11, 17, 24, 32, 40, 47, 53,
     3,  7, 12, 18, 25, 33, 41, 48,
     1,  4,  8, 13, 19, 26, 34, 42,
     0,  2,  5,  9, 14, 20, 27, 35
};

/*
    Table to shift bits aligned by the diagonal A1-H8 table so that bits
    on the same diagonal can be moved at bit 0 (and used as indexes).
*/
int shiftDA1H8[64] =
{
    28, 36, 43, 49, 54, 58, 61, 63,
    21, 28, 36, 43, 49, 54, 58, 61, 
    15, 21, 28, 36, 43, 49, 54, 58, 
    10, 15, 21, 28, 36, 43, 49, 54, 
     6, 10, 15, 21, 28, 36, 43, 49, 
     3,  6, 10, 15, 21, 28, 36, 43, 
     1,  3,  6, 10, 15, 21, 28, 36, 
     0,  1,  3,  6, 10, 15, 21, 28
};

/*
 0 a1
 1 a2 b1
 3 a3 b2 c1
 6 a4 b3 c2 d1
10 a5 b4 c3 d2 e1
15 a6 b5 c4 d3 e2 f1
21 a7 b6 c5 d4 e3 f2 g1
28 a8 b7 c6 d5 e4 f3 g2 h1
36 b8 c7 d6 e5 f4 g3 h2
43 c8 d7 e6 f5 g4 h3
49 d8 e7 f6 g5 h4
54 e8 f7 g6 h5
58 f8 g7 h6
61 g8 h7
63 h8
*/

/*
    Table to rearrange the board so that squares on diagonals parallel to A8-H1
    are placed in contiguous bits (in a bitboard)
*/
int boardDA8H1[64] =
{
     0,  2,  5,  9, 14, 20, 27, 35,
     1,  4,  8, 13, 19, 26, 34, 42,
     3,  7, 12, 18, 25, 33, 41, 48,
     6, 11, 17, 24, 32, 40, 47, 53,
    10, 16, 23, 31, 39, 46, 52, 57,
    15, 22, 30, 38, 45, 51, 56, 60,
    21, 29, 37, 44, 50, 55, 59, 62,
    28, 36, 43, 49, 54, 58, 61, 63
};

/*
    Table to shift bits aligned by the diagonal A8-H1 table so that bits
    on the same diagonal can be moved at bit 0 (and used as indexes).
*/
int shiftDA8H1[64] =
{
     0,  1,  3,  6, 10, 15, 21, 28,
     1,  3,  6, 10, 15, 21, 28, 36, 
     3,  6, 10, 15, 21, 28, 36, 43, 
     6, 10, 15, 21, 28, 36, 43, 49, 
    10, 15, 21, 28, 36, 43, 49, 54, 
    15, 21, 28, 36, 43, 49, 54, 58, 
    21, 28, 36, 43, 49, 54, 58, 61, 
    28, 36, 43, 49, 54, 58, 61, 63
};

// Note: some versions of Winboard are not able to parse uppercase letters here!
static const char * NameOfFile = "abcdefgh";
static const char * NameOfRank = "12345678";

char * Board::getSquareName( char * name, int square )
{
    char * result = name;

    *name++ = NameOfFile[ square & 7 ];
    *name++ = NameOfRank[ square >> 3 ];
    *name++ = '\0';

    return result;
}

const char * Board::squareName( int square )
{
    static char buf[3];

    getSquareName( buf, square );

    return buf;
}

void Board::initialize()
{
    for( int i=0; i<64; i++ ) {
        SetTA1H8[i] = BitBoard::Set[ boardTA1H8[i] ];
        SetDA1H8[i] = BitBoard::Set[ boardDA1H8[i] ];
        SetDA8H1[i] = BitBoard::Set[ boardDA8H1[i] ];
    }
}

int Board::operator == ( const Board & b ) const
{
    for( int i=0; i<64; i++ )
        if( piece[i] != b.piece[i] ) return 0;
    return 1;
}

void Board::dump( const char * header ) const
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
        fprintf( f, "[board] %s\n", header );
    }

    fprintf( f, "  +-----------------+\n" );
    for( int rank=0; rank<8; rank++ ) {
        fprintf( f, "%c | ", '8'-rank );
        for( int file=0; file<8; file++ ) {
            char    c = (file+rank) & 1 ? DarkSquareChar : LiteSquareChar;

            switch( piece[map[rank][file]] ) {
            case BlackPawn:     c = 'p'; break;
            case BlackKnight:   c = 'n'; break;
            case BlackBishop:   c = 'b'; break;
            case BlackRook:     c = 'r'; break;
            case BlackQueen:    c = 'q'; break;
            case BlackKing:     c = 'k'; break;
            case WhitePawn:     c = 'P'; break;
            case WhiteKnight:   c = 'N'; break;
            case WhiteBishop:   c = 'B'; break;
            case WhiteRook:     c = 'R'; break;
            case WhiteQueen:    c = 'Q'; break;
            case WhiteKing:     c = 'K'; break;
            }
            fprintf( f, "%c ", c );
        }
        fprintf( f, "|\n" );
    }
    fprintf( f, "  +-----------------+\n" );
    fprintf( f, "    a b c d e f g h\n" );
}
