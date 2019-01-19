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
#include "mask.h"

BitBoard    Mask::Rank[8];
BitBoard    Mask::RanksTo1st[8];
BitBoard    Mask::RanksTo8th[8];
BitBoard    Mask::File[8];
BitBoard    Mask::NotFile[8];
BitBoard    Mask::SquaresAround[64];
BitBoard    Mask::FilesAround[8];
BitBoard    Mask::SideSquares[64];
BitBoard    Mask::SquaresTo1stRank[64];
BitBoard    Mask::SquaresTo8thRank[64];
BitBoard    Mask::SquaresTo1stFile[64];
BitBoard    Mask::SquaresTo8thFile[64];
BitBoard    Mask::SquaresToDiagA1[64];
BitBoard    Mask::SquaresToDiagH8[64];
BitBoard    Mask::SquaresToDiagA8[64];
BitBoard    Mask::SquaresToDiagH1[64];
BitBoard    Mask::Absolute2nd;
BitBoard    Mask::Absolute7th;
BitBoard    Mask::DarkSquares;
BitBoard    Mask::LiteSquares;
BitBoard    Mask::Color[64];
BitBoard    Mask::SweetCenter;
BitBoard    Mask::WideCenter;
BitBoard    Mask::BlackSide;
BitBoard    Mask::WhiteSide;
BitBoard    Mask::SweetCenterDarkSquares;
BitBoard    Mask::SweetCenterLiteSquares;
BitBoard    Mask::CenterSide;
BitBoard    Mask::D2_E2;
BitBoard    Mask::D7_E7;
BitBoard    Mask::KingSide;
BitBoard    Mask::QueenSide;
BitBoard    Mask::BlackPassed[64];
BitBoard    Mask::WhitePassed[64];
BitBoard    Mask::WhitePassed_Square_WTM[64];
BitBoard    Mask::WhitePassed_Square_BTM[64];
BitBoard    Mask::WhitePassed_Effective[64];
BitBoard    Mask::WhitePassed_RelativeEffective[64];
BitBoard    Mask::BlackPassed_Square_WTM[64];
BitBoard    Mask::BlackPassed_Square_BTM[64];
BitBoard    Mask::BlackPassed_Effective[64];
BitBoard    Mask::BlackPassed_RelativeEffective[64];
BitBoard    Mask::BlackCrossCastleKing;
BitBoard    Mask::BlackCrossCastleQueen;
BitBoard    Mask::WhiteCrossCastleKing;
BitBoard    Mask::WhiteCrossCastleQueen;

BitBoard    Mask::BlackBackwardPawn[64];
BitBoard    Mask::WhiteBackwardPawn[64];

BitBoard    Mask::BlackDoublePawn[64];
BitBoard    Mask::WhiteDoublePawn[64];

BitBoard    Mask::KingAttack_Danger[64];

#define SqFromFileRank( file, rank )    ((file)+((rank)*8))

static void setBit( BitBoard & bb, int file, int rank )
{
    if( file >= 0 && file <= 7 && rank >= 0 && rank <= 7 ) {
        bb.setBit( FileRankToSquare( file, rank ) );
    }
}

static void setBitLine( BitBoard & bb, int r, int f1, int f2 ) 
{
    // Clip values
    if( f1 < 0 ) f1 = 0;
    if( f2 > 7 ) f2 = 7;

    while( f1 <= f2 ) {
        bb.setBit( FileRankToSquare( f1, r ) );
        f1++;
    }
}

static void setBitRectangle( BitBoard & bb, int f1, int r1, int f2, int r2 )
{
    // Clip values
    if( r1 < 0 ) r1 = 0;
    if( r2 > 7 ) r2 = 7;

    while( r1 <= r2 ) {
        setBitLine( bb, r1, f1, f2 );
        r1++;
    }
}

