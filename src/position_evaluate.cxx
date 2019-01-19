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
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "counters.h"
#include "engine.h"
#include "hash.h"
#include "log.h"
#include "mask.h"
#include "metrics.h"
#include "pawnhash.h"
#include "position.h"
#include "score.h"

#undef PRINT
//#define PRINT( s )  printf s
//#define PRINT( s )  Log::write s
#ifndef PRINT
#define PRINT( s )
#endif

#define HAVE_MOBILITY
#define HAVE_XRAY_QR

const bool  scoreQuantizationEnabled    = true;

#define AttackedByBlackPawn( square )   (Attacks::WhitePawn[square] & blackPawns)
#define AttackedByWhitePawn( square )   (Attacks::BlackPawn[square] & whitePawns)

#define SafeFromBlackPawns( square )    (!(Attacks::BlackPawnCouldAttack[square] & blackPawns))
#define SafeFromWhitePawns( square )    (!(Attacks::WhitePawnCouldAttack[square] & whitePawns))

#define IsBlackPawnPassed( square )     (!(Mask::BlackPassed[square] & whitePawns))
#define IsWhitePawnPassed( square )     (!(Mask::WhitePassed[square] & blackPawns))

#define sqname( sq )                    Board::squareName( sq )

// Attack value is made of strength (bits 0-15) and number of attackers (bits 24-31)

/*
    TODO: we could have two masks for king attack, one with the 3x3 square surrounding
    the king and another one with the pawns in said square. Then, attacks that only
    hit the pawns could be penalized by some factor.
*/

#define MK_ATK( value ) (value + 0x01000000)

const int   PawnAtk     = MK_ATK( 10 ) - MK_ATK( 0 );
const int   KnightAtk   = MK_ATK( 25 );
const int   BishopAtk   = MK_ATK( 25 );
const int   RookAtk     = MK_ATK( 40 );
const int   QueenAtk    = MK_ATK( 90 );
const int   KingAtk     = MK_ATK( 10 );

const int   AttackStrength[16] = {
    0, 32, 128, 192, 224, 248, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
};

const int   KingDefectsPenalty[8] = {
    0,  5, 15, 30, 45, 60, 60, 60
};

const int   PassedPawnBonus_Opening[8] = {
    0, 10, 20, 30, 40, 50, 65, 100
};

const int   PassedPawnBonus_Endgame[8] = {
    0, 15, 30, 50, 75, 100, 130, 200
};

const int   TrappedRookPenalty      = 60;
const int   TrappedBishopPenalty    = 100;

EvalItem evalCache[ ItemsInEvalCache ];

