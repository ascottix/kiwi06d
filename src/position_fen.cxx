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
#include "log.h"
#include "position.h"
#include "san.h"
#include "zobrist.h"

enum {
    fenOk                       = 0,
    fenInvalidPiece             = -1,
    fenInvalidSideToMove        = -2,
    fenInvalidCastleSide        = -3,
    fenSideHasNoKing            = -4,
    fenSideHasTooManyKings      = -5,
    fenSideNotToMoveInCheck     = -6,
    fenInvalidEnPassantSquare   = -7,
};

/* 
    Sets the current position from a FEN string. 
    
    Note: since the move number is not stored in a Position object, this function 
    can parse and return it to the caller if needed.

    Note: there is very little error checking here, because we expect the FEN
    string to come from a program or a knowledgeable user (i.e. using the engine
    in console mode).
*/
int Position::setBoard( const char * s, int * moveNumber )
{
    int     rank    = 7;
    int     square  = rank*8;
    int     nmove   = 0;
    int     enPassantSquare = -1;
    int     halfMoveClock = 0;
    Board   b;

    // Initialize with default values
    sideToPlay = White;
    boardFlags = 0;  // En-passant square not available

    // Board
    b.clear();

    int numOfBlackKings = 0;
    int numOfWhiteKings = 0;

    while( *s != ' ' ) {
        switch( *s ) {
        case '8': b.piece[square++] = None; // Fall thru...
        case '7': b.piece[square++] = None;
        case '6': b.piece[square++] = None;
        case '5': b.piece[square++] = None;
        case '4': b.piece[square++] = None;
        case '3': b.piece[square++] = None;
        case '2': b.piece[square++] = None;
        case '1': b.piece[square++] = None;
            break;

        case '/':
            square = (--rank) * 8;
            break;

        case 'p': b.piece[square++] = BlackPawn;    break;
        case 'n': b.piece[square++] = BlackKnight;  break;
        case 'b': b.piece[square++] = BlackBishop;  break;
        case 'r': b.piece[square++] = BlackRook;    break;
        case 'q': b.piece[square++] = BlackQueen;   break;
        case 'k': b.piece[square++] = BlackKing; numOfBlackKings++; break;
        case 'P': b.piece[square++] = WhitePawn;    break;
        case 'N': b.piece[square++] = WhiteKnight;  break;
        case 'B': b.piece[square++] = WhiteBishop;  break;
        case 'R': b.piece[square++] = WhiteRook;    break;
        case 'Q': b.piece[square++] = WhiteQueen;   break;
        case 'K': b.piece[square++] = WhiteKing; numOfWhiteKings++; break;

        default:
            return fenInvalidPiece;
        }

        s++;
    }

    if( numOfBlackKings == 0 || numOfWhiteKings == 0 ) {
        return fenSideHasNoKing;
    }

    if( numOfBlackKings != 1 || numOfWhiteKings != 1 ) {
        return fenSideHasTooManyKings;
    }

    setBoard( b );
    s++;

    // Side to play
    switch( *s ) {
        case 'b': 
            sideToPlay = Black; 
            hashCode ^= ZobSideToPlay;
            break;
        case 'w': 
            sideToPlay = White; 
            break;
        default:
            return fenInvalidSideToMove;
    }
    
    s++;

    while( *s == ' ' ) s++;

    if( *s ) {
        // Castling availability
        if( *s != '-' ) {
            while( *s != ' ' ) {
                switch( *s ) {
                case 'K': 
                    boardFlags |= WhiteCastleKing;
                    hashCode ^= Zobrist::WhiteCastleKing;
                    break;
                case 'Q': 
                    boardFlags |= WhiteCastleQueen;  
                    hashCode ^= Zobrist::WhiteCastleQueen;
                    break;
                case 'k': 
                    boardFlags |= BlackCastleKing;
                    hashCode ^= Zobrist::BlackCastleKing;
                    break;
                case 'q': 
                    boardFlags |= BlackCastleQueen;  
                    hashCode ^= Zobrist::BlackCastleQueen;
                    break;
                default:
                    return fenInvalidCastleSide;
                }

                s++;
            }
        }
        else {
            s++;
        }

        while( *s == ' ' ) s++;
    }

    // If castling is not available, assume that the corresponding side 
    // has indeed castled (just a guess, but in the evaluation we're going 
    // to penalize a side for not having castled and it's probably better to
    // avoid that for a setup position)
    if( (boardFlags & (WhiteCastleKing | WhiteCastleQueen)) == 0 ) {
        boardFlags |= WhiteHasCastled;
    }

    if( (boardFlags & (BlackCastleKing | BlackCastleQueen)) == 0 ) {
        boardFlags |= BlackHasCastled;
    }

    if( *s ) {
        // En-passant target square
        if( *s != '-' ) {
            enPassantSquare = (s[0]-'a') + (s[1]-'1')*8;

            hashCode ^= Zobrist::EnPassant[ enPassantSquare ];
        }
        s++;

        while( *s == ' ' ) s++;
    }

    if( *s ) {
        // Halfmove clock
        while( (*s >= '0')&&(*s <= '9') ) {
            halfMoveClock = halfMoveClock*10 + (*s - '0');
            s++;
        }

        while( *s == ' ' ) s++;
    }

    if( *s ) {
        // Move number
        while( (*s >= '0')&&(*s <= '9') ) {
            nmove = nmove*10 + (*s - '0');
            s++;
        }
    }

    // Convert move number to half-moves
    nmove *= 2; 
    if( sideToPlay == Black ) {
        nmove += 1;
    }

    // Assign move number if needed
    if( moveNumber != 0 ) {
        *moveNumber = nmove;
    }

    // Initialize check flag
    if( isSideInCheck(sideToPlay) ) {
        boardFlags |= SideToPlayInCheck;
    }

    // Set en-passant square
    if( enPassantSquare >= 0 ) {
        boardFlags |= EnPassantAvailable | (unsigned char) enPassantSquare;
    }

    // Set half-move clock
    boardFlags |= halfMoveClock << HalfMoveClockShift;

    return fenOk;
}

