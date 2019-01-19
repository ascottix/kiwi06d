/*
    Kiwi
    Move generation

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

#include "attacks.h"
#include "board.h"
#include "bitboard.h"
#include "counters.h"
#include "log.h"
#include "mask.h"
#include "move.h"
#include "movelist.h"
#include "position.h"
#include "score.h"
#include "undoinfo.h"

#define addMoves( moves, from, board )                          \
    {                                                           \
        while( board.isNotZero() ) { \
            int to = bitSearchAndReset(board); \
            moves.add( from, to );                              \
        }                                                       \
    }

#define addWhitePawnMovesNoPromotion( moves, board, from )  \
        while( board.isNotZero() ) { \
            pos = bitSearchAndReset(board); \
            moves.add( from, pos );                              \
    }

#define addWhitePawnMoves( moves, board, from )             \
        while( board.isNotZero() ) { \
            pos = bitSearchAndReset(board); \
        if( pos >= A8 ) {                                   \
            moves.add( from, pos, WhiteBishop );            \
            moves.add( from, pos, WhiteKnight );            \
            moves.add( from, pos, WhiteRook );              \
            moves.add( from, pos, WhiteQueen );             \
        }                                                   \
        else {                                              \
            moves.add( from, pos );                         \
        }                                                   \
    }

#define addBlackPawnMovesNoPromotion( moves, board, from )  \
        while( board.isNotZero() ) { \
            pos = bitSearchAndReset(board); \
            moves.add( from, pos );                              \
    }

#define addBlackPawnMoves( moves, board, from )             \
        while( board.isNotZero() ) { \
            pos = bitSearchAndReset(board); \
        if( pos <= H1 ) {                                   \
            moves.add( from, pos, BlackBishop );            \
            moves.add( from, pos, BlackKnight );            \
            moves.add( from, pos, BlackRook );              \
            moves.add( from, pos, BlackQueen );             \
        }                                                   \
        else {                                              \
            moves.add( from, pos );                         \
        }                                                   \
    }

/*
    Generates all the pseudo-legal moves that the specified side
    can do from the current position, including captures and non-captures.
    Some of the generated moves may leave the king in check: this is handled
    later when we actually try to perform the move on the board.
*/
void Position::generateMoves( MoveList & moves ) const
{
    BitBoard    emptySquares    = ~allPieces;
    BitBoard    enemyOrEmpty;
    BitBoard    bb;
    BitBoard    wb;

    int pos;
    int enPassantSquare = (boardFlags & EnPassantSquareMask) - EnPassantAvailable;

    Counters::callsToGenMoves++;

    if( sideToPlay == Black ) {
        enemyOrEmpty = ~blackPieces;

        // Knights
        wb = blackKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Rooks and queen "rook movement"
        wb = blackQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb  = rookAttacks( pos ) & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = blackQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Pawns: advance
        wb = (blackPawns >> 8) & emptySquares;
        bb = ((wb & Mask::Rank[5]) >> 8) & emptySquares;

        addBlackPawnMoves( moves, wb, pos+8 );
        addBlackPawnMovesNoPromotion( moves, bb, pos+16 );

        // Pawns: captures
        BitBoard    pp = whitePieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        wb = ((blackPawns & Mask::NotFile[0]) >> 9) & pp;
        addBlackPawnMoves( moves, wb, pos+9 );
        wb = ((blackPawns & Mask::NotFile[7]) >> 7) & pp;
        addBlackPawnMoves( moves, wb, pos+7 );

        // King
        wb = Attacks::King[ blackKingSquare ] & enemyOrEmpty;
        addMoves( moves, blackKingSquare, wb );

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function
        if( (boardFlags & BlackCastleKing) && 
            (board.piece[F8] == None) && (board.piece[G8] == None) &&
            !isSquareAttackedBy(F8,White) && !isSquareAttackedBy(G8,White) )
        {
            moves.add( blackKingSquare, G8 );
        }

        if( (boardFlags & BlackCastleQueen) && 
            (board.piece[D8] == None) && (board.piece[C8] == None) && (board.piece[B8] == None) && 
            !isSquareAttackedBy(D8,White) && !isSquareAttackedBy(C8,White) )
        {
            moves.add( blackKingSquare, C8 );
        }
    }
    else { // White
        enemyOrEmpty = ~whitePieces;

        // Knights
        wb = whiteKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Rooks and queen "rook movement"
        wb = whiteQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = rookAttacks( pos ) & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = whiteQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & enemyOrEmpty;
            addMoves( moves, pos, bb );
        }

        // Pawns: advance
        wb = (whitePawns << 8) & emptySquares;
        bb = ((wb & Mask::Rank[2]) << 8) & emptySquares;

        addWhitePawnMoves( moves, wb, pos-8 );
        addWhitePawnMovesNoPromotion( moves, bb, pos-16 );

        // Pawns: captures
        BitBoard    pp = blackPieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        wb = ((whitePawns & Mask::NotFile[7]) << 9) & pp;
        addWhitePawnMoves( moves, wb, pos-9 );
        wb = ((whitePawns & Mask::NotFile[0]) << 7) & pp;
        addWhitePawnMoves( moves, wb, pos-7 );

        // King
        wb = Attacks::King[ whiteKingSquare ] & enemyOrEmpty;
        addMoves( moves, whiteKingSquare, wb );

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function
        if( (boardFlags & WhiteCastleKing) && 
            (board.piece[F1] == None) && (board.piece[G1] == None) &&
            !isSquareAttackedBy(F1,Black) && !isSquareAttackedBy(G1,Black) )
        {
            moves.add( whiteKingSquare, G1 );
        }

        if( (boardFlags & WhiteCastleQueen) && 
            (board.piece[D1] == None) && (board.piece[C1] == None) && (board.piece[B1] == None) &&
            !isSquareAttackedBy(D1,Black) && !isSquareAttackedBy(C1,Black) )
        {
            moves.add( whiteKingSquare, C1 );
        }
    }
}