int Position::getEvaluation() const
{
    // Evaluate draws for insufficient material, and remember whether a side can win or not
    bool blackCanWin = true;
    bool whiteCanWin = true;

    if( ! hasBlackPawns() && ! hasBlackMajorPieces() && numOfBlackMinorPieces() <= 1 ) {
        blackCanWin = false;
    }

    if( ! hasWhitePawns() && ! hasWhiteMajorPieces() && numOfWhiteMinorPieces() <= 1 ) {
        whiteCanWin = false;
    }

    if( ! whiteCanWin && ! blackCanWin ) {
        return 0;
    }

    // Probe eval cache
    EvalItem * ev_item = evalCache + (((hashCode >> 32).toUnsigned()) & (ItemsInEvalCache-1));

    if( ev_item->code == hashCode.toUnsigned() ) {
        return ev_item->eval;
    }

    BitBoard    allPawns    = blackPawns | whitePawns;

    BitBoard    whiteEnemyOrEmpty   = ~whitePieces;
    BitBoard    blackEnemyOrEmpty   = ~blackPieces;

    BitBoard    whiteKingDanger = Mask::KingAttack_Danger[ whiteKingSquare ];
    BitBoard    blackKingDanger = Mask::KingAttack_Danger[ blackKingSquare ];
    BitBoard    blackPassed( 0 );
    BitBoard    whitePassed( 0 );

    BitBoard    bb;
    int         pos;
    int         positionalScore = 0;
    int         posOpening = 0;
    int         posEndgame = 0;
    unsigned    whiteAttack = 0;
    unsigned    blackAttack = 0;
    unsigned    whiteDefend = KingAtk;
    unsigned    blackDefend = KingAtk;

    int         result = 0;

    int stage = getWhiteStage() + getBlackStage();

    positionalScore += evaluateDevelopment();
    positionalScore += evaluatePatterns();

    PRINT(( "Positional at startup = %d\n", positionalScore ));

    //--------------------------------------------------
    //
    // Pawn structure
    //
    //--------------------------------------------------
    PawnHashEntry * entry = Engine::getPawnHashTable()->probe( *this );

    if( entry == 0 ) {
        entry = evaluatePawnStructure();
    }

    int pawnOpening = (((entry->whiteScore >> 16) & 0xFFFF) - 0x8000) - (((entry->blackScore >> 16) & 0xFFFF) - 0x8000);
    int pawnEndgame = ((entry->whiteScore & 0xFFFF) - 0x8000) - ((entry->blackScore & 0xFFFF) - 0x8000);

    // Note: one may be tempted use a "switch" statemement on the board pieces,
    // which would make for some cleaner code. Surprisingly, last time I tried
    // that the program run *a lot* slower. Probably, short loops like those
    // below make better use of the CPU cache...

    // Mobility evaluation method is from Fruit
    const int ExpKnightMob  =  5;   // In open board: min=2, max=8
    const int ExpBishopMob  =  5;   // In open board: min=7, max=13
    const int ExpRookMob    =  8;   // In open board: min=max=14
    const int ExpQueenMob   = 12;   // In open board: min=21, max=27

    const int ValKnightMobOpening   = 3;
    const int ValKnightMobEndgame   = 3;
    const int ValBishopMobOpening   = 4;
    const int ValBishopMobEndgame   = 4;
    const int ValRookMobOpening     = 2;
    const int ValRookMobEndgame     = 4;
    const int ValQueenMobOpening    = 0;
    const int ValQueenMobEndgame    = 2;

    int mobOpening = 0;
    int mobEndgame = 0;

    BitBoard atk;
    int x;

    BitBoard    bpAtk;
    BitBoard    wpAtk;

    //--------------------------------------------------
    //
    // Pawn
    //
    //--------------------------------------------------
    bb = ((blackPawns & Mask::NotFile[0]) >> 9) | ((blackPawns & Mask::NotFile[7]) >> 7);

    bpAtk = bb;

    if( bb & whiteKingDanger ) {
        blackAttack += PawnAtk;
    }

    bb = ((whitePawns & Mask::NotFile[7]) << 9) | ((whitePawns & Mask::NotFile[0]) << 7);

    wpAtk = bb;

    if( bb & blackKingDanger ) {
        whiteAttack += PawnAtk;
    }

    //--------------------------------------------------
    //
    // Passed pawns
    //
    //--------------------------------------------------
    bb = blackPawns;
    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        if( (Mask::BlackPassed[pos] & whitePawns).isZero() && (Mask::SquaresTo1stRank[pos] & blackPawns).isZero() ) {
            PRINT(( "The black pawn %s is passed\n", sqname(pos) ));

            int bonus =
                (PassedPawnBonus_Opening[ 7 - RankOfSquare(pos) ]*stage +
                PassedPawnBonus_Endgame[ 7 - RankOfSquare(pos) ]*(2*Stage_Max-stage)) / (2*Stage_Max);

            result -= bonus;

            PRINT(( "  ...bonus = %d\n", bonus ));

            blackPassed.setBit( pos );

            // Protected pawn
            if( (Attacks::WhitePawn[pos] & blackPawns).isNotZero() ) {
                PRINT(( "  ...and protected\n" ));

                result -= bonus / 4;
            }

            // King is better close to the pawn in the endgame
            posEndgame += (distance( pos, blackKingSquare ) * bonus) / 16;

            PRINT(( "  ...king distance = %d\n", (distance( pos, blackKingSquare ) * bonus) / 16 ));

            // Rook behind passed pawn
            BitBoard tmp = rookAttacksOnFile(pos) & blackQueensRooks;
            if( tmp ) {
                tmp &= Mask::SquaresTo8thRank[pos];
                tmp &= ~(blackQueensBishops & blackQueensRooks);

                if( tmp ) {
                    PRINT(( "  ...pushed by rook\n" ));

                    posEndgame -= Score::RookBehindPassedPawn; // Rook behind passer
                }
            }

            // Blocked pawn
            if( whitePieces.getBit( pos-8 ) ) {
                int penalty = (bonus * 2) / 4;

                if( board.piece[pos-8] == WhiteKnight ) {
                    PRINT(( "  ...blocked by knight\n" ));
                    penalty += bonus / 8;
                }
                
                PRINT(( "  ...blocked, penalty = %d\n", penalty ));

                result += penalty;
            }
        }
    }

    bb = whitePawns;
    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        if( (Mask::WhitePassed[pos] & blackPawns).isZero() && (Mask::SquaresTo8thRank[pos] & whitePawns).isZero() ) {
            PRINT(( "The white pawn %s is passed\n", sqname(pos) ));

            int bonus =
                (PassedPawnBonus_Opening[ RankOfSquare(pos) ]*stage +
                PassedPawnBonus_Endgame[ RankOfSquare(pos) ]*(2*Stage_Max-stage)) / (2*Stage_Max);

            PRINT(( "  ...bonus = %d\n", bonus ));

            result += bonus;

            whitePassed.setBit( pos );

            // Protected pawn
            if( (Attacks::BlackPawn[pos] & whitePawns).isNotZero() ) {
                PRINT(( "  ...and protected\n" ));

                result += bonus / 4;
            }

            // King is better close to the pawn in the endgame
            posEndgame -= (distance( pos, whiteKingSquare ) * bonus) / 16;

            PRINT(( "  ...king distance = %d\n", (distance( pos, whiteKingSquare ) * bonus) / 16 ));

            // Rook behind passed pawn
            BitBoard tmp = rookAttacksOnFile(pos) & whiteQueensRooks;
            if( tmp ) {
                tmp &= Mask::SquaresTo1stRank[pos];
                tmp &= ~(whiteQueensBishops & whiteQueensRooks);

                if( tmp ) {
                    PRINT(( "  ...pushed by rook\n" ));

                    posEndgame += Score::RookBehindPassedPawn; // Rook behind passer
                }
            }

            // Blocked pawn
            if( blackPieces.getBit( pos+8 ) ) {
                int penalty = (bonus * 2) / 4;

                if( board.piece[pos+8] == BlackKnight ) {
                    PRINT(( "  ...blocked by knight\n" ));
                    penalty += bonus / 8;
                }
                
                PRINT(( "  ...blocked, penalty = %d\n", penalty ));

                result -= penalty;
            }
        }
    }

    //--------------------------------------------------
    //
    // Knight
    //
    //--------------------------------------------------
    bb = blackKnights;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        atk = Attacks::Knight[pos];


