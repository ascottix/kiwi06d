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
#include "attacks.h"
#include "board.h"
#include "log.h"

BitBoard    Attacks::BlackPawn[64];
BitBoard    Attacks::WhitePawn[64];
BitBoard    Attacks::BishopA1H8[64][256];
BitBoard    Attacks::BishopA8H1[64][256];
BitBoard    Attacks::King[64];
BitBoard    Attacks::Knight[64];
BitBoard    Attacks::RookRank[64][256];
BitBoard    Attacks::RookFile[64][256];
BitBoard    Attacks::Rook[64];
BitBoard    Attacks::Bishop[64];
BitBoard    Attacks::BlackPawnCouldAttack[64];
BitBoard    Attacks::WhitePawnCouldAttack[64];
BitBoard    Attacks::SquaresBetween[64][64];
int         Attacks::Distance[64][64];
int         Attacks::KingDistance[64][64];
char        Attacks::Direction[64][64];
char        Attacks::DirectionEx[64][64];   // Extended info

/*
*/
void Attacks::initialize()
{
    int i, j;

    for( i=0; i<64; i++ ) {
        int rank = RankOfSquare(i);
        int file = FileOfSquare(i);

        // Knight
        Knight[i].clear();
        setBitByFileRank( Knight[i], file-1, rank-2 );
        setBitByFileRank( Knight[i], file-1, rank+2 );
        setBitByFileRank( Knight[i], file-2, rank-1 );
        setBitByFileRank( Knight[i], file-2, rank+1 );
        setBitByFileRank( Knight[i], file+1, rank-2 );
        setBitByFileRank( Knight[i], file+1, rank+2 );
        setBitByFileRank( Knight[i], file+2, rank-1 );
        setBitByFileRank( Knight[i], file+2, rank+1 );

        // King
        King[i].clear();
        setBitByFileRank( King[i], file-1, rank-1 );
        setBitByFileRank( King[i], file-1, rank   );
        setBitByFileRank( King[i], file-1, rank+1 );
        setBitByFileRank( King[i], file  , rank-1 );
        setBitByFileRank( King[i], file  , rank+1 );
        setBitByFileRank( King[i], file+1, rank-1 );
        setBitByFileRank( King[i], file+1, rank   );
        setBitByFileRank( King[i], file+1, rank+1 );

        // Black pawn
        BlackPawn[i].clear();
        setBitByFileRank( BlackPawn[i], file+1, rank-1 );
        setBitByFileRank( BlackPawn[i], file-1, rank-1 );

        // White pawn
        WhitePawn[i].clear();
        setBitByFileRank( WhitePawn[i], file+1, rank+1 );
        setBitByFileRank( WhitePawn[i], file-1, rank+1 );
    }

    // Rook: ranks
    for( i=0; i<64; i++ ) {
        int rank = RankOfSquare(i);
        int file = FileOfSquare(i);

        for( int r=0; r<256; r++ ) {
            RookRank[i][r].clear();

            for( j = file-1; j >= 0; j-- ) {
                RookRank[i][r].setBit( rank*8+j );             
                if( r & (1 << j) ) break;
            }
            for( j = file+1; j < 8; j++ ) {
                RookRank[i][r].setBit( rank*8+j );
                if( r & (1 << j) ) break;
            }
        }
    }

    // Rook: files
    for( i=0; i<64; i++ ) {
        int rank = i / 8;
        int file = i % 8;

        for( int r=0; r<256; r++ ) {
            RookFile[i][r].clear();

            for( j = rank-1; j >= 0; j-- ) {
                RookFile[i][r].setBit( file+8*j );             
                if( r & (1 << j) ) break;
            }
            for( j = rank+1; j < 8; j++ ) {
                RookFile[i][r].setBit( file+8*j );
                if( r & (1 << j) ) break;
            }
        }
    }

    // Bishop: A1-H8 diagonal
    for( i=0; i<64; i++ ) {
        int rank = RankOfSquare(i);
        int file = FileOfSquare(i);
        int dpos = 0;
        int dlen = 0;

        // Compute position within diagonal
        while( isValidSquare( rank-dpos, file-dpos ) ) dpos++;
        dpos--;

        // Move to beginning of diagonal
        rank -= dpos;
        file -= dpos;

        // Compute diagonal length
        while( isValidSquare( rank+dlen, file+dlen ) ) dlen++;

        for( int r=0; r<256; r++ ) {
            BishopA1H8[i][r].clear();

            for( j = dpos-1; j >= 0; j-- ) {
                setBitByFileRank( BishopA1H8[i][r], file+j, rank+j );
                if( r & (1 << j) ) break;
            }
            for( j = dpos+1; j < dlen; j++ ) {
                setBitByFileRank( BishopA1H8[i][r], file+j, rank+j );
                if( r & (1 << j) ) break;
            }
        }
    }

    // Bishop: A8-H1 diagonal
    for( i=0; i<64; i++ ) {
        int rank = RankOfSquare(i);
        int file = FileOfSquare(i);
        int dpos = 0;
        int dlen = 0;

        // Compute position within diagonal
        while( isValidSquare( rank+dpos, file-dpos ) ) dpos++;
        dpos--;

        // Move to beginning of diagonal
        rank += dpos;
        file -= dpos;

        // Compute diagonal length
        while( isValidSquare( rank-dlen, file+dlen ) ) dlen++;

        for( int r=0; r<256; r++ ) {
            BishopA8H1[i][r].clear();

            for( j = dpos-1; j >= 0; j-- ) {
                setBitByFileRank( BishopA8H1[i][r], file+j, rank-j );
                if( r & (1 << j) ) break;
            }
            for( j = dpos+1; j < dlen; j++ ) {
                setBitByFileRank( BishopA8H1[i][r], file+j, rank-j );
                if( r & (1 << j) ) break;
            }
        }
    }

    // Rook and bishop (regardless of interposing pieces)
    for( i=0; i<64; i++ ) {
        Rook[i] = RookFile[i][0] | RookRank[i][0];
        Bishop[i] = BishopA1H8[i][0] | BishopA8H1[i][0];
    }

    // Pawns that can attack a square, possibly by advancing first
    for( i=0; i<64; i++ ) {
        int rank = RankOfSquare(i);
        int file = FileOfSquare(i);

        BlackPawnCouldAttack[i].clear();
        for( j=rank+1; j<=6; j++ ) {
            setBitByFileRank( BlackPawnCouldAttack[i], file-1, j );
            setBitByFileRank( BlackPawnCouldAttack[i], file+1, j );
        }

        WhitePawnCouldAttack[i].clear();
        for( j=rank-1; j>=1; j-- ) {
            setBitByFileRank( WhitePawnCouldAttack[i], file-1, j );
            setBitByFileRank( WhitePawnCouldAttack[i], file+1, j );
        }
    }

    // Distance
    for( i=0; i<64; i++ ) {
        int rank1 = RankOfSquare(i);
        int file1 = FileOfSquare(i);

        for( j=0; j<64; j++ ) {
            int rank2 = RankOfSquare(j);
            int file2 = FileOfSquare(j);

            int rdist = rank2 - rank1;
            int fdist = file2 - file1;

            if( rdist < 0 ) rdist = -rdist;
            if( fdist < 0 ) fdist = -fdist;

            Distance[i][j] = rdist + fdist;
            KingDistance[i][j] = rdist > fdist ? rdist : fdist;
        }
    }

    // Direction
    for( i=0; i<64; i++ ) {
        int rank1 = RankOfSquare(i);
        int file1 = FileOfSquare(i);

        for( j=0; j<64; j++ ) {
            Direction[i][j] = DirNone;
            DirectionEx[i][j] = DirNone;

            if( i == j ) {
                continue;
            }

            int rank2 = RankOfSquare(j);
            int file2 = FileOfSquare(j);

            if( rank1 == rank2 ) {
                Direction[i][j] = DirRank;
                DirectionEx[i][j] = file1 < file2 ? DirRank_A : DirRank_H;
            }
            else if( file1 == file2 ) {
                Direction[i][j] = DirFile;
                DirectionEx[i][j] = rank1 < rank2 ? DirFile_1 : DirFile_8;
            }
            else if( (file1-file2) == (rank1-rank2) ) {
                Direction[i][j] = DirA1H8;
                DirectionEx[i][j] = rank1 < rank2 ? DirA1H8_A1 : DirA1H8_H8;
            }
            else if( (file1-file2) == (rank2-rank1) ) {
                Direction[i][j] = DirA8H1;
                DirectionEx[i][j] = rank1 < rank2 ? DirA8H1_H1 : DirA8H1_A8;
            }
        }
    }

    // Interposing squares
    for( i=0; i<64; i++ ) {
        int rank1 = RankOfSquare(i);
        int file1 = FileOfSquare(i);

        for( j=0; j<64; j++ ) {
            int rank2 = RankOfSquare(j);
            int file2 = FileOfSquare(j);

            int x;
            int y;

            SquaresBetween[i][j].clear();

            switch( Direction[i][j] ) {
            case DirRank:
                // Same rank
                for( x=file1+1; x<file2; x++ ) setSquareBetweenBit( i, j, x, rank1 );
                for( x=file2+1; x<file1; x++ ) setSquareBetweenBit( i, j, x, rank1 );
                break;
            case DirFile:
                // Same file
                for( y=rank1+1; y<rank2; y++ ) setSquareBetweenBit( i, j, file1, y );
                for( y=rank2+1; y<rank1; y++ ) setSquareBetweenBit( i, j, file1, y );
                break;
            case DirA1H8:
                // Both on A1-H8 diagonal
                for(  x=file1+1, y=rank1+1; x<file2 && y<rank2; x++, y++ )
                    setSquareBetweenBit( i, j, x, y );
                for(  x=file2+1, y=rank2+1; x<file1 && y<rank1; x++, y++ )
                    setSquareBetweenBit( i, j, x, y );
                break;
            case DirA8H1:
                // Both on A8-H1 diagonal
                for(  x=file1+1, y=rank1-1; x<file2 && y>rank2; x++, y-- )
                    setSquareBetweenBit( i, j, x, y );
                for(  x=file2+1, y=rank2-1; x<file1 && y>rank1; x++, y-- )
                    setSquareBetweenBit( i, j, x, y );
                break;
            }
        }
    }
}

bool Attacks::isValidSquare( int file, int rank )
{
    return (file >= 0) && (file < 8) && (rank >= 0) && (rank < 8);
}

void Attacks::setBitByFileRank( BitBoard & b, int file, int rank )
{
    if( isValidSquare(file,rank) ) {
        b.setBit( FileRankToSquare( file, rank ) );
    }
}

void Attacks::setSquareBetweenBit( int src, int dst, int file, int rank )
{
    setBitByFileRank( SquaresBetween[src][dst], file, rank );
}
