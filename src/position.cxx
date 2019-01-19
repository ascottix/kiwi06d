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
#include "bitboard.h"
#include "counters.h"
#include "log.h"
#include "mask.h"
#include "move.h"
#include "movelist.h"
#include "position.h"
#include "score.h"
#include "zobrist.h"

const char * Position::startPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/**
    Default constructor, setups the standard initial position.
*/
Position::Position()
{
    operator = ( startPosition );
}

/**
    Copy constructor.
*/
Position::Position( const Position & p )
{
    operator = ( p );
}

/**
    Assignment operator.
*/
const Position & Position::operator = ( const Position & p )
{
    board               = p.board;

    sideToPlay          = p.sideToPlay;
    boardFlags          = p.boardFlags;
    blackKingSquare     = p.blackKingSquare;
    whiteKingSquare     = p.whiteKingSquare;

    whitePieceCount     = p.whitePieceCount;
    blackPieceCount     = p.blackPieceCount;
    
    blackPawns          = p.blackPawns;
    blackKnights        = p.blackKnights;
    blackQueensBishops  = p.blackQueensBishops;
    blackQueensRooks    = p.blackQueensRooks;
    blackPieces         = p.blackPieces;
    
    whitePawns          = p.whitePawns;
    whiteKnights        = p.whiteKnights;
    whiteQueensBishops  = p.whiteQueensBishops;
    whiteQueensRooks    = p.whiteQueensRooks;
    whitePieces         = p.whitePieces;
    
    allPieces           = p.allPieces;
    allPiecesTA1H8      = p.allPiecesTA1H8;
    allPiecesDA1H8      = p.allPiecesDA1H8;
    allPiecesDA8H1      = p.allPiecesDA8H1;

    hashCode            = p.hashCode;
    pawnHashCode        = p.pawnHashCode;

    materialSignature   = p.materialSignature;
    materialScore       = p.materialScore;
    pstScoreOpening     = p.pstScoreOpening;
    pstScoreEndgame     = p.pstScoreEndgame;

    return *this;
}

void Position::verifyHashCode() const
{
    BitBoard code;

    code.clear();

    // Copy board and setup bitboards for specific pieces
    for( int i=A1; i<=H8; i++ ) {
        signed char piece = board.piece[i];
        
        switch( piece ) {
        case BlackPawn:
            code ^= Zobrist::BlackPawn[i];
            break;
        case BlackKnight:
            code ^= Zobrist::BlackKnight[i];
            break;
        case BlackBishop:
            code ^= Zobrist::BlackBishop[i];
            break;
        case BlackRook:
            code ^= Zobrist::BlackRook[i];
            break;
        case BlackQueen:
            code ^= Zobrist::BlackQueen[i];
            break;
        case BlackKing:
            code ^= Zobrist::BlackKing[i];
            break;
        case WhitePawn:
            code ^= Zobrist::WhitePawn[i];
            break;
        case WhiteKnight:
            code ^= Zobrist::WhiteKnight[i];
            break;
        case WhiteBishop:
            code ^= Zobrist::WhiteBishop[i];
            break;
        case WhiteRook:
            code ^= Zobrist::WhiteRook[i];
            break;
        case WhiteQueen:
            code ^= Zobrist::WhiteQueen[i];
            break;
        case WhiteKing:
            code ^= Zobrist::WhiteKing[i];
            break;
        }
    }

    if( boardFlags & WhiteCastleKing ) code ^= Zobrist::WhiteCastleKing;
    if( boardFlags & WhiteCastleQueen ) code ^= Zobrist::WhiteCastleQueen;
    if( boardFlags & BlackCastleKing ) code ^= Zobrist::BlackCastleKing;
    if( boardFlags & BlackCastleQueen ) code ^= Zobrist::BlackCastleQueen;

    if( boardFlags & EnPassantAvailable ) {
        int enPassantSquare = boardFlags & EnPassantRawSquareMask;
        
        code ^= Zobrist::EnPassant[ enPassantSquare ];
    }

    if( sideToPlay == Black ) {
        code ^= ZobSideToPlay;
    }

    if( code != hashCode ) {
        printf( "*** Hash code verification error!\n" ); 
#ifdef WIN32
        __asm int 3;
#endif
    }
}