#ifdef HAVE_MOBILITY
        x = bitCount( atk & blackEnemyOrEmpty ) - ExpKnightMob;

        mobOpening -= ValKnightMobOpening * x;
        mobEndgame -= ValKnightMobEndgame * x;
#endif

        if( atk & whiteKingDanger ) {
            blackAttack += KnightAtk;
        }

        if( atk & blackKingDanger ) {
            blackDefend += KnightAtk;
        }

        // Outpost
        if( SafeFromWhitePawns(pos) ) {
        }
    }

    bb = whiteKnights;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        atk = Attacks::Knight[pos];

#ifdef HAVE_MOBILITY
        x = bitCount( atk & whiteEnemyOrEmpty ) - ExpKnightMob;

        mobOpening += ValKnightMobOpening * x;
        mobEndgame += ValKnightMobEndgame * x;
#endif

        if( atk & blackKingDanger ) {
            whiteAttack += KnightAtk;
        }

        if( atk & whiteKingDanger ) {
            whiteDefend += KnightAtk;
        }

        // Outpost
        if( SafeFromBlackPawns(pos) ) {
        }
    }

    //--------------------------------------------------
    //
    // Bishop
    //
    //--------------------------------------------------
    BitBoard    blackQueens = blackQueensBishops & blackQueensRooks;

    bb = blackQueensBishops ^ blackQueens;

    if( bitCount(bb) >= 2 ) { // TODO: we ignore promotions for now
        positionalScore -= Score::BishopPair;
    }

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        PRINT(( "Black bishop at %s\n", sqname(pos) ));

        atk = bishopAttacks( pos );

        x = bitCount( atk & blackEnemyOrEmpty ) - ExpBishopMob;

        PRINT(( "  mobility = %d\n", x ));

        mobOpening -= ValBishopMobOpening * x;
        mobEndgame -= ValBishopMobEndgame * x;

        if( atk & whiteKingDanger ) {
            blackAttack += BishopAtk;
        }

        if( atk & blackKingDanger ) {
            blackDefend += BishopAtk;
        }

        if( blackPassed || whitePassed ) {
            posEndgame -= Score::EndgameBishopWithPassers;
        }
    }

    BitBoard    whiteQueens = whiteQueensBishops & whiteQueensRooks;

    bb = whiteQueensBishops ^ whiteQueens;

    if( bitCount(bb) >= 2 ) { // TODO: we ignore promotions for now
        positionalScore += Score::BishopPair;
    }

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        PRINT(( "White bishop at %s\n", sqname(pos) ));

        atk = bishopAttacks( pos );

        x = bitCount( atk & whiteEnemyOrEmpty ) - ExpBishopMob;

        PRINT(( "  mobility = %d\n", x ));

        mobOpening += ValBishopMobOpening * x;
        mobEndgame += ValBishopMobEndgame * x;

        if( atk & blackKingDanger ) {
            whiteAttack += BishopAtk;
        }

        if( atk & whiteKingDanger ) {
            whiteDefend += BishopAtk;
        }

        if( blackPassed || whitePassed ) {
            posEndgame += Score::EndgameBishopWithPassers;
        }
    }