/*
    Generates all the pseudo-legal captures that the specified side
    can do from the current position.
*/
void Position::generateTactical( MoveList & moves ) const
{
    BitBoard    bb;
    BitBoard    wb;

    int pos;
    int enPassantSquare = (boardFlags & EnPassantSquareMask) - EnPassantAvailable;

    if( sideToPlay == Black ) {
        // King
        wb = Attacks::King[ blackKingSquare ] & whitePieces;
        addMoves( moves, blackKingSquare, wb );

        // Rooks and queen "rook movement"
        wb = blackQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb  = rookAttacks( pos ) & whitePieces;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = blackQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & whitePieces;
            addMoves( moves, pos, bb );
        }

        // Knights
        wb = blackKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & whitePieces;
            addMoves( moves, pos, bb );
        }

        // Pawns: captures
        BitBoard    pp = whitePieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        wb = ((blackPawns & Mask::NotFile[0]) >> 9) & pp;
        addBlackPawnMoves( moves, wb, pos+9 );
        wb = ((blackPawns & Mask::NotFile[7]) >> 7) & pp;
        addBlackPawnMoves( moves, wb, pos+7 );

        // Pawns: promotions
        wb = ((blackPawns & Mask::Rank[1]) >> 8) & (~allPieces);

        addBlackPawnMoves( moves, wb, pos+8 );
    }
    else {
        // King
        wb = Attacks::King[ whiteKingSquare ] & blackPieces;
        addMoves( moves, whiteKingSquare, wb );

        // Rooks and queen "rook movement"
        wb = whiteQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb  = rookAttacks( pos ) & blackPieces;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = whiteQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & blackPieces;
            addMoves( moves, pos, bb );
        }

        // Knights
        wb = whiteKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & blackPieces;
            addMoves( moves, pos, bb );
        }

        // Pawns: captures
        BitBoard    pp = blackPieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        wb = ((whitePawns & Mask::NotFile[7]) << 9) & pp;
        addWhitePawnMoves( moves, wb, pos-9 );
        wb = ((whitePawns & Mask::NotFile[0]) << 7) & pp;
        addWhitePawnMoves( moves, wb, pos-7 );

        // Pawns: promotions
        wb = ((whitePawns & Mask::Rank[6]) << 8) & (~allPieces);

        addWhitePawnMoves( moves, wb, pos-8 );
    }
}

/*
    Generates all the pseudo-legal non-captures that the specified side
    can do from the current position.
*/