// Returns the current position in FEN notation
int Position::getBoard( char * fen, int moveNumber ) const
{
    for( int rank=7; rank>=0; rank-- ) {
        int empty = 0;

        for( int file=0; file<8; file++ ) {
            int p = board.piece[ FileRankToSquare(file,rank) ];

            if( p == None ) {
                empty++;
            }
            else {
                if( empty > 0 ) {
                    *fen++ = (char) (empty + '0');
                    empty = 0;
                }

                switch( p ) {
                case BlackPawn:   *fen = 'p'; break;
                case BlackKnight: *fen = 'n'; break;
                case BlackBishop: *fen = 'b'; break;
                case BlackRook:   *fen = 'r'; break;
                case BlackQueen:  *fen = 'q'; break;
                case BlackKing:   *fen = 'k'; break;
                case WhitePawn:   *fen = 'P'; break;
                case WhiteKnight: *fen = 'N'; break;
                case WhiteBishop: *fen = 'B'; break;
                case WhiteRook:   *fen = 'R'; break;
                case WhiteQueen:  *fen = 'Q'; break;
                case WhiteKing:   *fen = 'K'; break;
                }

                fen++;
            }
        }

        if( empty > 0 ) {
            *fen++ = (char) (empty + '0');
        }

        *fen++ = (rank == 0) ? ' ' : '/';
    }

    // Side to play
    *fen++ = (sideToPlay == Black) ? 'b' : 'w';
    *fen++ = ' ';

    // Casting availability
    if( boardFlags & (WhiteCastleKing | WhiteCastleQueen | BlackCastleKing | BlackCastleQueen) ) {
        if( boardFlags & WhiteCastleKing ) *fen++ = 'K';
        if( boardFlags & WhiteCastleQueen ) *fen++ = 'Q';
        if( boardFlags & BlackCastleKing ) *fen++ = 'k';
        if( boardFlags & BlackCastleQueen ) *fen++ = 'q';
    }
    else {
        *fen++ = '-';
    }
    *fen++ = ' ';

    // En-passant availability
    if( boardFlags & EnPassantAvailable ) {
        Board::getSquareName( fen, boardFlags & EnPassantRawSquareMask );
        fen += 2;
    }
    else {
        *fen++ = '-';
    }
    *fen++ = ' ';

    // Half-move clock
    int halfMoveClock = getHalfMoveClock();

    if( halfMoveClock > 0 ) {
        fen += sprintf( fen, "%d", halfMoveClock );
    }
    else {
        *fen++ = '-';
    }
    *fen++ = ' ';

    // Move number
    if( moveNumber > 0 ) {
        fen += sprintf( fen, "%d", moveNumber );
    }
    else {
        *fen++ = '-';
    }

    *fen = '\0';

    return fenOk;
}