#ifdef HAVE_XRAY_QR
    // Rook + Queen preparation: basically we prepare bitboards with all
    // pieces but the white and black queens/rooks, which allows any
    // rook or queen to be automatically "x-rayed" thru other rooks or
    // queens of the same color
    BitBoard    rotNoBlackQR = allPiecesTA1H8;
    BitBoard    rotNoWhiteQR = allPiecesTA1H8;

    bb = blackQueensRooks;
    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        rotNoBlackQR ^= SetTA1H8[pos];
    }

    bb = whiteQueensRooks;
    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        rotNoWhiteQR ^= SetTA1H8[pos];
    }

    BitBoard    allNoBlackQR = allPieces ^ blackQueensRooks;
    BitBoard    allNoWhiteQR = allPieces ^ whiteQueensRooks;
#endif

    //--------------------------------------------------
    //
    // Rook
    //
    //--------------------------------------------------
    bb = blackQueensRooks ^ blackQueens;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        int file = FileOfSquare( pos );

        PRINT(( "Black rook at %s\n", sqname(pos) ));

        // Mobility
#ifndef HAVE_XRAY_QR
        atk = rookAttacks( pos );
#else
        atk =
            (Attacks::RookRank[pos][allNoBlackQR.getRank(pos / 8)]) |
            (Attacks::RookFile[pos][rotNoBlackQR.getRank(pos % 8)]);