// TODO: do not generate pawn promotions here
void Position::generateNonTactical( MoveList & moves ) const
{
    BitBoard    emptySquares    = ~allPieces;
    BitBoard    bb;
    BitBoard    wb;

    int         pos;

    Counters::callsToGenMoves++;

    if( sideToPlay == Black ) {
        // Knights
        wb = blackKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Rooks and queen "rook movement"
        wb = blackQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb  = rookAttacks( pos ) & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = blackQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Pawns: advance
        wb = (blackPawns >> 8) & emptySquares;
        bb = ((wb & Mask::Rank[5]) >> 8) & emptySquares;
        addBlackPawnMoves( moves, wb, pos+8 );
        addBlackPawnMovesNoPromotion( moves, bb, pos+16 );

        // King
        wb = Attacks::King[ blackKingSquare ] & emptySquares;
        addMoves( moves, blackKingSquare, wb );

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function)
        if( (boardFlags & BlackCastleKing) &&
            (board.piece[F8] == None) && (board.piece[G8] == None) &&
            !isSquareAttackedBy(F8,White) && !isSquareAttackedBy(G8,White) )
        {
            moves.add( blackKingSquare, G8 );
        }

        if( (boardFlags & BlackCastleQueen) &&
            (board.piece[D8] == None) && (board.piece[C8] == None) && (board.piece[B8] == None) &&
            !isSquareAttackedBy(D8,White) && !isSquareAttackedBy(C8,White) )
        {
            moves.add( blackKingSquare, C8 );
        }
    }
    else { // White
        // Knights
        wb = whiteKnights;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = Attacks::Knight[pos] & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Rooks and queen "rook movement"
        wb = whiteQueensRooks;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb  = rookAttacks( pos ) & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Bishops and queen "bishop movement"
        wb = whiteQueensBishops;
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            bb = bishopAttacks( pos ) & emptySquares;
            addMoves( moves, pos, bb );
        }

        // Pawns: advance
        wb = (whitePawns << 8) & emptySquares;
        bb = ((wb & Mask::Rank[2]) << 8) & emptySquares;

        addWhitePawnMoves( moves, wb, pos-8 );
        addWhitePawnMovesNoPromotion( moves, bb, pos-16 );

        // King
        wb = Attacks::King[ whiteKingSquare ] & emptySquares;
        addMoves( moves, whiteKingSquare, wb );

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function)
        if( (boardFlags & WhiteCastleKing) &&
            (board.piece[F1] == None) && (board.piece[G1] == None) &&
            !isSquareAttackedBy(F1,Black) && !isSquareAttackedBy(G1,Black) )
        {
            moves.add( whiteKingSquare, G1 );
        }

        if( (boardFlags & WhiteCastleQueen) &&
            (board.piece[D1] == None) && (board.piece[C1] == None) && (board.piece[B1] == None) &&
            !isSquareAttackedBy(D1,Black) && !isSquareAttackedBy(C1,Black) )
        {
            moves.add( whiteKingSquare, C1 );
        }
    }
}