/**
    Uses the specified board to setup the position. On exit several
    variables are undefined and must be set manually.
*/
void Position::setBoard( const Board & b )
{
    int i;

    // Clear bitboards for black and white pieces
    blackPawns          .clear();
    blackKnights        .clear();
    blackQueensBishops  .clear();
    blackQueensRooks    .clear();
    //
    whitePawns          .clear();
    whiteKnights        .clear();
    whiteQueensBishops  .clear();
    whiteQueensRooks    .clear();

    // Clear material counters
    whitePieceCount = 0;
    blackPieceCount = 0;
    
    materialSignature = 0;
    materialScore = 0;
    pstScoreOpening = 0;
    pstScoreEndgame = 0;

    // Clear hash codes
    hashCode.clear();
    pawnHashCode.clear();

    // Copy board and setup bitboards for specific pieces
    for( i=A1; i<=H8; i++ ) {
        signed char piece = b.piece[i];
        
        board.piece[i] = piece;
        
        materialScore += Score::Piece[ piece ];

        switch( piece ) {
        case BlackPawn:
            blackPawns.setBit( i );
            blackPieceCount += AllPawns_Unit;
            hashCode ^= Zobrist::BlackPawn[i];
            pawnHashCode ^= Zobrist::BlackPawn[i];
            updateSignatureAdd( Black, Pawn );
            break;
        case BlackKnight:
            pstScoreOpening -= Score::BlackKnight_Opening[ i ];
            pstScoreEndgame -= Score::BlackKnight_Endgame[ i ];

            blackKnights.setBit( i );
            blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            hashCode ^= Zobrist::BlackKnight[i];
            updateSignatureAdd( Black, Knight );
            break;
        case BlackBishop:
            pstScoreOpening -= Score::BlackBishop_Opening[ i ];
            pstScoreEndgame -= Score::BlackBishop_Endgame[ i ];

            blackQueensBishops.setBit( i );
            blackPieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            hashCode ^= Zobrist::BlackBishop[i];
            updateSignatureAdd( Black, Bishop );
            break;
        case BlackRook:
            pstScoreOpening -= Score::BlackRook_Opening[ i ];
            pstScoreEndgame -= Score::BlackRook_Endgame[ i ];

            blackQueensRooks.setBit( i );
            blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            hashCode ^= Zobrist::BlackRook[i];
            updateSignatureAdd( Black, Rook );
            break;
        case BlackQueen:
            pstScoreOpening -= Score::BlackQueen_Opening[ i ];
            pstScoreEndgame -= Score::BlackQueen_Endgame[ i ];

            blackQueensRooks.setBit( i );
            blackQueensBishops.setBit( i );
            blackPieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            hashCode ^= Zobrist::BlackQueen[i];
            updateSignatureAdd( Black, Queen );
            break;
        case BlackKing:
            pstScoreOpening -= Score::BlackKing_Opening[ i ];
            pstScoreEndgame -= Score::BlackKing_Endgame[ i ];

            blackKingSquare = i;
            hashCode ^= Zobrist::BlackKing[i];
            break;
        case WhitePawn:
            whitePawns.setBit( i );
            whitePieceCount += AllPawns_Unit;
            hashCode ^= Zobrist::WhitePawn[i];
            pawnHashCode ^= Zobrist::WhitePawn[i];
            updateSignatureAdd( White, Pawn );
            break;
        case WhiteKnight:
            pstScoreOpening += Score::WhiteKnight_Opening[ i ];
            pstScoreEndgame += Score::WhiteKnight_Endgame[ i ];

            whiteKnights.setBit( i );
            whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllKnights_Unit;
            hashCode ^= Zobrist::WhiteKnight[i];
            updateSignatureAdd( White, Knight );
            break;
        case WhiteBishop:
            pstScoreOpening += Score::WhiteBishop_Opening[ i ];
            pstScoreEndgame += Score::WhiteBishop_Endgame[ i ];

            whiteQueensBishops.setBit( i );
            whitePieceCount += AllPieces_Unit | MinorPieces_Unit | AllBishops_Unit;
            hashCode ^= Zobrist::WhiteBishop[i];
            updateSignatureAdd( White, Bishop );
            break;
        case WhiteRook:
            pstScoreOpening += Score::WhiteRook_Opening[ i ];
            pstScoreEndgame += Score::WhiteRook_Endgame[ i ];

            whiteQueensRooks.setBit( i );
            whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllRooks_Unit;
            hashCode ^= Zobrist::WhiteRook[i];
            updateSignatureAdd( White, Rook );
            break;
        case WhiteQueen:
            pstScoreOpening += Score::WhiteQueen_Opening[ i ];
            pstScoreEndgame += Score::WhiteQueen_Endgame[ i ];

            whiteQueensBishops.setBit( i );
            whiteQueensRooks.setBit( i );
            whitePieceCount += AllPieces_Unit | MajorPieces_Unit | AllQueens_Unit;
            hashCode ^= Zobrist::WhiteQueen[i];
            updateSignatureAdd( White, Queen );
            break;
        case WhiteKing:
            pstScoreOpening += Score::WhiteKing_Opening[ i ];
            pstScoreEndgame += Score::WhiteKing_Endgame[ i ];

            whiteKingSquare = i;
            hashCode ^= Zobrist::WhiteKing[i];
            break;
        }
    }

    // Initialize bit boards
    blackPieces.clear();
    whitePieces.clear();

    allPiecesDA1H8.clear();
    allPiecesDA8H1.clear();
    allPiecesTA1H8.clear();

    for( i=A1; i<=H8; i++ ) {
        if( board.piece[i] != None ) {
            allPiecesDA1H8.setBit( boardDA1H8[i] );
            allPiecesDA8H1.setBit( boardDA8H1[i] );
            allPiecesTA1H8.setBit( boardTA1H8[i] );
            if( (board.piece[i] & PieceSideMask) == Black ) {
                blackPieces.setBit( i );
            }
            else {
                whitePieces.setBit( i );
            }
        }
    }

    allPieces = blackPieces | whitePieces;
}

