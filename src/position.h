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
#ifndef POSITION_H_
#define POSITION_H_

#include "attacks.h"
#include "board.h"
#include "move.h"
#include "score.h"  // For the stage definitions

class MoveList;
class PawnHashEntry;
struct UndoInfo;

/*
    Material signatures are bit-packed as follows:

    Bits    Size    Description
    ----    ----    -----------
    0-7     8       White material (for stage)
    8-15    8       Black material (for stage)
    16-20   5       White material signature
    21-25   5       Black material signature
    26-30   5       Combined black/white material signature
    31      *       Unused

    Note: these constants are public because they are also used
    by the recognizer code.
*/
enum
{
    SignatureMaterialWhitePawn      = 0,    // Must be zero
    SignatureMaterialWhiteKnight    = Stage_MinorValue,
    SignatureMaterialWhiteBishop    = Stage_MinorValue,
    SignatureMaterialWhiteRook      = Stage_RookValue,
    SignatureMaterialWhiteQueen     = Stage_QueenValue,

    SignatureMaterialBlackPawn      = 0,    // Must be zero
    SignatureMaterialBlackKnight    = Stage_MinorValue << 8,
    SignatureMaterialBlackBishop    = Stage_MinorValue << 8,
    SignatureMaterialBlackRook      = Stage_RookValue << 8,
    SignatureMaterialBlackQueen     = Stage_QueenValue << 8,

    SignatureOffset     = 16,   // Offset of signature in bits

    SignatureWhitePawn  = 0x00010000,
    SignatureWhiteKnight= 0x00020000,
    SignatureWhiteBishop= 0x00040000,
    SignatureWhiteRook  = 0x00080000,
    SignatureWhiteQueen = 0x00100000,

    SignatureBlackPawn  = 0x00200000,
    SignatureBlackKnight= 0x00400000,
    SignatureBlackBishop= 0x00800000,
    SignatureBlackRook  = 0x01000000,
    SignatureBlackQueen = 0x02000000,
};

class Position
{
public:
    /*
        Board flags are bit packed as follows:

        Bits    Size    Description
        ----    ----    -----------
        0-6     7       En-passant square (high bit set means square not available)
        7       1       Unused
        8-15    8       Half move clock
        16-19   4       Castling availability
        20      1       White has castled
        21      1       Black has castled
        22      1       Side in check
        23-31   *       Unused
    */
    enum
    {
        EnPassantRawSquareMask  = 0x0000003F,
        EnPassantAvailable      = 0x00000040,
        EnPassantSquareMask     = EnPassantRawSquareMask | EnPassantAvailable,
        HalfMoveClockMask       = 0x0000FF00,
        HalfMoveClockShift      = 8,
        HalfMoveClockIncrement  = 1 << HalfMoveClockShift,
        PositionRepeatPossible  = 0x0000FC00,   // Half move clock >= 4

        // Castling availability
        WhiteCastleKing     = 0x00010000,
        WhiteCastleQueen    = 0x00020000,
        BlackCastleKing     = 0x00040000,
        BlackCastleQueen    = 0x00080000,
        AnyCastle           = WhiteCastleKing | WhiteCastleQueen | BlackCastleKing | BlackCastleQueen,

        // Flags
        WhiteHasCastled     = 0x00100000,
        BlackHasCastled     = 0x00200000,

        SideToPlayInCheck   = 0x00400000,   // Side to play is in check
    };

    // Default constructor (sets the standard initial position)
    Position();

    // Copy constructor
    Position( const Position & );

    //
    ~Position() {
    }

    // Assignment operator
    const Position & operator = ( const Position & );

    // Set the position from a FEN string
    const Position & operator = ( const char * fen );

    int setBoard( const char * fen, int * moveNumber = 0 );

    int setBoard( const Board & board, bool whiteToMove, int enPassantSquare = -1 );

    int getBoard( char * fen, int moveNumber = 0 ) const;

    //
    int getEvaluation() const;

    PawnHashEntry * evaluatePawnStructure() const;

    int evaluateDevelopment() const;

    int evaluatePawnRaces( const BitBoard &, const BitBoard & ) const;

    int evaluateKingShield( int pos, int side ) const;

    int evaluateExchange( int, int ) const;

    int evaluatePatterns() const;

    int evaluatePassedPawns() const;

    int getHalfMoveClock() const {
        return (int) ((boardFlags & HalfMoveClockMask) >> HalfMoveClockShift);
    }

    //
    int         doMove( Move & m );
    int         doNullMove();
    int         isValidMove( const Move & m ) const;
    void        undoMove( const Move & m, const UndoInfo & info );
    void        undoNullMove( const UndoInfo & info );

