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
#include <stdlib.h>

#include "assert.h"
#include "board.h"
#include "log.h"
#include "movelist.h"
#include "position.h"
#include "score.h"
#include "undoinfo.h"

/**
    Undoes the move done by doMove().

    Theoretically here we could simply copy all of the parent position, however
    it is much faster in practice to undo only the actions that were actually
    performed (to avoid copying unchanged data).

    Running perft(6) on the starting position yields 1240 KNps using undoMove()
    versus 1068 KNps when the entire object is backed up, speeding the operation
    up by almost 14% (test run on a 700 MHz Athlon).

    Note: after several fixes and optimizations, perft(6) now yields 2565 KNps
    on the same machine...
*/
void Position::undoMove( const Move & m, const UndoInfo & info )
{
    int         from    = m.getFrom();
    int         to      = m.getTo();

    // Restore information common to all pieces
    allPieces       = info.allPieces;
    allPiecesTA1H8  = info.allPiecesTA1H8;
    allPiecesDA1H8  = info.allPiecesDA1H8;
    allPiecesDA8H1  = info.allPiecesDA8H1;

    hashCode        = info.hashCode;
    pawnHashCode    = info.pawnHashCode;

    boardFlags      = info.boardFlags;
    materialSignature=info.matSignature;

    pstScoreOpening = info.pstScoreOpening;
    pstScoreEndgame = info.pstScoreEndgame;

    sideToPlay      = OppositeSide( sideToPlay );

    // Restore information specific to the moved piece
    int movedPiece;
    int promotedPiece = m.getPromoted();

    if( promotedPiece != None ) {
        // Remove promoted piece and replace it with a pawn
        if( (promotedPiece & PieceSideMask) == Black ) {
            // Black
            materialScore -= (Score::Piece[promotedPiece]+Score::Pawn);

            blackPawns.setBit(to);  // To emulate a simple pawn move
            movedPiece = BlackPawn;

            blackPieceCount += AllPawns_Unit;

            switch( promotedPiece ) {
            case BlackKnight:
                blackPieceCount -= AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
                blackKnights.clrBit(to);
                break;
            case BlackBishop:
                blackPieceCount -= AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
                blackQueensBishops.clrBit(to);
                break;
            case BlackRook:
                blackPieceCount -= AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
                blackQueensRooks.clrBit(to);
                break;
            case BlackQueen:
                blackPieceCount -= AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
                blackQueensBishops.clrBit(to);
                blackQueensRooks.clrBit(to);
                break;
            }
        }
        else {
            // White
            materialScore -= (Score::Piece[promotedPiece]-Score::Pawn);

            whitePawns.setBit(to);  // To emulate a simple pawn move
            movedPiece = WhitePawn;

            whitePieceCount += AllPawns_Unit;

            switch( promotedPiece ) {
            case WhiteKnight:
                whitePieceCount -= AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
                whiteKnights.clrBit(to);
                break;
            case WhiteBishop:
                whitePieceCount -= AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
                whiteQueensBishops.clrBit(to);
                break;
            case WhiteRook:
                whitePieceCount -= AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
                whiteQueensRooks.clrBit(to);
                break;
            case WhiteQueen:
                whitePieceCount -= AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
                whiteQueensBishops.clrBit(to);
                whiteQueensRooks.clrBit(to);
                break;
            }
        }
    }
    else {
        movedPiece = board.piece[to];
    }

    BitBoard    fromTo  = BitBoard::Set[from] | BitBoard::Set[to];

    switch( movedPiece ) {
    case BlackPawn:
        blackPieces ^= fromTo;
        blackPawns ^= fromTo;
        break;
    case WhitePawn:
        whitePieces ^= fromTo;
        whitePawns ^= fromTo;
        break;
    case BlackKnight:
        blackPieces ^= fromTo;
        blackKnights ^= fromTo;
        break;
    case WhiteKnight:
        whitePieces ^= fromTo;
        whiteKnights ^= fromTo;
        break;
    case BlackBishop:
        blackPieces ^= fromTo;
        blackQueensBishops ^= fromTo;
        break;
    case WhiteBishop:
        whitePieces ^= fromTo;
        whiteQueensBishops ^= fromTo;
        break;
    case BlackRook:
        blackPieces ^= fromTo;
        blackQueensRooks ^= fromTo;
        break;
    case WhiteRook:
        whitePieces ^= fromTo;
        whiteQueensRooks ^= fromTo;
        break;
    case BlackQueen:
        blackPieces ^= fromTo;
        blackQueensBishops ^= fromTo;
        blackQueensRooks ^= fromTo;
        break;
    case WhiteQueen:
        whitePieces ^= fromTo;
        whiteQueensBishops ^= fromTo;
        whiteQueensRooks ^= fromTo;
        break;
    case BlackKing:
        blackPieces     ^= fromTo;
        blackKingSquare = from;
        if( from == E8 ) {
            if( to == G8 ) {        // Kingside castle: restore rook position
                fromTo = BitBoard::Set[H8] | BitBoard::Set[F8];
                blackPieces         ^= fromTo;
                blackQueensRooks    ^= fromTo;
                board.piece[H8]     = BlackRook;
                board.piece[F8]     = None;
            }
            else if( to == C8 ) {   // Queenside castle: restore rook position
                fromTo = BitBoard::Set[A8] | BitBoard::Set[D8];
                blackPieces         ^= fromTo;
                blackQueensRooks    ^= fromTo;
                board.piece[A8]     = BlackRook;
                board.piece[D8]     = None;
            }
        }
        break;
    case WhiteKing:
        whitePieces ^= fromTo;
        whiteKingSquare = from;
        if( from == E1 ) {
            if( to == G1 ) {        // Kingside castle: restore rook position
                fromTo = BitBoard::Set[H1] | BitBoard::Set[F1];
                whitePieces         ^= fromTo;
                whiteQueensRooks    ^= fromTo;
                board.piece[H1]     = WhiteRook;
                board.piece[F1]     = None;
            }
            else if( to == C1 ) {   // Queenside castle: restore rook position
                fromTo = BitBoard::Set[A1] | BitBoard::Set[D1];
                whitePieces         ^= fromTo;
                whiteQueensRooks    ^= fromTo;
                board.piece[A1]     = WhiteRook;
                board.piece[D1]     = None;
            }
        }
        break;
    }

    // Move piece back to starting position
    board.piece[from] = (Piece)movedPiece;

    // Get the type of a captured piece (or empty square)
    int pieceCaptured   = m.getCaptured();
    int pieceCapturedPos= to;

    if( m.getEnPassant() ) {
        // Undo en-passant capture
        board.piece[to] = None;

        if( to <= H3 ) {
            // This is a black pawn capturing on the 3rd rank
            pieceCaptured = WhitePawn;
            pieceCapturedPos = to+8;
        }
        else {
            // White pawn capturing on the 6th rank
            pieceCaptured = BlackPawn;
            pieceCapturedPos = to-8;
        }
    }

    // Restore captured piece (or empty square)
    board.piece[pieceCapturedPos] = (Piece)pieceCaptured;

    // If it was a capture, restore the position information
    if( pieceCaptured != None ) {
        materialScore += Score::Piece[ pieceCaptured ];

        switch( pieceCaptured ) {
        case BlackPawn:
            blackPieceCount += AllPawns_Unit;
            blackPieces.setBit(pieceCapturedPos);
            blackPawns.setBit(pieceCapturedPos);
            break;
        case WhitePawn:
            whitePieceCount += AllPawns_Unit;
            whitePieces.setBit(pieceCapturedPos);
            whitePawns.setBit(pieceCapturedPos);
            break;
        case BlackKnight:
            blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            blackPieces.setBit(pieceCapturedPos);
            blackKnights.setBit(pieceCapturedPos);
            break;
        case WhiteKnight:
            whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            whitePieces.setBit(pieceCapturedPos);
            whiteKnights.setBit(pieceCapturedPos);
            break;
        case BlackBishop:
            blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            blackPieces.setBit(pieceCapturedPos);
            blackQueensBishops.setBit(pieceCapturedPos);
            break;
        case WhiteBishop:
            whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            whitePieces.setBit(pieceCapturedPos);
            whiteQueensBishops.setBit(pieceCapturedPos);
            break;
        case BlackRook:
            blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            blackPieces.setBit(pieceCapturedPos);
            blackQueensRooks.setBit(pieceCapturedPos);
            break;
        case WhiteRook:
            whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            whitePieces.setBit(pieceCapturedPos);
            whiteQueensRooks.setBit(pieceCapturedPos);
            break;
        case BlackQueen:
            blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            blackPieces.setBit(pieceCapturedPos);
            blackQueensBishops.setBit(pieceCapturedPos);
            blackQueensRooks.setBit(pieceCapturedPos);
            break;
        case WhiteQueen:
            whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            whitePieces.setBit(pieceCapturedPos);
            whiteQueensBishops.setBit(pieceCapturedPos);
            whiteQueensRooks.setBit(pieceCapturedPos);
            break;
        }
    }
}

void Position::undoNullMove( const UndoInfo & info ) 
{ 
    hashCode        = info.hashCode;
    pawnHashCode    = info.pawnHashCode;
    boardFlags      = info.boardFlags;
    sideToPlay      = OppositeSide( sideToPlay );
}