int Position::setBoard( const Board & board, bool whiteToMove, int enPassantSquare )
{
    setBoard( board );

    sideToPlay = whiteToMove ? White : Black;
    boardFlags = 0;  // En-passant square not available

    // TODO: implement castling flags
    // TODO: implement half move clock

    // Initialize check flag
    if( isSideInCheck(sideToPlay) ) {
        boardFlags |= SideToPlayInCheck;
    }

    // Set en-passant square
    if( enPassantSquare >= 0 ) {
        boardFlags |= EnPassantAvailable | (unsigned char) enPassantSquare;
    }

    return 0;
}

const Position & Position::operator = ( const char * s )
{
    setBoard( s );

    return *this;
}

void Position::dump( const char * header ) const
{
    FILE * f = Log::file();

    if( header != 0 ) {
        fprintf( f, "[position] %s\n", header );
    }

    board.dump();

    fprintf( f, "%s to play, material: %d, pst=%d/%d, flags: %x\n", 
        sideToPlay == White ? "White" : "Black",
        materialScore,
        pstScoreOpening,
        pstScoreEndgame,
        boardFlags );
}

int Position::operator == ( const Position & p ) const
{
    return(
        (board              == p.board) &&
        (sideToPlay         == p.sideToPlay) &&
        (boardFlags         == p.boardFlags) &&
        (blackKingSquare    == p.blackKingSquare) &&
        (whiteKingSquare    == p.whiteKingSquare) &&
        (blackPieceCount    == p.blackPieceCount) &&
        (whitePieceCount    == p.whitePieceCount) &&
        (blackPawns         == p.blackPawns) &&
        (blackKnights       == p.blackKnights) &&
        (blackQueensBishops == p.blackQueensBishops) &&
        (blackQueensRooks   == p.blackQueensRooks) &&
        (blackPieces        == p.blackPieces) &&
        (whitePawns         == p.whitePawns) &&
        (whiteKnights       == p.whiteKnights) &&
        (whiteQueensBishops == p.whiteQueensBishops) &&
        (whiteQueensRooks   == p.whiteQueensRooks) &&
        (whitePieces        == p.whitePieces) &&
        (allPieces          == p.allPieces) &&
        (allPiecesTA1H8     == p.allPiecesTA1H8) &&
        (allPiecesDA1H8     == p.allPiecesDA1H8) &&
        (allPiecesDA8H1     == p.allPiecesDA8H1) &&
        (materialScore      == p.materialScore) &&
        (pstScoreOpening    == p.pstScoreOpening) &&
        (pstScoreEndgame    == p.pstScoreEndgame) &&
        (hashCode           == p.hashCode) &&
        (pawnHashCode       == p.pawnHashCode) );
}