#endif

#ifdef HAVE_MOBILITY
        x = bitCount( atk & blackEnemyOrEmpty ) - ExpRookMob;

        PRINT(( "  mobility = %d\n", x ));

        mobOpening -= ValRookMobOpening * x;
        mobEndgame -= ValRookMobEndgame * x;

        x = bitCount( atk & blackEnemyOrEmpty & ~wpAtk );

        if( x <= 1 ) {
            posEndgame += 150;
        }
#endif

        // 8/p7/1p2p3/3p1k2/1R1P3P/P2r2P1/5P2/6K1 w - - 0 35

        // Attack
        if( atk & whiteKingDanger ) {
            blackAttack += RookAtk;
        }

        if( atk & blackKingDanger ) {
            blackDefend += RookAtk;
        }

        // Open files
        if( !(Mask::File[file] & allPawns) ) {
            PRINT(( "  on open file (%d)\n", Score::RookOnOpenFile[file] ));
            //positionalScore -= Score::RookOnOpenFile[file];
            posOpening -= Score::RookOnOpenFile[file];
        }
        else if( ! (Mask::File[file] & blackPawns ) ) {
            PRINT(( "  on half-open file\n" ));
            positionalScore -= Score::RookOnHalfOpenFile;
        }

        // Seventh rank (i.e. second rank for black)
        if( RankOfSquare(pos) == 1 ) {
            if( (whiteKingSquare <= H1) && (Mask::Rank[1] & whitePawns) ) {
                PRINT(( "  on 7th rank\n" ));

                // The king is trapped or there are pawns to attack
                posOpening -= Score::RookOn7thRank_Opening;
                posEndgame -= Score::RookOn7thRank_Endgame;

                // If the king cannot escape from the 2nd rank by moving
                // behind a pawn we have an "absolute" 2nd rank
                if( !(Mask::Absolute2nd & whitePawns) ) {
                    positionalScore -= Score::RookOn7thRankAbsolute;
                    PRINT(( "  on 7th rank absolute\n" ));
                }
            }
        }
    }

    bb = whiteQueensRooks ^ whiteQueens;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        int file = FileOfSquare( pos );

        PRINT(( "White rook at %s\n", sqname(pos) ));

        // Mobility
#ifndef HAVE_XRAY_QR
        atk = rookAttacks( pos );
#else
        atk =
            (Attacks::RookRank[pos][allNoWhiteQR.getRank(pos / 8)]) |
            (Attacks::RookFile[pos][rotNoWhiteQR.getRank(pos % 8)]);
#endif

#ifdef HAVE_MOBILITY
        x = bitCount( atk & whiteEnemyOrEmpty ) - ExpRookMob;

        PRINT(( "  mobility = %d\n", x ));

        mobOpening += ValRookMobOpening * x;
        mobEndgame += ValRookMobEndgame * x;

        x = bitCount( atk & whiteEnemyOrEmpty & ~bpAtk );

        // 8/p7/1p2p3/3p1k2/1R1P3P/P2r2P1/5P2/6K1 w - - 0 35

        if( x <= 1 ) {
            posEndgame -= 150;
        }