void Mask::initialize()
{
    int i, j;
    int file, rank;

    // Files and ranks
    for( rank=0; rank<8; rank++ ) {
        Rank[rank].clear();
        for( file=0; file<8; file++ ) Rank[rank].setBit( SqFromFileRank(file,rank) );
    }

    for( file=0; file<8; file++ ) {
        File[file].clear();
        for( rank=0; rank<8; rank++ ) File[file].setBit( SqFromFileRank(file,rank) );
        NotFile[file] = ~File[file];
    }

    for( file=0; file<8; file++ ) {
        FilesAround[file].clear();
        if( file > 0 ) FilesAround[file] |= File[file-1];
        if( file < 7 ) FilesAround[file] |= File[file+1];
    }

    for( rank=0; rank<8; rank++ ) {
        RanksTo1st[rank].clear();
        RanksTo8th[rank].clear();
        for( j=rank-1; j>=0; j-- ) RanksTo1st[rank] |= Rank[j];
        for( j=rank+1; j<=7; j++ ) RanksTo8th[rank] |= Rank[j];
    }

    for( i=0; i<64; i++ ) {
        int f = FileOfSquare( i );
        int r = RankOfSquare( i );

        setBit( SquaresAround[i], f-1, r-1 );
        setBit( SquaresAround[i], f-1, r   );
        setBit( SquaresAround[i], f-1, r+1 );
        setBit( SquaresAround[i], f  , r-1 );
        setBit( SquaresAround[i], f  , r+1 );
        setBit( SquaresAround[i], f+1, r-1 );
        setBit( SquaresAround[i], f+1, r   );
        setBit( SquaresAround[i], f+1, r+1 );
    }

    // Absolute 7th rank
    Absolute2nd = Rank[1] ^ (BitBoard::Set[A2] | BitBoard::Set[H2]);
    Absolute7th = Rank[6] ^ (BitBoard::Set[A7] | BitBoard::Set[H7]);

    // Board areas
    SweetCenter = BitBoard::Set[E4] | BitBoard::Set[D4] | BitBoard::Set[D5] | BitBoard::Set[E5];

    WideCenter  = BitBoard::Set[C3] | BitBoard::Set[D3] | BitBoard::Set[E3] | BitBoard::Set[F3] |
                  BitBoard::Set[C4] | BitBoard::Set[F4] |
                  SweetCenter |
                  BitBoard::Set[C5] | BitBoard::Set[F5] |
                  BitBoard::Set[C6] | BitBoard::Set[D6] | BitBoard::Set[E6] | BitBoard::Set[F6];

    BlackSide   = Rank[4] | Rank[5] | Rank[6] | Rank[7];
    WhiteSide   = Rank[0] | Rank[1] | Rank[2] | Rank[3];

    CenterSide  = File[3] | File[4];

    KingSide    = File[5] | File[6] | File[7];

    QueenSide   = File[0] | File[1] | File[2];

    D2_E2 = BitBoard::Set[D2] | BitBoard::Set[E2];
    D7_E7 = BitBoard::Set[D7] | BitBoard::Set[E7];

    // Dark and light squares
    DarkSquares.clear();
    LiteSquares.clear();
    for( file=0; file<8; file++ ) {
        for( rank=0; rank<8; rank++ ) {
            if( (file+rank) & 1 )
                LiteSquares.setBit( SqFromFileRank(file,rank) );
            else
                DarkSquares.setBit( SqFromFileRank(file,rank) );
        }
    }
    for( file=0; file<8; file++ ) {
        for( rank=0; rank<8; rank++ ) {
            int sq = SqFromFileRank( file, rank );
            if( (file+rank) & 1 )
                Color[sq] = LiteSquares;
            else
                Color[sq] = DarkSquares;
        }
    }

    SweetCenterDarkSquares  = SweetCenter & DarkSquares;
    SweetCenterLiteSquares  = SweetCenter & LiteSquares;

    // Passed pawns and side squares
    for( i=0; i<64; i++ ) {
        int file = FileOfSquare(i);
        int rank = RankOfSquare(i);

        // Passed pawns
        BlackPassed[i] = (File[file] | FilesAround[file]) & RanksTo1st[rank];
        WhitePassed[i] = (File[file] | FilesAround[file]) & RanksTo8th[rank];

        // Side squares
        SideSquares[i].clear();
        if( file > 0 ) SideSquares[i].setBit( i-1 );
        if( file < 7 ) SideSquares[i].setBit( i+1 );

        // Directions
        SquaresTo1stRank[i] = File[file] & RanksTo1st[rank];
        SquaresTo8thRank[i] = File[file] & RanksTo8th[rank];

        SquaresTo1stFile[i].clear();
        SquaresTo8thFile[i].clear();
        setBitRectangle( SquaresTo1stFile[i], 0, rank, file-1, rank );
        setBitRectangle( SquaresTo8thFile[i], file+1, rank, 7, rank );

        SquaresToDiagA1[i].clear();
        SquaresToDiagH8[i].clear();
        SquaresToDiagA8[i].clear();
        SquaresToDiagH1[i].clear();

        {
            int f;
            int r;

            f = file-1;
            r = rank-1;
            while( f >= 0 && r >= 0 ) {
                SquaresToDiagA1[i].setBit( FileRankToSquare( f, r ) );
                f--;
                r--;
            }

            f = file+1;
            r = rank+1;
            while( f <= 7 && r <= 7 ) {
                SquaresToDiagH8[i].setBit( FileRankToSquare( f, r ) );
                f++;
                r++;
            }
        
            f = file-1;
            r = rank+1;
            while( f >= 0 && r <= 7 ) {
                SquaresToDiagA8[i].setBit( FileRankToSquare( f, r ) );
                f--;
                r++;
            }

            f = file+1;
            r = rank-1;
            while( f <= 7 && r >= 0 ) {
                SquaresToDiagH1[i].setBit( FileRankToSquare( f, r ) );
                f++;
                r--;
            }
        }

        // Square rule for passed pawns
        WhitePassed_Square_WTM[i].clear();
        WhitePassed_Square_BTM[i].clear();
        BlackPassed_Square_WTM[i].clear();
        BlackPassed_Square_BTM[i].clear();

        if( rank >= 1 && rank <= 6 ) {
            int d;

            // White
            d = rank == 1 ? 5 : 7 - rank;   // Distance to promotion

            setBitRectangle( WhitePassed_Square_WTM[i], file-d,   7-d, file+d,   7 );
            setBitRectangle( WhitePassed_Square_BTM[i], file-d-1, 6-d, file+d+1, 7 );

            // Black
            d = rank == 6 ? 5 : rank;

            setBitRectangle( BlackPassed_Square_BTM[i], file-d,   0, file+d,   d   );
            setBitRectangle( BlackPassed_Square_WTM[i], file-d-1, 0, file+d+1, d+1 );
        }

        // Absolute and relative effective squares for passed pawns
        WhitePassed_Effective[i].clear();
        WhitePassed_RelativeEffective[i].clear();
        BlackPassed_Effective[i].clear();
        BlackPassed_RelativeEffective[i].clear();

        if( file == 0 || file == 7 ) continue;

        if( rank >= 4 ) {
            setBitRectangle( WhitePassed_Effective[i], file-1, rank+1, file+1, 7 );
            if( rank >= 5 ) {
                setBit( WhitePassed_RelativeEffective[i], file-1, rank );
                setBit( WhitePassed_RelativeEffective[i], file+1, rank );
            }
        }
        else {
            setBitRectangle( WhitePassed_RelativeEffective[i], file-1, rank+1, file+1, rank+1 );
            setBitRectangle( WhitePassed_Effective[i], file-1, rank+2, file+1, 7 );
        }

        if( rank <= 3 ) {
            setBitRectangle( BlackPassed_Effective[i], file-1, 0, file+1, rank-1 );
        }
        else {
            setBitRectangle( BlackPassed_Effective[i], file-1, 0, file+1, rank-2 );
        }
    }

    // Intermediate squares while castling
    BlackCrossCastleKing    = BitBoard::Set[F8] | BitBoard::Set[G8];
    BlackCrossCastleQueen   = BitBoard::Set[D8] | BitBoard::Set[C8] | BitBoard::Set[B8];
    WhiteCrossCastleKing    = BitBoard::Set[F1] | BitBoard::Set[G1];
    WhiteCrossCastleQueen   = BitBoard::Set[D1] | BitBoard::Set[C1] | BitBoard::Set[B1];

    // Backward pawns
    for( i=0; i<64; i++ ) {
        BlackBackwardPawn[i].clear();
        WhiteBackwardPawn[i].clear();

        int file = FileOfSquare(i);
        int rank = RankOfSquare(i);

        /*
            Set a mask to help detect whether a pawn is backwards, i.e. not supported
            by a friendly pawn and not able to get protection even after advancing:

                . . * . .
                . . * . .
                . * P * .
                . * . * .
                . . . . .

            If no friendly pawn is on the * squares then the pawn is backward. At this
            level we don't deal with pawns that can advance two steps: that will be
            handled during pawn evaluation.
        */
        if( (rank > 0) && (rank < 7) ) {
            setBitRectangle( BlackBackwardPawn[i], file-1, rank, file-1, 7 );
            setBitRectangle( BlackBackwardPawn[i], file+1, rank, file+1, 7 );
            setBitRectangle( BlackBackwardPawn[i], file, 0, file, rank-1 );

            setBitRectangle( WhiteBackwardPawn[i], file-1, 0, file-1, rank );
            setBitRectangle( WhiteBackwardPawn[i], file+1, 0, file+1, rank );
            setBitRectangle( WhiteBackwardPawn[i], file, rank+1, file, 7 );
        }
    }

    // Double pawns
    for( i=0; i<64; i++ ) {
        int file = FileOfSquare(i);

        BlackDoublePawn[i] = File[ file ];
        BlackDoublePawn[i].clrBit( i );

        WhiteDoublePawn[i] = File[ file ];
        WhiteDoublePawn[i].clrBit( i );
    }

    /*
    */
    for( i=0; i<64; i++ ) {
        int file = FileOfSquare(i);
        int rank = RankOfSquare(i);

        KingAttack_Danger[i].clear();
        setBitRectangle( KingAttack_Danger[i], file-1, rank-1, file+1, rank+1 );
    }
}