BitBoard Position::getAttacksFromSquare( int square ) const
{
    switch( board.piece[square] & PieceTypeMask ) {
    case Pawn:
        if( board.piece[square] == BlackPawn )
            return Attacks::BlackPawn[square];
        else
            return Attacks::WhitePawn[square];
    case Knight:
        return Attacks::Knight[square];
    case Bishop:
        return bishopAttacks( square );
    case Rook:
        return rookAttacks( square );
    case Queen:
        return bishopAttacks( square ) | rookAttacks( square );
    case King:
        return Attacks::King[square];
    }

    return BitBoard( 0 );
}

BitBoard Position::getAttacksToSquare( int square, int side ) const
{
    BitBoard    result;

    if( side == Black ) {
        result  = Attacks::WhitePawn[square] & blackPawns;
        result |= Attacks::Knight[square] & blackKnights;
        result |= Attacks::King[square] & BitBoard::Set[blackKingSquare];
        result |= bishopAttacks(square) & blackQueensBishops;
        result |= rookAttacks(square) & blackQueensRooks;
    }
    else {
        result  = Attacks::BlackPawn[square] & whitePawns;
        result |= Attacks::Knight[square] & whiteKnights;
        result |= Attacks::King[square] & BitBoard::Set[whiteKingSquare];
        result |= bishopAttacks(square) & whiteQueensBishops;
        result |= rookAttacks(square) & whiteQueensRooks;
    }

    return result;
}

int Position::isSquareAttackedBy( int square, int side ) const
{
    BitBoard    bb;

    if( side == Black ) {
        if( Attacks::WhitePawn[square] & blackPawns ) return 1;
        if( Attacks::Knight[square] & blackKnights ) return 1;
        if( Attacks::King[square] & BitBoard::Set[blackKingSquare] ) return 1;
        if( bishopAttacks(square) & blackQueensBishops ) return 1;
        if( rookAttacks(square) & blackQueensRooks ) return 1;
    }
    else {
        if( Attacks::BlackPawn[square] & whitePawns ) return 1;
        if( Attacks::Knight[square] & whiteKnights ) return 1;
        if( Attacks::King[square] & BitBoard::Set[whiteKingSquare] ) return 1;
        if( bishopAttacks(square) & whiteQueensBishops ) return 1;
        if( rookAttacks(square) & whiteQueensRooks ) return 1;
    }

    return 0;
}

// Same as isSquareAttackedBy() but doesn't take into account the king
bool Position::isSquareDefendedBy( int square, int side ) const
{
    BitBoard    bb;

    if( side == Black ) {
        if( rookAttacks(square) & blackQueensRooks ) 
            return true;

        if( bishopAttacks(square) & blackQueensBishops ) 
            return true;

        if( Attacks::Knight[square] & blackKnights ) 
            return true;

        if( Attacks::WhitePawn[square] & blackPawns ) 
            return true;
    }
    else {
        if( rookAttacks(square) & whiteQueensRooks ) 
            return true;

        if( bishopAttacks(square) & whiteQueensBishops ) 
            return true;

        if( Attacks::Knight[square] & whiteKnights ) 
            return true;

        if( Attacks::BlackPawn[square] & whitePawns ) 
            return true;
    }

    return false;
}