#endif

        // Attack
        if( atk & blackKingDanger ) {
            whiteAttack += RookAtk;
        }

        if( atk & whiteKingDanger ) {
            whiteDefend += RookAtk;
        }

        // Open files
        if( !(Mask::File[file] & allPawns) ) {
            PRINT(( "  on open file\n", Score::RookOnOpenFile[file] ));
            //positionalScore += Score::RookOnOpenFile[file];
            posOpening += Score::RookOnOpenFile[file];
        }
        else if( ! (Mask::File[file] & whitePawns ) ) {
            PRINT(( "  on half-open file\n" ));
            positionalScore += Score::RookOnHalfOpenFile;
        }

        // Seventh rank (i.e. second rank for black)
        if( RankOfSquare(pos) == 6 ) {
            if( (blackKingSquare >= A8) && (Mask::Rank[6] & blackPawns) ) {
                PRINT(( "  on 7th rank\n" ));

                // The king is trapped or there are pawns to attack
                posOpening += Score::RookOn7thRank_Opening;
                posEndgame += Score::RookOn7thRank_Endgame;

                // If the king cannot escape from the 7th rank by moving
                // behind a pawn we have an "absolute" 7th rank
                if( !(Mask::Absolute7th & blackPawns) ) {
                    positionalScore += Score::RookOn7thRankAbsolute;
                    PRINT(( "  on 7th rank absolute\n" ));
                }
            }
        }
    }

    //--------------------------------------------------
    //
    // Queen
    //
    //--------------------------------------------------
    bb = blackQueens;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        PRINT(( "Black queen at %s\n", sqname(pos) ));

        // Mobility
#ifndef HAVE_XRAY_QR
        atk = rookAttacks( pos ) | bishopAttacks( pos );
#else
        atk =
            (Attacks::RookRank[pos][allNoBlackQR.getRank(pos / 8)]) |
            (Attacks::RookFile[pos][rotNoBlackQR.getRank(pos % 8)]) |
            bishopAttacks( pos );
#endif

#ifdef HAVE_MOBILITY
        x = bitCount( atk & blackEnemyOrEmpty ) - ExpQueenMob;

        mobOpening -= ValQueenMobOpening * x;
        mobEndgame -= ValQueenMobEndgame * x;