/*
    Generates all the pseudo-legal moves that the specified side can do
    when it's under check.

    Note that some of the generated moves may still leave the king in check,
    however this function usually produces much less moves than the generic
    generation routine.
*/
void Position::generateCheckEscapes( MoveList & moves ) const
{
    BitBoard    bb, wb;
    BitBoard    enemyOrEmpty;
    BitBoard    attacks;
    int         king;
    int         enPassantSquare = (boardFlags & EnPassantSquareMask) - EnPassantAvailable;

    if( sideToPlay == Black ) {
        enemyOrEmpty = ~blackPieces;
        king = blackKingSquare;
        attacks = getAttacksToSquare( king, White );
    }
    else {
        enemyOrEmpty = ~whitePieces;
        king = whiteKingSquare;
        attacks = getAttacksToSquare( king, Black );
    }

    int posAttacker1 = bitScanAndResetForward(attacks);
    int posAttacker2 = bitScanAndResetForward(attacks);

    if( posAttacker2 < 0 ) {
        // There is only one piece giving check: try to interpose a piece
        // between it and the king, and also to capture the attacker
        BitBoard    validSquares = Attacks::SquaresBetween[posAttacker1][king];
        validSquares.setBit( posAttacker1 );

        BitBoard    emptySquares = ~allPieces;
        int         pos;

        // Now we can use the standard move generation code, but the reduced 
        // set of valid destination squares should produce much less moves
        if( sideToPlay == Black ) {
            // Knights
            wb = blackKnights;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb = Attacks::Knight[pos] & validSquares;
                addMoves( moves, pos, bb );
            }

            // Rooks and queen "rook movement"
            wb = blackQueensRooks;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb  = rookAttacks( pos ) & validSquares;
                addMoves( moves, pos, bb );
            }

            // Bishops and queen "bishop movement"
            wb = blackQueensBishops;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb = bishopAttacks( pos ) & validSquares;
                addMoves( moves, pos, bb );
            }

            // Pawns: advance
            wb = (blackPawns >> 8) & emptySquares;
            bb = (((wb & Mask::Rank[5]) >> 8) & emptySquares) & validSquares;
            wb &= validSquares;
            addBlackPawnMoves( moves, wb, pos+8 );
            addBlackPawnMovesNoPromotion( moves, bb, pos+16 );

            // Pawns: captures
            BitBoard    pp = whitePieces;
            pp &= validSquares;
            if( enPassantSquare >= 0 ) {
                pp.setBit( enPassantSquare );
            }

            wb = ((blackPawns & Mask::NotFile[0]) >> 9) & pp;
            addBlackPawnMoves( moves, wb, pos+9 );
            wb = ((blackPawns & Mask::NotFile[7]) >> 7) & pp;
            addBlackPawnMoves( moves, wb, pos+7 );
        }
        else {
            // Knights
            wb = whiteKnights;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb = Attacks::Knight[pos] & validSquares;
                addMoves( moves, pos, bb );
            }

            // Rooks and queen "rook movement"
            wb = whiteQueensRooks;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb  = rookAttacks( pos ) & validSquares;
                addMoves( moves, pos, bb );
            }

            // Bishops and queen "bishop movement"
            wb = whiteQueensBishops;
            while( wb.isNotZero() ) {
                pos = bitSearchAndReset( wb );
                bb = bishopAttacks( pos ) & validSquares;
                addMoves( moves, pos, bb );
            }

            // Pawns: advance
            wb = (whitePawns << 8) & emptySquares;
            bb = (((wb & Mask::Rank[2]) << 8) & emptySquares) & validSquares;
            wb &= validSquares;
            addWhitePawnMoves( moves, wb, pos-8 );
            addWhitePawnMovesNoPromotion( moves, bb, pos-16 );

            // Pawns: captures
            BitBoard    pp = blackPieces;
            pp &= validSquares;
            if( enPassantSquare >= 0 ) {
                pp.setBit( enPassantSquare );
            }

            wb = ((whitePawns & Mask::NotFile[7]) << 9) & pp;
            addWhitePawnMoves( moves, wb, pos-9 );
            wb = ((whitePawns & Mask::NotFile[0]) << 7) & pp;
            addWhitePawnMoves( moves, wb, pos-7 );
        }
    }

    // King moves must always be tried when under check
    bb = Attacks::King[ king ] & enemyOrEmpty;
    addMoves( moves, king, bb );
}

/*
    Generates all the valid moves from the current position.

    If a target square is specified (by a valid square number),
    then this function generates only the moves to that square
    (useful for GUI's and SAN parsing).
*/
void Position::generateValidMoves( MoveList & moves, int to ) const
{
    Position pos( *this );
    UndoInfo ui( pos );
    MoveList all;

    moves.reset();

    // Generate moves
    if( boardFlags & SideToPlayInCheck ) {
        generateCheckEscapes( all );
    }
    else {
        if( to >= 0 ) {
            generateMovesToSquare( all, to );
        }
        else {
            generateMoves( all );
        }
    }

    // Trim out illegal moves
    for( int i=0; i<all.count(); i++ ) {
        if( (to < 0) || (all.move[i].getTo() == to) ) {
            if( pos.doMove( all.move[i] ) == 0 ) {
                moves.add( all.move[i] );
            }

            pos.undoMove( all.move[i], ui );
        }
    }
}