int Position::isSideInCheck( int side ) const
{
    Counters::callsToSideInCheck++;

    BitBoard    bb;
    int         sq;

    if( side == Black ) {
        sq = blackKingSquare;

        if( Attacks::Knight[sq] & whiteKnights )
            return 1;

        if( (Attacks::Rook[sq] & whiteQueensRooks) && (rookAttacks(sq) & whiteQueensRooks) ) 
            return 1;

        if( (Attacks::Bishop[sq] & whiteQueensBishops) && (bishopAttacks(sq) & whiteQueensBishops) )
            return 1;

        if( Attacks::BlackPawn[sq] & whitePawns )
            return 1;
    }
    else {
        sq = whiteKingSquare;

        if( Attacks::Knight[sq] & blackKnights ) 
            return 1;

        if( (Attacks::Rook[sq] & blackQueensRooks) && (rookAttacks(sq) & blackQueensRooks) ) 
            return 1;

        if( (Attacks::Bishop[sq] & blackQueensBishops) && (bishopAttacks(sq) & blackQueensBishops) ) 
            return 1;

        if( Attacks::WhitePawn[sq] & blackPawns ) 
            return 1;
    }

    return 0;
}

/*
    Checks to see if piece in the "from" square is hiding an attack (by a sliding piece) 
    from the specified direction. If so, the new attacker is added to the list.
    
*/
void Position::addXRayAttacker( BitBoard & attackers, int from, int attackDirection ) const
{
    switch( attackDirection ) {
    case DirRank_A:
        attackers |= rookAttacksOnRank( from ) & (blackQueensRooks | whiteQueensRooks) & Mask::SquaresTo1stFile[from];
        break;
    case DirRank_H:
        attackers |= rookAttacksOnRank( from ) & (blackQueensRooks | whiteQueensRooks) & Mask::SquaresTo8thFile[from];
        break;
    case DirFile_1:
        attackers |= rookAttacksOnFile( from ) & (blackQueensRooks | whiteQueensRooks) & Mask::SquaresTo1stRank[from];
        break;
    case DirFile_8:
        attackers |= rookAttacksOnFile( from ) & (blackQueensRooks | whiteQueensRooks) & Mask::SquaresTo8thRank[from];
        break;
    case DirA1H8_A1:
        attackers |= bishopAttacksOnDiagA1H8( from ) & (blackQueensBishops | whiteQueensBishops) & Mask::SquaresToDiagA1[from];
        break;
    case DirA1H8_H8:
        attackers |= bishopAttacksOnDiagA1H8( from ) & (blackQueensBishops | whiteQueensBishops) & Mask::SquaresToDiagH8[from];
        break;
    case DirA8H1_A8:
        attackers |= bishopAttacksOnDiagA8H1( from ) & (blackQueensBishops | whiteQueensBishops) & Mask::SquaresToDiagA8[from];
        break;
    case DirA8H1_H1:
        attackers |= bishopAttacksOnDiagA8H1( from ) & (blackQueensBishops | whiteQueensBishops) & Mask::SquaresToDiagH1[from];
        break;
    }
}