    void        generateMoves( MoveList & moves ) const;
    void        generateMovesToSquare( MoveList & moves, int to ) const;
    void        generateTactical( MoveList & moves ) const;
    void        generateNonTactical( MoveList & moves ) const;
    void        generateCheckEscapes( MoveList & moves ) const;
    void        generateValidMoves( MoveList & moves, int to = -1 ) const;

    //
    BitBoard    getAttacksFromSquare( int square ) const;
    BitBoard    getAttacksToSquare  ( int square, int side ) const;
    int         isSideInCheck       ( int side ) const;
    int         isSquareAttackedBy  ( int square, int side ) const;
    bool        isSquareDefendedBy  ( int square, int side ) const;

    // Boolean operators
    int operator == ( const Position & ) const;

    int operator != ( const Position & p ) const {
        return ! operator == (p);
    }

    // Initial position (FEN)
    static const char * startPosition;

    //
    void dump( const char * header = 0 ) const;

    bool blackToMove() const {
        return sideToPlay == Black;
    }

    bool whiteToMove() const {
        return sideToPlay != Black;
    }

    BitBoard relativeHashCode() const {
        return hashCode;
    }

    /** Returns true (non-zero) if the side on move is in check, false (zero) otherwise. */
    unsigned isSideToMoveInCheck() const {
        return boardFlags & SideToPlayInCheck;
    }

    /** Returns the number of black pawns on the board. */
    unsigned numOfBlackPawns() const {
        return (blackPieceCount & AllPawns_Mask) >> AllPawns_Shift;
    }

    /** Returns the number of black bishops on the board. */
    unsigned numOfBlackBishops() const {
        return (blackPieceCount & AllBishops_Mask) >> AllBishops_Shift;
    }

    /** Returns the number of black knights on the board. */
    unsigned numOfBlackKnights() const {
        return (blackPieceCount & AllKnights_Mask) >> AllKnights_Shift;
    }

    /** Returns the number of black rooks on the board. */
    unsigned numOfBlackRooks() const {
        return (blackPieceCount & AllRooks_Mask) >> AllRooks_Shift;
    }

    /** Returns the number of black queens on the board. */
    unsigned numOfBlackQueens() const {
        return (blackPieceCount & AllQueens_Mask) >> AllQueens_Shift;
    }

    /** Returns the number of black pieces (king and pawns excluded) on the board. */
    unsigned numOfBlackPieces() const {
        return (blackPieceCount & AllPieces_Mask) >> AllPieces_Shift;
    }

    /** Returns the number of black minor pieces on the board. */
    unsigned numOfBlackMinorPieces() const {
        return (blackPieceCount & MinorPieces_Mask) >> MinorPieces_Shift;
    }

    /** Returns the number of black major pieces on the board. */
    unsigned numOfBlackMajorPieces() const {
        return (blackPieceCount & MajorPieces_Mask) >> MajorPieces_Shift;
    }

    /** Returns true if black has one or more pawns on the board. */
    bool hasBlackPawns() const {
        return (blackPieceCount & AllPawns_Mask) != 0;
    }

    /** Returns true if black has one or more pieces (non-pawns) on the board. */
    bool hasBlackPieces() const {
        return (blackPieceCount & AllPieces_Mask) != 0;
    }

    /** Returns true if black has one or more knights on the board. */
    bool hasBlackKnights() const {
        return (blackPieceCount & AllKnights_Mask) != 0;
    }

    /** Returns true if black has one or more bishops on the board. */
    bool hasBlackBishops() const {
        return (blackPieceCount & AllBishops_Mask) != 0;
    }

    /** Returns true if black has one or more rooks on the board. */
    bool hasBlackRooks() const {
        return (blackPieceCount & AllRooks_Mask) != 0;
    }

    /** Returns true if black has one or more queens on the board. */
    bool hasBlackQueens() const {
        return (blackPieceCount & AllQueens_Mask) != 0;
    }

    /** Returns true if black has at least one knight or bishop on the board. */
    bool hasBlackMinorPieces() const {
        return (blackPieceCount & MinorPieces_Mask) != 0;
    }

    /** Returns true if black has at least one rook or queen on the board. */
    bool hasBlackMajorPieces() const {
        return (blackPieceCount & MajorPieces_Mask) != 0;
    }

    /** Returns the number of white pawns on the board. */
    unsigned numOfWhitePawns() const {
        return (whitePieceCount & AllPawns_Mask) >> AllPawns_Shift;
    }