#endif

        // Attack
        if( atk & whiteKingDanger ) {
            blackAttack += QueenAtk;
        }

        if( atk & blackKingDanger ) {
            blackDefend += QueenAtk;
        }
    }

    bb = whiteQueens;

    while( bb.isNotZero() ) {
        pos = bitSearchAndReset( bb );
        PRINT(( "White queen at %s\n", sqname(pos) ));

        // Mobility
#ifndef HAVE_XRAY_QR
        atk = rookAttacks( pos ) | bishopAttacks( pos );
#else
        atk =
            (Attacks::RookRank[pos][allNoWhiteQR.getRank(pos / 8)]) |
            (Attacks::RookFile[pos][rotNoWhiteQR.getRank(pos % 8)]) |
            bishopAttacks( pos );
#endif

#ifdef HAVE_MOBILITY
        x = bitCount( atk & whiteEnemyOrEmpty ) - ExpQueenMob;

        mobOpening += ValQueenMobOpening * x;
        mobEndgame += ValQueenMobEndgame * x;
#endif

        // Attack
        if( atk & blackKingDanger ) {
            whiteAttack += QueenAtk;
        }

        if( atk & whiteKingDanger ) {
            whiteDefend += QueenAtk;
        }

    }

    //--------------------------------------------------
    //
    // King
    //
    //--------------------------------------------------
    int blackKingDefects = 0;
    int whiteKingDefects = 0;

    if( RankOfSquare( blackKingSquare ) == 7 ) {
        blackKingDefects = evaluateKingShield( blackKingSquare, Black );
    }
    else {
        blackKingDefects = imin( 7, 3*(7 - RankOfSquare( blackKingSquare )) );
    }

    if( RankOfSquare( whiteKingSquare ) == 0 ) {
        whiteKingDefects = evaluateKingShield( whiteKingSquare, White );
    }
    else {
        whiteKingDefects = imin( 7, 3*RankOfSquare( whiteKingSquare ) );
    }

    PRINT(( "King defects black=%d (%d), white=%d (%d)\n",
        blackKingDefects,
        KingDefectsPenalty[ blackKingDefects ],
        whiteKingDefects,
        KingDefectsPenalty[ whiteKingDefects ] ));

    assert( blackKingDefects <= 7 );
    assert( whiteKingDefects <= 7 );

    // TODO: not entirely correct... it may happen that the shield is completely
    // destroyed and gets a serious penalty, then the king will just move one step
    // towards the center in order to get the "secondary" penalty, which is smaller!
    posOpening += KingDefectsPenalty[ blackKingDefects ];
    posOpening -= KingDefectsPenalty[ whiteKingDefects ];

    int blackAttackScore = 0;
    int whiteAttackScore = 0;

    if( distance( blackKingSquare, whiteKingSquare ) == 2 ) {
        // blackAttack += KingAtk;
        // whiteAttack += KingAtk;
   }

    if( blackAttack >= 0x01000000 ) {
        PRINT(( "Black attack = %x\n", blackAttack ));

        blackAttackScore = ((blackAttack & 0xFFFF) * AttackStrength[blackAttack >> 24]) / 256;

        if( blackAttack <= whiteDefend ) {
            blackAttackScore >>= 1;
        }

        if( whiteKingDefects >= 4 ) {
            PRINT(( "White king not shielded... black attack increased\n" ));
            blackAttackScore <<= 1;
        }
    }

    if( whiteAttack >= 0x01000000 ) {
        whiteAttackScore = ((whiteAttack & 0xFFFF) * AttackStrength[whiteAttack >> 24]) / 256;

        if( whiteAttack <= blackDefend ) {
            whiteAttackScore >>= 1;
        }

        if( blackKingDefects >= 4 ) {
            PRINT(( "Black king not shielded... white attack increased\n" ));
            whiteAttackScore <<= 1;
        }
    }

    //--------------------------------------------------
    //
    // Conclusions...
    //
    //--------------------------------------------------
    PRINT(( "Current = %d\n", result ));
    PRINT(( "Material = %d\n", materialScore ));
    PRINT(( "King attack for black = %d\n", blackAttackScore ));
    PRINT(( "King attack for white = %d\n", whiteAttackScore ));
    PRINT(( "Piece/square = %d / %d\n", pstScoreOpening, pstScoreEndgame ));
    PRINT(( "Mobility = %d / %d\n", mobOpening, mobEndgame ));
    PRINT(( "Pawn = %d / %d\n", pawnOpening, pawnEndgame ));
    PRINT(( "Positional = %d / %d + %d\n", posOpening, posEndgame, positionalScore ));

    result += materialScore + whiteAttackScore - blackAttackScore;

    PRINT(( "Interpolation = %d -> %d : %d\n",
        ( pstScoreOpening + mobOpening + pawnOpening + posOpening + positionalScore ),
        ( pstScoreEndgame + mobEndgame + pawnEndgame + posEndgame + positionalScore ),

        (( pstScoreOpening + mobOpening + pawnOpening + posOpening + positionalScore )*(stage) +
         ( pstScoreEndgame + mobEndgame + pawnEndgame + posEndgame + positionalScore )*(2*Stage_Max-stage)) / (2*Stage_Max) ));

    result += (
        ( pstScoreOpening + mobOpening + pawnOpening + posOpening + positionalScore )*(stage) +
        ( pstScoreEndgame + mobEndgame + pawnEndgame + posEndgame + positionalScore )*(2*Stage_Max-stage)) / (2*Stage_Max);

    PRINT(( "Result = %d (stage = %d)\n", result, stage ));

    // Start to decrease score if getting closer to draw by the 50-moves rule
    /*
    if( getHalfMoveClock() >= 75 ) {
        result = result * (100 - getHalfMoveClock()) / 25;
    }
    */

    // If a side thinks it's winning but it's not, adjust the score accordingly
    if( result > 0 && ! whiteCanWin ) {
        result = -5;
    }

    if( result < 0 && ! blackCanWin ) {
        result = +5;
    }

    // Roundup the score to help MTD(f) converge faster
    if( scoreQuantizationEnabled ) {
        if( result < 0 ) {
            result = -((-result) & ~3); // Force rounding towards zero, rather than minus infinity
        }
        else {
            result &= ~3;
        }
    }

    ev_item->code = hashCode.toUnsigned();
    ev_item->eval = result;

    return result;
}