/*
    Evaluates the probable outcome of an exchange started by piece on "from" square
    capturing the piece on the "to" square. Takes into account x-rays, but not pins.

    Note: this code is based on Crafty's implementation.
*/
int Position::evaluateExchange( int from, int to ) const
{
    BitBoard    attacksToTarget;
    int         swapList[32];
    int         attackersCount;
    int         valueOfTarget;
    int         currentSide;
    int         attackDirection;

    // Compute all possible attacks to the target square, for both sides
    attacksToTarget =
        (Attacks::WhitePawn[to] & blackPawns) |
        (Attacks::BlackPawn[to] & whitePawns) |
        (Attacks::Knight[to] & (blackKnights | whiteKnights)) |
        (Attacks::King[to] & (BitBoard::Set[blackKingSquare] | BitBoard::Set[whiteKingSquare])) |
        (bishopAttacks(to) & (blackQueensBishops | whiteQueensBishops)) |
        (rookAttacks(to) & (blackQueensRooks | whiteQueensRooks));

    // Initialize the swap list
    attacksToTarget.clrBit( from ); // Remove the starting square from the list, as we're going to add it manually
    attackersCount = 1;
    swapList[ 0 ] = Score::PieceAbs[ board.piece[ to ] ];   // Force capture on target square...
    valueOfTarget = Score::PieceAbs[ board.piece[ from ] ]; // ...from starting square
    currentSide = OppositeSide( PieceSide( board.piece[ from ] ) );

    // Check for a discover attack
    attackDirection = Attacks::DirectionEx[ from ][ to ];
    if( attackDirection != DirNone ) {
        addXRayAttacker( attacksToTarget, from, attackDirection );
    }

    BitBoard    blackQueens = blackQueensBishops & blackQueensRooks;
    BitBoard    blackBishops = blackQueensBishops ^ blackQueens;
    BitBoard    blackRooks = blackQueensRooks ^ blackQueens;
    BitBoard    whiteQueens = whiteQueensBishops & whiteQueensRooks;
    BitBoard    whiteBishops = whiteQueensBishops ^ whiteQueens;
    BitBoard    whiteRooks = whiteQueensRooks ^ whiteQueens;

    while( attacksToTarget ) {
        // Get the least valuable attacker for the current side
        if( currentSide == Black ) {
            if( attacksToTarget & blackPawns ) {
                from = bitSearch( attacksToTarget & blackPawns );
            }
            else if( attacksToTarget & blackKnights ) {
                from = bitSearch( attacksToTarget & blackKnights );
            }
            else if( attacksToTarget & blackBishops ) {
                from = bitSearch( attacksToTarget & blackBishops );
            }
            else if( attacksToTarget & blackRooks ) {
                from = bitSearch( attacksToTarget & blackRooks );
            }
            else if( attacksToTarget & blackQueens ) {
                from = bitSearch( attacksToTarget & blackQueens );
            }
            else if( attacksToTarget.getBit( blackKingSquare ) && ! (attacksToTarget & whitePieces) ) {
                from = blackKingSquare;
            }
            else {
                // No more attackers for this side, exit
                break;
            }
        }
        else {
            if( attacksToTarget & whitePawns ) {
                from = bitSearch( attacksToTarget & whitePawns );
            }
            else if( attacksToTarget & whiteKnights ) {
                from = bitSearch( attacksToTarget & whiteKnights );
            }
            else if( attacksToTarget & whiteBishops ) {
                from = bitSearch( attacksToTarget & whiteBishops );
            }
            else if( attacksToTarget & whiteRooks ) {
                from = bitSearch( attacksToTarget & whiteRooks );
            }
            else if( attacksToTarget & whiteQueens ) {
                from = bitSearch( attacksToTarget & whiteQueens );
            }
            else if( attacksToTarget.getBit( whiteKingSquare ) && ! (attacksToTarget & blackPieces) ) {
                from = whiteKingSquare;
            }
            else {
                // No more attackers for this side, exit
                break;
            }
        }

        attacksToTarget.clrBit( from );

        // Add the piece to the swap list
        swapList[ attackersCount ] = valueOfTarget - swapList[ attackersCount - 1 ];
        attackersCount++;

        valueOfTarget = Score::PieceAbs[ board.piece[ from ] ];

        if( valueOfTarget == Score::King ) {
            break;
        }

        currentSide = OppositeSide( currentSide );

        // Check for a discover attack
        attackDirection = Attacks::DirectionEx[ from ][ to ];
        if( attackDirection != DirNone ) {
            addXRayAttacker( attacksToTarget, from, attackDirection );
        }
    }
    
    // Now resolve the capture sequence
    while( --attackersCount > 0 ) {
        if( swapList[ attackersCount ] > -swapList[ attackersCount - 1 ] ) {
            swapList[ attackersCount - 1 ] = -swapList[ attackersCount ];
        }
    }

    return swapList[ 0 ];
}