    /** Returns the number of white bishops on the board. */
    unsigned numOfWhiteBishops() const {
        return (whitePieceCount & AllBishops_Mask) >> AllBishops_Shift;
    }

    /** Returns the number of white knights on the board. */
    unsigned numOfWhiteKnights() const {
        return (whitePieceCount & AllKnights_Mask) >> AllKnights_Shift;
    }

    /** Returns the number of white rooks on the board. */
    unsigned numOfWhiteRooks() const {
        return (whitePieceCount & AllRooks_Mask) >> AllRooks_Shift;
    }

    /** Returns the number of white queens on the board. */
    unsigned numOfWhiteQueens() const {
        return (whitePieceCount & AllQueens_Mask) >> AllQueens_Shift;
    }

    /** Returns the number of white pieces (king and pawns excluded) on the board. */
    unsigned numOfWhitePieces() const {
        return (whitePieceCount & AllPieces_Mask) >> AllPieces_Shift;
    }

    /** Returns the number of white minor pieces on the board. */
    unsigned numOfWhiteMinorPieces() const {
        return (whitePieceCount & MinorPieces_Mask) >> MinorPieces_Shift;
    }

    /** Returns the number of white major pieces on the board. */
    unsigned numOfWhiteMajorPieces() const {
        return (whitePieceCount & MajorPieces_Mask) >> MajorPieces_Shift;
    }

    /** Returns true if white has one or more pawns on the board. */
    bool hasWhitePawns() const {
        return (whitePieceCount & AllPawns_Mask) != 0;
    }

    /** Returns true if white has one or more pieces (non-pawns) on the board. */
    bool hasWhitePieces() const {
        return (whitePieceCount & AllPieces_Mask) != 0;
    }

    /** Returns true if white has one or more knights on the board. */
    bool hasWhiteKnights() const {
        return (whitePieceCount & AllKnights_Mask) != 0;
    }

    /** Returns true if white has one or more bishops on the board. */
    bool hasWhiteBishops() const {
        return (whitePieceCount & AllBishops_Mask) != 0;
    }

    /** Returns true if white has one or more rooks on the board. */
    bool hasWhiteRooks() const {
        return (whitePieceCount & AllRooks_Mask) != 0;
    }

    /** Returns true if white has one or more queens on the board. */
    bool hasWhiteQueens() const {
        return (whitePieceCount & AllQueens_Mask) != 0;
    }

    /** Returns true if white has at least one knight or bishop on the board. */
    bool hasWhiteMinorPieces() const {
        return (whitePieceCount & MinorPieces_Mask) != 0;
    }

    /** Returns true if white has at least one rook or queen on the board. */
    bool hasWhiteMajorPieces() const {
        return (whitePieceCount & MajorPieces_Mask) != 0;
    }

    /** Returns the current stage for white, based on black's material. */
    unsigned getWhiteStage() const {
        unsigned result = (int) ((materialSignature >> 8) & 0xFF);

        return result < (int) Stage_Cap ? result : Stage_Cap;
    }

    /** Returns the current stage for black, based on white's material. */
    unsigned getBlackStage() const {
        unsigned result = (int) (materialSignature & 0xFF);

        return result < (int) Stage_Cap ? result : Stage_Cap;
    }

    /** Returns the "short" (5-bits) material signature (combines both black and white pieces). */
    unsigned getShortMaterialSignature() const {
        return ((materialSignature >> 16) | (materialSignature >> 21)) & 0x1F;
    }

    /** Returns the 10-bits material signature. */
    unsigned getMaterialSignature() const {
        return (materialSignature >> 16) & 0x3FF;
    }

    void verifyHashCode() const;

public:
    enum {
        // Note: even if all pawns promote, the piece count cannot
        // exceed 15 because the king isn't included
        AllPieces_Shift     = 0,
        AllPieces_Bits      = 4,
        AllPieces_Unit      = 1 << AllPieces_Shift,
        AllPieces_Mask      = ((1 << AllPieces_Bits) - 1) << AllPieces_Shift,

        AllPawns_Shift      = AllPieces_Shift + AllPieces_Bits,
        AllPawns_Bits       = 4,
        AllPawns_Unit       = 1 << AllPawns_Shift,
        AllPawns_Mask       = ((1 << AllPawns_Bits) - 1) << AllPawns_Shift,

        MajorPieces_Shift   = AllPawns_Shift + AllPawns_Bits,
        MajorPieces_Bits    = 4,
        MajorPieces_Unit    = 1 << MajorPieces_Shift,
        MajorPieces_Mask    = ((1 << MajorPieces_Bits) - 1) << MajorPieces_Shift,

