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
#include <assert.h>
#include <stdlib.h>

#include "board.h"
#include "hash.h"
#include "log.h"
#include "mask.h"
#include "movelist.h"
#include "position.h"
#include "san.h"
#include "score.h"
#include "zobrist.h"

/*
    Performs the specified move.

    This function performs very little validation and will only fail if the move:
    - captures an invalid piece (e.g. king);
    - promotes a pawn to an invalid piece;
    - is a pawn double step and the middle square is not empty;
    - leaves the king in check.

    To fully validate a move (e.g. an external input) then it is necessary
    to call isValidMove() first.
*/
int Position::doMove( Move & m )
{
    int         to          = m.getTo();
    int         from        = m.getFrom();
    BitBoard    fromTo      = BitBoard::Set[from] | BitBoard::Set[to];
    int         pieceMoved  = board.piece[from];

    int pieceCaptured;
    int pieceCapturedPos;

    if( boardFlags & EnPassantAvailable ) {
        int enPassantSquare = boardFlags & EnPassantRawSquareMask;
        
        // Reset en-passant availability
        boardFlags &= ~EnPassantSquareMask;
        hashCode ^= Zobrist::EnPassant[enPassantSquare];

        if( (to == enPassantSquare) && (PieceType(pieceMoved)==Pawn) ) {
            // En-passant capture
            m.setEnPassant();

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

            board.piece[pieceCapturedPos] = None;

            allPieces ^= BitBoard::Set[pieceCapturedPos] | BitBoard::Set[to];

            allPiecesTA1H8 ^= SetTA1H8[pieceCapturedPos] | SetTA1H8[to];
            allPiecesDA1H8 ^= SetDA1H8[pieceCapturedPos] | SetDA1H8[to];
            allPiecesDA8H1 ^= SetDA8H1[pieceCapturedPos] | SetDA8H1[to];
        }
        else {
            pieceCaptured   = board.piece[to];
            pieceCapturedPos = to;
        }
    }
    else {
        pieceCaptured   = board.piece[to];
        pieceCapturedPos = to;
    }

    // Note: I think this is compiler and platform dependent, but the above code
    // is about 11% faster of every other variation I have tried!

    if( pieceCaptured != None ) {
        // Remove the moved piece from the global boards (later the captured piece
        // will be "overwritten" on the destination square by the moved piece)
        allPieces.clrBit( from );
        allPiecesTA1H8 ^= SetTA1H8[from];
        allPiecesDA1H8 ^= SetDA1H8[from];
        allPiecesDA8H1 ^= SetDA8H1[from];

        // Reset the half-move clock
        boardFlags &= ~HalfMoveClockMask;

        // Remember that this move captures a piece
        m.setCaptured( pieceCaptured );

        // Update material score
        materialScore -= Score::Piece[ pieceCaptured ];

        // Remove the captured piece from the board
        switch( pieceCaptured ) {
        case BlackPawn:
            blackPieceCount -= AllPawns_Unit;
            blackPieces.clrBit(pieceCapturedPos);
            blackPawns.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::BlackPawn[pieceCapturedPos];
            pawnHashCode ^= Zobrist::BlackPawn[pieceCapturedPos];
            updateSignatureRemove( Black, Pawn );
            break;
        case WhitePawn:
            whitePieceCount -= AllPawns_Unit;
            whitePieces.clrBit(pieceCapturedPos);
            whitePawns.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::WhitePawn[pieceCapturedPos];
            pawnHashCode ^= Zobrist::WhitePawn[pieceCapturedPos];
            updateSignatureRemove( White, Pawn );
            break;
        case BlackKnight:
            pstScoreOpening += Score::BlackKnight_Opening[ pieceCapturedPos ];
            pstScoreEndgame += Score::BlackKnight_Endgame[ pieceCapturedPos ];

            blackPieceCount -= AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            blackPieces.clrBit(pieceCapturedPos);
            blackKnights.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::BlackKnight[pieceCapturedPos];
            updateSignatureRemove( Black, Knight );
            break;
        case WhiteKnight:
            pstScoreOpening -= Score::WhiteKnight_Opening[ pieceCapturedPos ];
            pstScoreEndgame -= Score::WhiteKnight_Endgame[ pieceCapturedPos ];

            whitePieceCount -= AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            whitePieces.clrBit(pieceCapturedPos);
            whiteKnights.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::WhiteKnight[pieceCapturedPos];
            updateSignatureRemove( White, Knight );
            break;
        case BlackBishop:
            pstScoreOpening += Score::BlackBishop_Opening[ pieceCapturedPos ];
            pstScoreEndgame += Score::BlackBishop_Endgame[ pieceCapturedPos ];

            blackPieceCount -= AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            blackPieces.clrBit(pieceCapturedPos);
            blackQueensBishops.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::BlackBishop[pieceCapturedPos];
            updateSignatureRemove( Black, Bishop );
            break;
        case WhiteBishop:
            pstScoreOpening -= Score::WhiteBishop_Opening[ pieceCapturedPos ];
            pstScoreEndgame -= Score::WhiteBishop_Endgame[ pieceCapturedPos ];

            whitePieceCount -= AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            whitePieces.clrBit(pieceCapturedPos);
            whiteQueensBishops.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::WhiteBishop[pieceCapturedPos];
            updateSignatureRemove( White, Bishop );
            break;
        case BlackRook:
            pstScoreOpening += Score::BlackRook_Opening[ pieceCapturedPos ];
            pstScoreEndgame += Score::BlackRook_Endgame[ pieceCapturedPos ];

            blackPieceCount -= AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            blackPieces.clrBit(pieceCapturedPos);
            blackQueensRooks.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::BlackRook[pieceCapturedPos];
            if( (pieceCapturedPos == H8) && (boardFlags & BlackCastleKing) ) {
                // Disable kingside castling
                boardFlags &= ~BlackCastleKing;
                hashCode ^= Zobrist::BlackCastleKing;
            }
            if( (pieceCapturedPos == A8) && (boardFlags & BlackCastleQueen) ) {
                // Disable queenside castling
                boardFlags &= ~BlackCastleQueen;
                hashCode ^= Zobrist::BlackCastleQueen;
            }
            updateSignatureRemove( Black, Rook );
            break;
        case WhiteRook:
            pstScoreOpening -= Score::WhiteRook_Opening[ pieceCapturedPos ];
            pstScoreEndgame -= Score::WhiteRook_Endgame[ pieceCapturedPos ];

            whitePieceCount -= AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            whitePieces.clrBit(pieceCapturedPos);
            whiteQueensRooks.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::WhiteRook[pieceCapturedPos];
            if( (pieceCapturedPos == H1) && (boardFlags & WhiteCastleKing) ) {
                // Disable kingside castling
                boardFlags &= ~WhiteCastleKing;
                hashCode ^= Zobrist::WhiteCastleKing;
            }
            if( (pieceCapturedPos == A1) && (boardFlags & WhiteCastleQueen) ) {
                // Disable queenside castling
                boardFlags &= ~WhiteCastleQueen;
                hashCode ^= Zobrist::WhiteCastleQueen;
            }
            updateSignatureRemove( White, Rook );
            break;
        case BlackQueen:
            pstScoreOpening += Score::BlackQueen_Opening[ pieceCapturedPos ];
            pstScoreEndgame += Score::BlackQueen_Endgame[ pieceCapturedPos ];

            blackPieceCount -= AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            blackPieces.clrBit(pieceCapturedPos);
            blackQueensBishops.clrBit(pieceCapturedPos);
            blackQueensRooks.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::BlackQueen[pieceCapturedPos];
            updateSignatureRemove( Black, Queen );
            break;
        case WhiteQueen:
            pstScoreOpening -= Score::WhiteQueen_Opening[ pieceCapturedPos ];
            pstScoreEndgame -= Score::WhiteQueen_Endgame[ pieceCapturedPos ];

            whitePieceCount -= AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            whitePieces.clrBit(pieceCapturedPos);
            whiteQueensBishops.clrBit(pieceCapturedPos);
            whiteQueensRooks.clrBit(pieceCapturedPos);
            hashCode ^= Zobrist::WhiteQueen[pieceCapturedPos];
            updateSignatureRemove( White, Queen );
            break;
        default:
            return 1;
        }
    }
    else {
        // Not a capture
        allPieces ^= fromTo;

        allPiecesTA1H8 ^= SetTA1H8[from] | SetTA1H8[to];
        allPiecesDA1H8 ^= SetDA1H8[from] | SetDA1H8[to];
        allPiecesDA8H1 ^= SetDA8H1[from] | SetDA8H1[to];

        // Bump the half-move clock (if this is a pawn move it will be reset later)
        boardFlags += HalfMoveClockIncrement;
    }

    switch( pieceMoved ) {
    case BlackPawn:
        boardFlags &= ~HalfMoveClockMask; // Reset half-move clock
        blackPieces ^= fromTo;
        hashCode ^= Zobrist::BlackPawn[from];
        pawnHashCode ^= Zobrist::BlackPawn[from];
        if( to <= H1 ) {
            // Promotion: remove pawn and add piece
            blackPawns.clrBit(from);
            board.piece[to] = (Piece)m.getPromoted();
            materialScore += (Score::Piece[m.getPromoted()]+Score::Pawn);

            blackPieceCount -= AllPawns_Unit;

            switch( m.getPromoted() ) {
            case BlackKnight:
                pstScoreOpening -= Score::BlackKnight_Opening[ to ];
                pstScoreEndgame -= Score::BlackKnight_Endgame[ to ];

                blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
                blackKnights.setBit(to);
                hashCode ^= Zobrist::BlackKnight[to];
                updateSignatureAdd( Black, Knight );
                break;
            case BlackBishop:
                pstScoreOpening -= Score::BlackBishop_Opening[ to ];
                pstScoreEndgame -= Score::BlackBishop_Endgame[ to ];

                blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
                blackQueensBishops.setBit(to);
                hashCode ^= Zobrist::BlackBishop[to];
                updateSignatureAdd( Black, Bishop );
                break;
            case BlackRook:
                pstScoreOpening -= Score::BlackRook_Opening[ to ];
                pstScoreEndgame -= Score::BlackRook_Endgame[ to ];

                blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
                blackQueensRooks.setBit(to);
                hashCode ^= Zobrist::BlackRook[to];
                updateSignatureAdd( Black, Rook );
                break;
            case BlackQueen:
                pstScoreOpening -= Score::BlackQueen_Opening[ to ];
                pstScoreEndgame -= Score::BlackQueen_Endgame[ to ];

                blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
                blackQueensBishops.setBit(to);
                blackQueensRooks.setBit(to);
                hashCode ^= Zobrist::BlackQueen[to];
                updateSignatureAdd( Black, Queen );
                break;
            default:
                return 1;
            }

            updateSignatureRemove( Black, Pawn );
        }
        else {
            blackPawns ^= fromTo;
            board.piece[to] = BlackPawn;
            hashCode ^= Zobrist::BlackPawn[to];
            pawnHashCode ^= Zobrist::BlackPawn[to];

            if( (from >= A7) && (to <= H5) && (Mask::SideSquares[to] & whitePawns) ) {
                // Enable en-passant for next move
                int enPassantSquare = from - 8;
                hashCode ^= Zobrist::EnPassant[enPassantSquare];
                boardFlags |= EnPassantAvailable | enPassantSquare;

                if( board.piece[enPassantSquare] != None ) {
                    return 1;
                }
            }
        }
        break;
    case WhitePawn:
        boardFlags &= ~HalfMoveClockMask; // Reset half-move clock
        whitePieces ^= fromTo;
        hashCode ^= Zobrist::WhitePawn[from];
        pawnHashCode ^= Zobrist::WhitePawn[from];
        if( to >= A8 ) {
            // Promotion: remove pawn and add piece
            whitePawns.clrBit(from);
            board.piece[to] = (Piece)m.getPromoted();
            materialScore += (Score::Piece[m.getPromoted()]-Score::Pawn);

            whitePieceCount -= AllPawns_Unit;

            switch( m.getPromoted() ) {
            case WhiteKnight:
                pstScoreOpening += Score::WhiteKnight_Opening[ to ];
                pstScoreEndgame += Score::WhiteKnight_Endgame[ to ];

                whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
                whiteKnights.setBit(to);
                hashCode ^= Zobrist::WhiteKnight[to];
                updateSignatureAdd( White, Knight );
                break;
            case WhiteBishop:
                pstScoreOpening += Score::WhiteBishop_Opening[ to ];
                pstScoreEndgame += Score::WhiteBishop_Endgame[ to ];

                whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
                whiteQueensBishops.setBit(to);
                hashCode ^= Zobrist::WhiteBishop[to];
                updateSignatureAdd( White, Bishop );
                break;
            case WhiteRook:
                pstScoreOpening += Score::WhiteRook_Opening[ to ];
                pstScoreEndgame += Score::WhiteRook_Endgame[ to ];

                whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
                whiteQueensRooks.setBit(to);
                hashCode ^= Zobrist::WhiteRook[to];
                updateSignatureAdd( White, Rook );
                break;
            case WhiteQueen:
                pstScoreOpening += Score::WhiteQueen_Opening[ to ];
                pstScoreEndgame += Score::WhiteQueen_Endgame[ to ];

                whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
                whiteQueensBishops.setBit(to);
                whiteQueensRooks.setBit(to);
                hashCode ^= Zobrist::WhiteQueen[to];
                updateSignatureAdd( White, Queen );
                break;
            }

            updateSignatureRemove( White, Pawn );
        }
        else {
            whitePawns ^= fromTo;
            board.piece[to] = WhitePawn;
            hashCode ^= Zobrist::WhitePawn[to];
            pawnHashCode ^= Zobrist::WhitePawn[to];

            if( (from <= H2) && (to >= A4) && (Mask::SideSquares[to] & blackPawns) ) {
                // Enable en-passant for next move
                int enPassantSquare = from + 8;
                hashCode ^= Zobrist::EnPassant[enPassantSquare];
                boardFlags |= EnPassantAvailable | enPassantSquare;

                if( board.piece[enPassantSquare] != None ) {
                    return 1;
                }
            }
        }
        break;
    case BlackKnight:
        pstScoreOpening += Score::BlackKnight_Opening[ from ];
        pstScoreOpening -= Score::BlackKnight_Opening[ to ];
        pstScoreEndgame += Score::BlackKnight_Endgame[ from ];
        pstScoreEndgame -= Score::BlackKnight_Endgame[ to ];

        blackPieces ^= fromTo;
        blackKnights ^= fromTo;
        board.piece[to] = BlackKnight;
        hashCode ^= Zobrist::BlackKnight[from];
        hashCode ^= Zobrist::BlackKnight[to];
        break;
    case WhiteKnight:
        pstScoreOpening -= Score::WhiteKnight_Opening[ from ];
        pstScoreOpening += Score::WhiteKnight_Opening[ to ];
        pstScoreEndgame -= Score::WhiteKnight_Endgame[ from ];
        pstScoreEndgame += Score::WhiteKnight_Endgame[ to ];

        whitePieces ^= fromTo;
        whiteKnights ^= fromTo;
        board.piece[to] = WhiteKnight;
        hashCode ^= Zobrist::WhiteKnight[from];
        hashCode ^= Zobrist::WhiteKnight[to];
        break;
    case BlackBishop:
        pstScoreOpening += Score::BlackBishop_Opening[ from ];
        pstScoreOpening -= Score::BlackBishop_Opening[ to ];
        pstScoreEndgame += Score::BlackBishop_Endgame[ from ];
        pstScoreEndgame -= Score::BlackBishop_Endgame[ to ];

        blackPieces ^= fromTo;
        blackQueensBishops ^= fromTo;
        board.piece[to] = BlackBishop;
        hashCode ^= Zobrist::BlackBishop[from];
        hashCode ^= Zobrist::BlackBishop[to];
        break;
    case WhiteBishop:
        pstScoreOpening -= Score::WhiteBishop_Opening[ from ];
        pstScoreOpening += Score::WhiteBishop_Opening[ to ];
        pstScoreEndgame -= Score::WhiteBishop_Endgame[ from ];
        pstScoreEndgame += Score::WhiteBishop_Endgame[ to ];

        whitePieces ^= fromTo;
        whiteQueensBishops ^= fromTo;
        board.piece[to] = WhiteBishop;
        hashCode ^= Zobrist::WhiteBishop[from];
        hashCode ^= Zobrist::WhiteBishop[to];
        break;
    case BlackRook:
        pstScoreOpening += Score::BlackRook_Opening[ from ];
        pstScoreOpening -= Score::BlackRook_Opening[ to ];
        pstScoreEndgame += Score::BlackRook_Endgame[ from ];
        pstScoreEndgame -= Score::BlackRook_Endgame[ to ];

        blackPieces ^= fromTo;
        blackQueensRooks ^= fromTo;
        board.piece[to] = BlackRook;
        hashCode ^= Zobrist::BlackRook[from];
        hashCode ^= Zobrist::BlackRook[to];
        if( (from == H8) && (boardFlags & BlackCastleKing) ) {
            // Disable kingside castling
            boardFlags &= ~BlackCastleKing;
            hashCode ^= Zobrist::BlackCastleKing;
        }
        if( (from == A8) && (boardFlags & BlackCastleQueen) ) {
            // Disable queenside castling
            boardFlags &= ~BlackCastleQueen;
            hashCode ^= Zobrist::BlackCastleQueen;
        }
        break;
    case WhiteRook:
        pstScoreOpening -= Score::WhiteRook_Opening[ from ];
        pstScoreOpening += Score::WhiteRook_Opening[ to ];
        pstScoreEndgame -= Score::WhiteRook_Endgame[ from ];
        pstScoreEndgame += Score::WhiteRook_Endgame[ to ];

        whitePieces ^= fromTo;
        whiteQueensRooks ^= fromTo;
        board.piece[to] = WhiteRook;
        hashCode ^= Zobrist::WhiteRook[from];
        hashCode ^= Zobrist::WhiteRook[to];
        if( (from == H1) && (boardFlags & WhiteCastleKing) ) {
            // Disable kingside castling
            boardFlags &= ~WhiteCastleKing;
            hashCode ^= Zobrist::WhiteCastleKing;
        }
        if( (from == A1) && (boardFlags & WhiteCastleQueen) ) {
            // Disable queenside castling
            boardFlags &= ~WhiteCastleQueen;
            hashCode ^= Zobrist::WhiteCastleQueen;
        }
        break;
    case BlackQueen:
        pstScoreOpening += Score::BlackQueen_Opening[ from ];
        pstScoreOpening -= Score::BlackQueen_Opening[ to ];
        pstScoreEndgame += Score::BlackQueen_Endgame[ from ];
        pstScoreEndgame -= Score::BlackQueen_Endgame[ to ];

        blackPieces ^= fromTo;
        blackQueensBishops ^= fromTo;
        blackQueensRooks ^= fromTo;
        board.piece[to] = BlackQueen;
        hashCode ^= Zobrist::BlackQueen[from];
        hashCode ^= Zobrist::BlackQueen[to];
        break;
    case WhiteQueen:
        pstScoreOpening -= Score::WhiteQueen_Opening[ from ];
        pstScoreOpening += Score::WhiteQueen_Opening[ to ];
        pstScoreEndgame -= Score::WhiteQueen_Endgame[ from ];
        pstScoreEndgame += Score::WhiteQueen_Endgame[ to ];

        whitePieces ^= fromTo;
        whiteQueensBishops ^= fromTo;
        whiteQueensRooks ^= fromTo;
        board.piece[to] = WhiteQueen;
        hashCode ^= Zobrist::WhiteQueen[from];
        hashCode ^= Zobrist::WhiteQueen[to];
        break;
    case BlackKing:
        pstScoreOpening += Score::BlackKing_Opening[ from ];
        pstScoreOpening -= Score::BlackKing_Opening[ to ];
        pstScoreEndgame += Score::BlackKing_Endgame[ from ];
        pstScoreEndgame -= Score::BlackKing_Endgame[ to ];

        blackPieces ^= fromTo;
        blackKingSquare = to;
        board.piece[to] = BlackKing;
        hashCode ^= Zobrist::BlackKing[from];
        hashCode ^= Zobrist::BlackKing[to];
        if( from == E8 ) {
            if( to == G8 ) {
                // Kingside castle: move the rook
                pstScoreOpening += Score::BlackRook_Opening[ H8 ];
                pstScoreOpening -= Score::BlackRook_Opening[ F8 ];
                pstScoreEndgame += Score::BlackRook_Endgame[ H8 ];
                pstScoreEndgame -= Score::BlackRook_Endgame[ F8 ];

                fromTo = BitBoard::Set[H8] | BitBoard::Set[F8];
                blackPieces ^= fromTo;
                blackQueensRooks ^= fromTo;
                allPieces ^= fromTo;
                allPiecesTA1H8 ^= SetTA1H8[H8] | SetTA1H8[F8];
                allPiecesDA1H8 ^= SetDA1H8[H8] | SetDA1H8[F8];
                allPiecesDA8H1 ^= SetDA8H1[H8] | SetDA8H1[F8];
                hashCode ^= Zobrist::BlackRook[H8];
                hashCode ^= Zobrist::BlackRook[F8];
                board.piece[H8] = None;
                board.piece[F8] = BlackRook;

                boardFlags |= BlackHasCastled;
            }
            else if( to == C8 ) {
                // Queenside castle: move the rook
                pstScoreOpening += Score::BlackRook_Opening[ A8 ];
                pstScoreOpening -= Score::BlackRook_Opening[ D8 ];
                pstScoreEndgame += Score::BlackRook_Endgame[ A8 ];
                pstScoreEndgame -= Score::BlackRook_Endgame[ D8 ];

                fromTo = BitBoard::Set[A8] | BitBoard::Set[D8];
                blackPieces ^= fromTo;
                blackQueensRooks ^= fromTo;
                allPieces ^= fromTo;
                allPiecesTA1H8 ^= SetTA1H8[A8] | SetTA1H8[D8];
                allPiecesDA1H8 ^= SetDA1H8[A8] | SetDA1H8[D8];
                allPiecesDA8H1 ^= SetDA8H1[A8] | SetDA8H1[D8];
                hashCode ^= Zobrist::BlackRook[A8];
                hashCode ^= Zobrist::BlackRook[D8];
                board.piece[A8] = None;
                board.piece[D8] = BlackRook;

                boardFlags |= BlackHasCastled;
            }
            // Clear castling flags
            if( boardFlags & BlackCastleKing )   hashCode ^= Zobrist::BlackCastleKing;
            if( boardFlags & BlackCastleQueen )  hashCode ^= Zobrist::BlackCastleQueen;
            boardFlags &= ~(BlackCastleKing | BlackCastleQueen);
        }
        break;
    case WhiteKing:
        pstScoreOpening -= Score::WhiteKing_Opening[ from ];
        pstScoreOpening += Score::WhiteKing_Opening[ to ];
        pstScoreEndgame -= Score::WhiteKing_Endgame[ from ];
        pstScoreEndgame += Score::WhiteKing_Endgame[ to ];

        whitePieces ^= fromTo;
        whiteKingSquare = to;
        board.piece[to] = WhiteKing;
        hashCode ^= Zobrist::WhiteKing[from];
        hashCode ^= Zobrist::WhiteKing[to];
        if( from == E1 ) {
            if( to == G1 ) {
                // Kingside castle: move the rook
                pstScoreOpening -= Score::WhiteRook_Opening[ H1 ];
                pstScoreOpening += Score::WhiteRook_Opening[ F1 ];
                pstScoreEndgame -= Score::WhiteRook_Endgame[ H1 ];
                pstScoreEndgame += Score::WhiteRook_Endgame[ F1 ];

                fromTo = BitBoard::Set[H1] | BitBoard::Set[F1];
                whitePieces ^= fromTo;
                whiteQueensRooks ^= fromTo;
                allPieces ^= fromTo;
                allPiecesTA1H8 ^= SetTA1H8[H1] | SetTA1H8[F1];
                allPiecesDA1H8 ^= SetDA1H8[H1] | SetDA1H8[F1];
                allPiecesDA8H1 ^= SetDA8H1[H1] | SetDA8H1[F1];
                hashCode ^= Zobrist::WhiteRook[H1];
                hashCode ^= Zobrist::WhiteRook[F1];
                board.piece[H1] = None;
                board.piece[F1] = WhiteRook;

                boardFlags |= WhiteHasCastled;
            }
            else if( to == C1 ) {
                // Queenside castle: move the rook
                pstScoreOpening -= Score::WhiteRook_Opening[ A1 ];
                pstScoreOpening += Score::WhiteRook_Opening[ D1 ];
                pstScoreEndgame -= Score::WhiteRook_Endgame[ A1 ];
                pstScoreEndgame += Score::WhiteRook_Endgame[ D1 ];

                fromTo = BitBoard::Set[A1] | BitBoard::Set[D1];
                whitePieces ^= fromTo;
                whiteQueensRooks ^= fromTo;
                allPieces ^= fromTo;
                allPiecesTA1H8 ^= SetTA1H8[A1] | SetTA1H8[D1];
                allPiecesDA1H8 ^= SetDA1H8[A1] | SetDA1H8[D1];
                allPiecesDA8H1 ^= SetDA8H1[A1] | SetDA8H1[D1];
                hashCode ^= Zobrist::WhiteRook[A1];
                hashCode ^= Zobrist::WhiteRook[D1];
                board.piece[A1] = None;
                board.piece[D1] = WhiteRook;

                boardFlags |= WhiteHasCastled;
            }

            // Clear castling flags
            if( boardFlags & WhiteCastleKing )   hashCode ^= Zobrist::WhiteCastleKing;
            if( boardFlags & WhiteCastleQueen )  hashCode ^= Zobrist::WhiteCastleQueen;

            boardFlags &= ~(WhiteCastleKing | WhiteCastleQueen);
        }
        break;
    }

    board.piece[from] = None;

    sideToPlay ^= PieceSideMask;
    hashCode ^= ZobSideToPlay;

    // Check whether this move put this side in check (then it's illegal) or
    // if it checks the other side
    if( sideToPlay != Black ) {
        // Note: the current side to play is the opposite of the one that
        // performed the move, so this code refers to black!

        int king = blackKingSquare;

        if( (boardFlags & SideToPlayInCheck) || (pieceMoved == BlackKing) || m.getEnPassant() ) {
            // Make sure we do not put the king in check
            if( Attacks::Knight[king] & whiteKnights ) return 1;
            if( Attacks::BlackPawn[king] & whitePawns ) return 1;
            if( rookAttacks(king) & whiteQueensRooks ) return 1;
            if( bishopAttacks(king) & whiteQueensBishops ) return 1;
            if( Attacks::KingDistance[king][whiteKingSquare] <= 1 ) return 1;
        }
        else {
            // If we are here then we can only get a discovered check
            // when our own piece moved
            unsigned    dir = Attacks::Direction[from][king];

            switch( dir ) {
            case DirRank:
                if( rookAttacksOnRank( king ) & whiteQueensRooks ) return 1;
                break;
            case DirFile:
                if( rookAttacksOnFile( king ) & whiteQueensRooks ) return 1;
                break;
            case DirA1H8:
                if( bishopAttacksOnDiagA1H8( king ) & whiteQueensBishops ) return 1;
                break;
            case DirA8H1:
                if( bishopAttacksOnDiagA8H1( king ) & whiteQueensBishops ) return 1;
                break;
            }
        }

        // Update side in check flag
        boardFlags &= ~SideToPlayInCheck;

        king = whiteKingSquare;

        if( ( Attacks::Knight[king] & blackKnights ) ||
            ( (Attacks::Rook[king] & blackQueensRooks) && (rookAttacks(king) & blackQueensRooks) )  ||
            ( (Attacks::Bishop[king] & blackQueensBishops) && (bishopAttacks(king) & blackQueensBishops) ) ||
            ( Attacks::WhitePawn[king] & blackPawns ) ) 
        {
            boardFlags |= SideToPlayInCheck;
        }
    }
    else {
        int king = whiteKingSquare;

        if( (boardFlags & SideToPlayInCheck) || (pieceMoved == WhiteKing) || m.getEnPassant() ) {
            // Make sure we do not put the king in check
            if( Attacks::Knight[king] & blackKnights ) return 1;
            if( Attacks::WhitePawn[king] & blackPawns ) return 1;
            if( rookAttacks(king) & blackQueensRooks ) return 1;
            if( bishopAttacks(king) & blackQueensBishops ) return 1;
            if( Attacks::KingDistance[king][blackKingSquare] <= 1 ) return 1;
        }
        else {
            // If we are here then we can only get a discovered check
            // when our own piece moved
            unsigned    dir = Attacks::Direction[from][king];

            switch( dir ) {
            case DirRank:
                if( rookAttacksOnRank( king ) & blackQueensRooks ) return 1;
                break;
            case DirFile:
                if( rookAttacksOnFile( king ) & blackQueensRooks ) return 1;
                break;
            case DirA1H8:
                if( bishopAttacksOnDiagA1H8( king ) & blackQueensBishops ) return 1;
                break;
            case DirA8H1:
                if( bishopAttacksOnDiagA8H1( king ) & blackQueensBishops ) return 1;
                break;
            }

        }

        // Update side in check flag
        boardFlags &= ~SideToPlayInCheck;

        king = blackKingSquare;

        if( ( Attacks::Knight[king] & whiteKnights ) ||
            ( (Attacks::Rook[king] & whiteQueensRooks) && (rookAttacks(king) & whiteQueensRooks) )  ||
            ( (Attacks::Bishop[king] & whiteQueensBishops) && (bishopAttacks(king) & whiteQueensBishops) ) ||
            ( Attacks::BlackPawn[king] & whitePawns ) ) 
        {
            boardFlags |= SideToPlayInCheck;
        }
    }

    return 0;
}