int Position::evaluateDevelopment() const
{
    int result = 0;

    // Make sure the center pawns are not blocked
    if( ((whitePawns & Mask::D2_E2) << 8) & allPieces ) {
        result -= Score::BlockedCenterPawn;
    }

    if( ((blackPawns & Mask::D7_E7) >> 8) & allPieces ) {
        result += Score::BlockedCenterPawn;
    }

    return result;
}

int Position::evaluateKingShield( int pos, int side ) const
{
    int defects = 0;

    int krank = RankOfSquare(pos);
    int kfile = FileOfSquare(pos);

    if( side == Black ) {
        assert( krank == 7 );

        if( ! blackPawns.getBit(pos -  8) ) {
            defects++;
            if( ! blackPawns.getBit(pos - 16) ) defects++;
        }

        if( kfile > 0 ) {
            if( ! blackPawns.getBit(pos -  9) ) {
                defects++;
                if( ! blackPawns.getBit(pos - 17) ) defects++;
            }
        }

        if( kfile < 7 ) {
            if( ! blackPawns.getBit(pos -  7) ) {
                defects++;
                if( ! blackPawns.getBit(pos - 15) ) defects++;
            }
        }
    }
    else {
        assert( krank == 0 );

        if( ! whitePawns.getBit(pos +  8) ) {
            defects++;
            if( ! whitePawns.getBit(pos + 16) ) defects++;
        }

        if( kfile > 0 ) {
            if( ! whitePawns.getBit(pos +  7) ) {
                defects++;
                if( ! whitePawns.getBit(pos + 15) ) defects++;
            }
        }

        if( kfile < 7 ) {
            if( ! whitePawns.getBit(pos +  9) ) {
                defects++;
                if( ! whitePawns.getBit(pos + 17) ) defects++;
            }
        }
    }

    return defects;
}

int Position::evaluatePatterns() const
{
    int result = 0;

    // White: trapped rook
    if( whiteQueensRooks.getBit(H1) && 
        whiteKingSquare >= F1 && whiteKingSquare <= G1 &&
        whitePawns.getBit(G2) &&
        (whitePawns.getBit(H2) || whitePawns.getBit(H3)) )
    {
        result -= TrappedRookPenalty;
    }

    if( whiteQueensRooks.getBit(A1) && 
        whiteKingSquare >= B1 && whiteKingSquare <= C1 &&
        whitePawns.getBit(B2) &&
        (whitePawns.getBit(A2) || whitePawns.getBit(A3)) )
    {
        result -= TrappedRookPenalty;
    }

    // White: trapped bishop
    if( whiteQueensBishops.getBit(A7) &&
        blackPawns.getBit(C7) &&
        blackPawns.getBit(B6) )
    {
        result -= TrappedBishopPenalty;
    }

    // Black: trapped rook
    if( blackQueensRooks.getBit(H8) && 
        blackKingSquare >= F8 && blackKingSquare <= G8 &&
        blackPawns.getBit(G7) &&
        (blackPawns.getBit(H7) || blackPawns.getBit(H6)) )
    {
        result += TrappedRookPenalty;
    }

    if( blackQueensRooks.getBit(A8) && 
        blackKingSquare >= B8 && blackKingSquare <= C8 &&
        blackPawns.getBit(B7) &&
        (blackPawns.getBit(A7) || blackPawns.getBit(A6)) )
    {
        result += TrappedRookPenalty;
    }

    // Black: trapped bishop
    if( blackQueensBishops.getBit(A2) &&
        blackPawns.getBit(C2) &&
        blackPawns.getBit(B3) )
    {
        result += TrappedBishopPenalty;
    }

    return result;
}

int Position::evaluatePassedPawns() const
{
    int result = 0;

    return 0;
}