/*
    Generates the list of pseudo-legal moves that target 
    the specified square.

    Note: this function is quite useless, I hacked it just to see if PGN parsing
    would benefit from it. Since it does provide a noticeable speedup (about 25-30%
    on a typical file) I'm leaving it in.
*/
void Position::generateMovesToSquare( MoveList & moves, int to ) const
{
    BitBoard    emptySquares    = ~allPieces;
    BitBoard    bb;
    BitBoard    wb;

    int pos;
    int enPassantSquare = (boardFlags & EnPassantSquareMask) - EnPassantAvailable;

    Counters::callsToGenMoves++;

    if( sideToPlay == Black ) {
        // Knights
        wb = blackKnights & Attacks::Knight[to];
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Rooks and queen "rook movement"
        wb = blackQueensRooks & rookAttacks( to );
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Bishops and queen "bishop movement"
        wb = blackQueensBishops & bishopAttacks( to );
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Pawns: advance
        wb = (blackPawns >> 8) & emptySquares;
        bb = ((wb & Mask::Rank[5]) >> 8) & emptySquares;

        wb &= BitBoard::Set[ to ];
        bb &= BitBoard::Set[ to ];

        addBlackPawnMoves( moves, wb, pos+8 );
        addBlackPawnMovesNoPromotion( moves, bb, pos+16 );

        // Pawns: captures
        BitBoard    pp = whitePieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        pp &= BitBoard::Set[ to ];

        wb = ((blackPawns & Mask::NotFile[0]) >> 9) & pp;
        addBlackPawnMoves( moves, wb, pos+9 );
        wb = ((blackPawns & Mask::NotFile[7]) >> 7) & pp;
        addBlackPawnMoves( moves, wb, pos+7 );

        // King
        if( Attacks::King[ blackKingSquare ].getBit( to ) ) {
            moves.add( blackKingSquare, to );
        }

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function
        if( (to == G8) &&
            (boardFlags & BlackCastleKing) && 
            (board.piece[F8] == None) && (board.piece[G8] == None) &&
            !isSquareAttackedBy(F8,White) && !isSquareAttackedBy(G8,White) )
        {
            moves.add( blackKingSquare, G8 );
        }

        if( (to == C8) &&
            (boardFlags & BlackCastleQueen) && 
            (board.piece[D8] == None) && (board.piece[C8] == None) && (board.piece[B8] == None) && 
            !isSquareAttackedBy(D8,White) && !isSquareAttackedBy(C8,White) )
        {
            moves.add( blackKingSquare, C8 );
        }
    }
    else { // White
        // Knights
        wb = whiteKnights & Attacks::Knight[to];
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Rooks and queen "rook movement"
        wb = whiteQueensRooks & rookAttacks( to );
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Bishops and queen "bishop movement"
        wb = whiteQueensBishops & bishopAttacks( to );
        while( wb.isNotZero() ) {
            pos = bitSearchAndReset( wb );
            moves.add( pos, to );
        }

        // Pawns: advance
        wb = (whitePawns << 8) & emptySquares;
        bb = ((wb & Mask::Rank[2]) << 8) & emptySquares;

        wb &= BitBoard::Set[ to ];
        bb &= BitBoard::Set[ to ];

        addWhitePawnMoves( moves, wb, pos-8 );
        addWhitePawnMovesNoPromotion( moves, bb, pos-16 );

        // Pawns: captures
        BitBoard    pp = blackPieces;
        if( enPassantSquare >= 0 ) pp.setBit( enPassantSquare );

        pp &= BitBoard::Set[ to ];

        wb = ((whitePawns & Mask::NotFile[7]) << 9) & pp;
        addWhitePawnMoves( moves, wb, pos-9 );
        wb = ((whitePawns & Mask::NotFile[0]) << 7) & pp;
        addWhitePawnMoves( moves, wb, pos-7 );

        // King
        if( Attacks::King[ whiteKingSquare ].getBit( to ) ) {
            moves.add( whiteKingSquare, to );
        }

        // Castling (note that there is no need to verify that the king is not in check,
        // because that would be handled by a specific move-generation function
        if( (to == G1) &&
            (boardFlags & WhiteCastleKing) && 
            (board.piece[F1] == None) && (board.piece[G1] == None) &&
            !isSquareAttackedBy(F1,Black) && !isSquareAttackedBy(G1,Black) )
        {
            moves.add( whiteKingSquare, G1 );
        }

        if( (to == C1) &&
            (boardFlags & WhiteCastleQueen) && 
            (board.piece[D1] == None) && (board.piece[C1] == None) && (board.piece[B1] == None) &&
            !isSquareAttackedBy(D1,Black) && !isSquareAttackedBy(C1,Black) )
        {
            moves.add( whiteKingSquare, C1 );
        }
    }
}