int Position::doNullMove() 
{ 
    if( boardFlags & EnPassantAvailable ) {
        // En-passant is available, update hash code and make it unavailable
        hashCode ^= Zobrist::EnPassant[boardFlags & EnPassantRawSquareMask];
        boardFlags &= ~EnPassantSquareMask;
    }

    sideToPlay = OppositeSide(sideToPlay);
    hashCode ^= ZobSideToPlay;

    return 0; 
}

/*
    Checks whether a move is valid or not.

    This function performs an extensive validation of the move and is
    designed to be called before doMove().

    Since isValidMove() and doMove() perform different validations, both
    functions must be called to be sure that a move is indeed playable,
    for example:

        if( pos.isValidMove( m ) ) {
            if( pos.doMove( m ) == 0 ) {
                // Ok, the move is valid
            }
        }
    
    The reasons there are two different functions is that generateMoves()
    does not generate moves that would fail the isValidMove() test, so
    calling this function can be avoided in most cases. The exceptions are
    "external" moves (e.g. from hash table, killer heuristic, and so on)
    that must always be validated with isValidMove().
*/
int Position::isValidMove( const Move & m ) const
{
    if( m == Move::Null )
        return 0;

    // Cannot move an enemy piece
    int from            = m.getFrom();
    int pieceMoved      = board.piece[ from ];
    
    if( (pieceMoved == None) || (PieceSide(pieceMoved) != sideToPlay) )
        return 0;

    // Cannot capture a friend piece or a king
    int to              = m.getTo();
    int pieceCaptured   = board.piece[ to ];

    if( (pieceCaptured != None) && ((PieceSide(pieceCaptured)==sideToPlay)||(PieceType(pieceCaptured)==King)) )
        return 0;

    // Get en-passant square
    int enPassantSquare = (boardFlags & EnPassantSquareMask) - EnPassantAvailable;

    // Verify that the piece can move to the destination square
    BitBoard    maskTo  = BitBoard::Set[ to ];

    // TODO: en-passant is not handled correctly here! If we get in check by capturing
    // the opponent pawn, this is not detected:
    // 6k1/8/8/8/2Pp4/8/B7/5K2 b - c3 0 1
    // here dxc3 is illegal yet Kiwi plays it!

    switch( PieceType(pieceMoved) ) {
    case Pawn:
        if( pieceMoved == BlackPawn ) {
            // En-passant capture
            if( to == enPassantSquare ) {
                if( board.piece[to+8] != WhitePawn ) return 0;
                if( maskTo & Attacks::BlackPawn[from]) return 1;
                return 0;
            }

            // Promotion
            if( RankOfSquare( to ) == 0 ) {
                int p = m.getPromoted();

                if( p != BlackQueen && p != BlackRook && p != BlackKnight && p != BlackBishop )
                    return 0;
            }

            // Capture
            if( pieceCaptured != None ) {
                if( maskTo & Attacks::BlackPawn[from]) return 1;
                return 0;
            }

            // Simple move
            if( from-to == 8 ) 
                return 1;
            if( (from-to == 16) && (RankOfSquare(from)==6) && (board.piece[from-8]==None) )
                return 1;
        }
        else {
            // En-passant capture
            if( to == enPassantSquare ) {
                if( board.piece[to-8] != BlackPawn ) return 0;
                if( maskTo & Attacks::WhitePawn[from]) return 1;
                return 0;
            }

            // Promotion
            if( RankOfSquare(to) == 7 ) {
                int p = m.getPromoted();

                if( p != WhiteQueen && p != WhiteRook && p != WhiteKnight && p != WhiteBishop )
                    return 0;
            }

            // Capture
            if( pieceCaptured != None ) {
                if( maskTo & Attacks::WhitePawn[from] ) return 1;
                return 0;
            }

            // Simple move
            if( to-from == 8 )
                return 1;
            if( (to-from == 16) && (RankOfSquare(from)==1) && (board.piece[from+8]==None) )
                return 1;
        }
        break;
    case Knight:
        if( maskTo & Attacks::Knight[from]) return 1;
        break;
    case Bishop:
        if( maskTo & bishopAttacks(from) )  return 1;
        break;
    case Rook:
        if( maskTo & rookAttacks(from) )    return 1;
        break;
    case Queen:
        if( maskTo & rookAttacks(from) )    return 1;
        if( maskTo & bishopAttacks(from) )  return 1;
        break;
    case King:
        if( pieceMoved == BlackKing ) {
            if( (from == E8) && (to == G8) ) {
                if( boardFlags & SideToPlayInCheck ) return 0;
                if( (boardFlags & BlackCastleKing) == 0 ) return 0;
                if( board.piece[H8] != BlackRook ) return 0;
                if( allPieces & Mask::BlackCrossCastleKing ) return 0;
                if( isSquareAttackedBy( F8, White ) ) return 0;
                if( isSquareAttackedBy( G8, White ) ) return 0;
                return 1;
            }
            else if( (from == E8) && (to == C8) ) {
                if( boardFlags & SideToPlayInCheck ) return 0;
                if( (boardFlags & BlackCastleQueen) == 0 ) return 0;
                if( board.piece[A8] != BlackRook ) return 0;
                if( allPieces & Mask::BlackCrossCastleQueen ) return 0;
                if( isSquareAttackedBy( D8, White ) ) return 0;
                if( isSquareAttackedBy( C8, White ) ) return 0;
                return 1;
            }
        }
        else {
            if( (from == E1) && (to == G1) ) {
                if( boardFlags & SideToPlayInCheck ) return 0;
                if( (boardFlags & WhiteCastleKing) == 0 ) return 0;
                if( board.piece[H1] != WhiteRook ) return 0;
                if( allPieces & Mask::WhiteCrossCastleKing ) return 0;
                if( isSquareAttackedBy( F1, Black ) ) return 0;
                if( isSquareAttackedBy( G1, Black ) ) return 0;
                return 1;
            }
            else if( (from == E1) && (to == C1) ) {
                if( boardFlags & SideToPlayInCheck ) return 0;
                if( (boardFlags & WhiteCastleQueen) == 0 ) return 0;
                if( board.piece[A1] != WhiteRook ) return 0;
                if( allPieces & Mask::WhiteCrossCastleQueen ) return 0;
                if( isSquareAttackedBy( D1, Black ) ) return 0;
                if( isSquareAttackedBy( C1, Black ) ) return 0;
                return 1;
            }
        }
        if( maskTo & Attacks::King[from] )  return 1;
        break;
    }

    return 0;
}
