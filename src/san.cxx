/*
    Kiwi
    SAN (Standard Algebraic Notation) move handling

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
#include <ctype.h>
#include <string.h>

#include "board.h"
#include "log.h"
#include "move.h"
#include "movelist.h"
#include "san.h"
#include "position.h"

// Additional specifier if needed
enum {
    SpecifyNone = 0x00,
    SpecifyFile = 0x01,
    SpecifyRank = 0x02
};

char SAN::nameOfKnight  = 'N';
char SAN::nameOfBishop  = 'B';
char SAN::nameOfRook    = 'R';
char SAN::nameOfQueen   = 'Q';
char SAN::nameOfKing    = 'K';

static inline bool isCastleChar( char c )
{
    return (c == 'O') || (c == 'o') || (c == '0');
}

int SAN::getPieceByName( char n )
{
    if( n == nameOfKnight ) return Knight;
    if( n == nameOfBishop ) return Bishop;
    if( n == nameOfRook   ) return Rook;
    if( n == nameOfQueen  ) return Queen;
    if( n == nameOfKing   ) return King;

    return None;
}

char SAN::getNameOfPiece( int piece )
{
    switch( PieceType(piece) ) {
    case Knight: return nameOfKnight;
    case Bishop: return nameOfBishop;
    case Rook:   return nameOfRook;
    case Queen:  return nameOfQueen;
    case King:   return nameOfKing;
    }

    return 0;
}

void SAN::setNameOfPiece( char n, int piece )
{
    switch( PieceType(piece) ) {
    case Knight: nameOfKnight = n; break;
    case Bishop: nameOfBishop = n; break;
    case Rook:   nameOfRook   = n; break;
    case Queen:  nameOfQueen  = n; break;
    case King:   nameOfKing   = n; break;
    }
}

int SAN::textToMove( Move * move, const Position & pos, const char * text )
{
    int i;

    if( move != 0 ) {
        *move = Move::Null;
    }

    // Check for castling
    if( isCastleChar(text[0]) && (text[1] == '-') && isCastleChar(text[2]) ) {
        Move    x;

        if( (text[3]=='-') && isCastleChar(text[4]) ) {
            x = pos.sideToPlay == Black ? Move(E8,C8) : Move(E1,C1);
        }
        else {
            x = pos.sideToPlay == Black ? Move(E8,G8) : Move(E1,G1);
        }

        if( move != 0 ) {
            *move = x;
        }

        return sanOk;
    }

    // Go to the end of the string, as it's better to parse backwards
    const char * end = text;

    while( *end ) {
        ++end;
    }

    --end;

    // Skip all useless info such as check signs, marks and so on
    while( end > text ) {
        if( getPieceByName(toupper(*end)) != None )
            break;

        if( (*end >= '1')&&(*end <= '8') )
            break;

        --end;
    }

    // Get the promoted piece (if any)
    int promoted = getPieceByName(toupper(*end));

    if( promoted != None ) {
        --end;
        if( (end > text) && (*end == '=') ) --end;

        // Allow any case for the promoted piece here
        promoted = PieceType(promoted) | (pos.sideToPlay == Black ? Black : White);
    }

    // Get the destination square
    int torank;
    int tofile;

    if( (end <= text) || (*end < '1') || (*end > '8') ) {
        return sanMalformedMoveText;
    }

    torank = (int)(*end - '1');

    --end;

    if( (*end < 'a') || (*end > 'h') ) {
        return sanMalformedMoveText;
    }

    tofile = (int)(tolower(*end) - 'a');

    // Skip over the capture mark if any
    if( (end > text) && ((tolower(end[-1]) == 'x') || (end[-1] == ':')) ) {
        --end;

        if( end <= text ) {
            return sanMalformedMoveText;
        }
    }

    // Parse all remaining info
    int fromrank = -1;
    int fromfile = -1;
    int moved = Pawn;

    if( end > text ) {
        --end;

        if( (*end >= '1') && (*end <= '8') )
            fromrank = (*end--) - '1';

        if( (*end >= 'a') && (*end <= 'h') )
            fromfile = tolower(*end--) - 'a';

        if( end >= text ) {
            moved = getPieceByName( *end );

            if( moved == None ) {
                return sanMalformedMoveText;
            }
        }
    }

    if( (fromrank >= 0) && (fromfile >= 0) ) {
        moved = PieceType( pos.board.piece[ FileRankToSquare(fromfile,fromrank) ] );
    }

    // Make sure we parsed it all
    if( end > text ) {
        return sanMalformedMoveText;
    }

    // Now build the move (the starting square is missing for now)
    Move    m( 0, FileRankToSquare( tofile, torank ) );

    if( promoted != None ) {
        m.setPromoted( promoted );
    }

    // Build a list of all moves targeting the destination square
    MoveList    hit;

    pos.generateValidMoves( hit, m.getTo() );

    if( hit.count() == 0 ) {
        return sanInvalidMove;
    }

    // If there are multiple moves to the target square, use collected info to disambiguate
    for( i=0; i<hit.count(); i++ ) {
        Move    x = hit.move[i];
        int     f = x.getFrom();

        if( x.getPromoted() != m.getPromoted() )
            continue;

        if( PieceType(pos.board.piece[f]) != moved )
            continue;

        if( (fromfile >= 0) && (fromfile != FileOfSquare(f)) )
            continue;

        if( (fromrank >= 0) && (fromrank != RankOfSquare(f)) )
            continue;

        if( move != 0 ) {
            *move = x;
        }

        return sanOk;
    }

    return sanAmbiguousMove;
}

int SAN::moveToText( char * text, const Position & pos, Move move )
{
    int spec = SpecifyNone;

    // Refute illegal moves
    Position p( pos );

    if( ! p.isValidMove( move ) || p.doMove( move ) ) {
        *text++ = 'n';
        *text++ = '/';
        *text++ = 'a';
        *text = 0;

        return sanInvalidMove;
    }

    // Setup move fields
    int moved = pos.board.piece[ move.getFrom() ];

    // Check the move list to see if the shortest form is ambiguous
    if( ! move.isPromotion() ) {
        MoveList    all;

        pos.generateValidMoves( all );

        for( int i=0; i<all.count(); i++ ) {
            Move m = all.move[i];

            if( (m != move) && (pos.board.piece[ m.getFrom() ] == moved) && (m.getTo() == move.getTo()) ) {
                // We need to specify some additional info for this move
                if( FileOfSquare(m.getFrom()) == FileOfSquare(move.getFrom()) ) {
                    spec |= SpecifyRank;
                }

                if( RankOfSquare(m.getFrom()) == RankOfSquare(move.getFrom()) ) {
                    spec |= SpecifyFile;
                }

                if( spec == 0 ) {
                    // Anything will do, but standard uses file
                    spec |= SpecifyFile;
                }
            }
        }
    }

    // Convert to text
    int     d = move.getTo() - move.getFrom();

    if( (PieceType(moved)==King) && ((d==2) || (d==-2)) ) {
        // Castling
        *text++ = 'O'; 
        *text++ = '-'; 
        *text++ = 'O';

        if( (move.getTo()==C1) || (move.getTo()==C8) ) { 
            *text++ = '-'; 
            *text++ = 'O'; 
        }
    }
    else {
        if( PieceType(moved) != Pawn ) {
            *text++ = getNameOfPiece( moved );
        }
        else if( move.getCaptured() != None ) {
            // Pawn capture needs to always specify the starting file
            spec = SpecifyFile;
        }

        if( spec & SpecifyFile ) {
            *text++ = 'a'+FileOfSquare(move.getFrom()); 
        }

        if( spec & SpecifyRank ) {
            *text++ = '1'+RankOfSquare(move.getFrom()); 
        }

        if( move.getCaptured() != None ) {
            *text++ = 'x';
        }

        *text++ = 'a'+FileOfSquare( move.getTo() );
        *text++ = '1'+RankOfSquare( move.getTo() );

        if( move.getPromoted() != None ) {
            *text++ = '=';
            *text++ = getNameOfPiece( move.getPromoted() );
        }
    }

    if( p.isSideInCheck( p.sideToPlay ) ) {
        *text++ = '+';
    }

    *text = 0;

    return sanOk;
}

const char * SAN::moveToText( const Position & pos, Move move )
{
    static char buf[8];

    if( moveToText( buf, pos, move ) != 0 ) {
        strcpy( buf, "n/a" );
    }

    return buf;
}

int SAN::moveToTextLong( char * text, const Position & pos, Move move )
{
    // Refute illegal moves
    Position p( pos );

    if( ! p.isValidMove( move ) || p.doMove( move ) ) {
        *text++ = 'n';
        *text++ = '/';
        *text++ = 'a';
        *text = 0;

        return sanInvalidMove;
    }

    // Convert to text
    int moved   = pos.board.piece[ move.getFrom() ];

    int d       = move.getTo() - move.getFrom();

    if( (PieceType(moved)==King) && ((d==2) || (d==-2)) ) {
        // Castling
        *text++ = 'O'; 
        *text++ = '-'; 
        *text++ = 'O';

        if( (move.getTo()==C1) || (move.getTo()==C8) ) { 
            *text++ = '-'; 
            *text++ = 'O'; 
        }
    }
    else {
        Board::getSquareName( text, move.getFrom() );
        text += 2;
        Board::getSquareName( text, move.getTo() );
        text += 2;
        
        if( move.isPromotion() ) {
            *text++ = tolower( getNameOfPiece( move.getPromoted() ) );
        }
    }

    *text = 0;

    return sanOk;
}