        MinorPieces_Shift   = MajorPieces_Shift + MajorPieces_Bits,
        MinorPieces_Bits    = 4,
        MinorPieces_Unit    = 1 << MinorPieces_Shift,
        MinorPieces_Mask    = ((1 << MinorPieces_Bits) - 1) << MinorPieces_Shift,

        AllQueens_Shift     = MinorPieces_Shift + MinorPieces_Bits,
        AllQueens_Bits      = 4,
        AllQueens_Unit      = 1 << AllQueens_Shift,
        AllQueens_Mask      = ((1 << AllQueens_Bits) - 1) << AllQueens_Shift,

        AllRooks_Shift      = AllQueens_Shift + AllQueens_Bits,
        AllRooks_Bits       = 4,
        AllRooks_Unit       = 1 << AllRooks_Shift,
        AllRooks_Mask       = ((1 << AllRooks_Bits) - 1) << AllRooks_Shift,

        AllKnights_Shift    = AllRooks_Shift + AllRooks_Bits,
        AllKnights_Bits     = 4,
        AllKnights_Unit     = 1 << AllKnights_Shift,
        AllKnights_Mask     = ((1 << AllKnights_Bits) - 1) << AllKnights_Shift,

        AllBishops_Shift    = AllKnights_Shift + AllKnights_Bits,
        AllBishops_Bits     = 4,
        AllBishops_Unit     = 1 << AllBishops_Shift,
        AllBishops_Mask     = ((1 << AllBishops_Bits) - 1) << AllBishops_Shift,
    };

    Board           board;
    
    int             blackKingSquare;
    int             whiteKingSquare;
    
#if 0
    unsigned        whitePieceCount;
    BitBoard        whitePieces;
    BitBoard        whitePawns;
    BitBoard        whiteKnights;
    BitBoard        whiteQueensBishops;
    BitBoard        whiteQueensRooks;
    
    unsigned        blackPieceCount;
    BitBoard        blackPieces;
    BitBoard        blackPawns;
    BitBoard        blackKnights;
    BitBoard        blackQueensBishops;
    BitBoard        blackQueensRooks;
#else
    unsigned        whitePieceCount;
    unsigned        blackPieceCount;
    BitBoard        whitePieces;
    BitBoard        blackPieces;
    BitBoard        whitePawns;
    BitBoard        blackPawns;
    BitBoard        whiteKnights;
    BitBoard        blackKnights;
    BitBoard        whiteQueensBishops;
    BitBoard        blackQueensBishops;
    BitBoard        whiteQueensRooks;
    BitBoard        blackQueensRooks;
#endif
    
    BitBoard        allPieces;
    BitBoard        allPiecesTA1H8;
    BitBoard        allPiecesDA1H8;
    BitBoard        allPiecesDA8H1;
    //
    BitBoard        hashCode;
    BitBoard        pawnHashCode;
    //
    unsigned        boardFlags;
    //
    int             sideToPlay;
    //
    unsigned        materialSignature;
    int             materialScore;
    int             pstScoreOpening;
    int             pstScoreEndgame;

private:
    void addXRayAttacker( BitBoard & attackers, int from, int attackDirection ) const;
    void setBoard( const Board & b );
};

#define rookAttacksOnRank( from )   \
    (Attacks::RookRank[from][allPieces.getRank(from / 8)])

#define rookAttacksOnFile( from )   \
    (Attacks::RookFile[from][allPiecesTA1H8.getRank(from % 8)])

#define rookAttacks( from )         \
    (rookAttacksOnRank(from) | rookAttacksOnFile(from))

#define bishopAttacksOnDiagA1H8( from ) \
    (Attacks::BishopA1H8[from][(allPiecesDA1H8 >> shiftDA1H8[from]).toByte()])

#define bishopAttacksOnDiagA8H1( from ) \
    (Attacks::BishopA8H1[from][(allPiecesDA8H1 >> shiftDA8H1[from]).toByte()])

#define bishopAttacks( from )           \
    (bishopAttacksOnDiagA1H8(from) | bishopAttacksOnDiagA8H1(from))

#define updateSignatureAdd( color, piece ) \
    materialSignature += SignatureMaterial##color##piece; \
    materialSignature |= Signature##color##piece;

#define updateSignatureRemove( color, piece ) \
    materialSignature -= SignatureMaterial##color##piece; \
    if( ! has##color##piece##s() ) materialSignature &= ~Signature##color##piece;

#endif // POSITION_H_
