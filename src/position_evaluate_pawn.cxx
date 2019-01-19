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
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "counters.h"
#include "engine.h"
#include "log.h"
#include "mask.h"
#include "pawnhash.h"
#include "position.h"
#include "score.h"

//#define PRINT( s ) printf s
//#define PRINT( s ) Log::write s
#ifndef PRINT
#define PRINT( s )
#endif

#define SLIM_EVALUATION

static char * sqname( int n )
{
    static char buf[10];

    Board::getSquareName( buf, n );

    return buf;
}

// Pawns
const int BlackPawn_Max[ 64 ] = {
    -15, -5,  0,  0,  0,  0, -5,-15,
    -15, -5,  0,  0,  0,  0, -5,-15,
    -15, -5,  0,  5,  5,  0, -5,-15,
    -15, -5,  0, 15, 15,  0, -5,-15,
    -15, -5,  0, 25, 25,  0, -5,-15,
    -15, -5,  0, 15, 15,  0, -5,-15,
    -15, -5,  0,  5,  5,  0, -5,-15,
    -15, -5,  0,  5,  5,  0, -5,-15,
};

const int BlackPawn_Min[ 64 ] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
};

static int WhitePawn_Max[ 64 ];
static int WhitePawn_Min[ 64 ];

// Backward pawns
const int BackwardPawn_Max          =  5;
const int BackwardPawn_Min          = 15;
const int Blocked_BackwardPawn_Max  = 10; // 10;
const int Blocked_BackwardPawn_Min  = 20; // 30;
const int Blocked_BackwardPawn_Doubled_Max  = 0; // 15;
const int Blocked_BackwardPawn_Doubled_Min  = 0; // 30;

// Isolated pawns (score gets worse with more isolani)
const int IsolatedOnOpenFile    = 10;                    // Further penalty if isolated pawn is on a half-open file
const int IsolatedPawn_Max[ 4 ] = {  8, 11, 15, 15 };   // By isolani count (minus one)
const int IsolatedPawn_Min[ 4 ] = { 12, 15, 20, 20 };

// Doubled pawns: the penalty is applied once for each pawn in the couple,
// but from the third pawn the "tripled" pawn penalty is applied
const int DoubledPawn_Max[ 8 ]  = { 10,  7,  5, 12, 12,  5,  7, 10 };   // By file
const int DoubledPawn_Min[ 8 ]  = { 17, 15, 15, 20, 20, 15, 15, 17 };

// Tripled pawns: this is applied for each pawn on the same file starting
// from the third (for the first two, the doubled pawn penalty is used)
const int TripledPawn_Max[ 8 ]  = { 25, 30, 30, 35, 35, 30, 30, 25 };   // By file
const int TripledPawn_Min[ 8 ]  = { 30, 35, 40, 50, 50, 40, 35, 30 };

// Initialization
static bool initialize = true;

/*
    Evaluation of pawn structure includes:
    - isolated pawns;
    - doubled pawns;
    - backward pawns.
*/
PawnHashEntry * Position::evaluatePawnStructure() const
{
    PRINT(( "********** Pawn evaluation **********\n" ));

    // TODO: move initialization at program startup and avoid this check!
    if( initialize ) {
        initialize = false;

        Score::getSymmetricSquareValueTable( WhitePawn_Max, BlackPawn_Max );
        Score::getSymmetricSquareValueTable( WhitePawn_Min, BlackPawn_Min );
    }

    // Keep separate scores for max stage (opening) and min stage (ending)
    int blackScoreMax = 0;
    int blackScoreMin = 0;
    int whiteScoreMax = 0;
    int whiteScoreMin = 0;

    int sq;

    unsigned    pawnFlags1 = 0;
    unsigned    pawnFlags2 = 0;

    int blackIsolani    = 0;
    int whiteIsolani    = 0;

    // For double-pawn detection, we keep a 4-bit pawn counter for each file
    // (all the counters are packed in a single variable).
    unsigned    doublePawnCounter;

    int whiteSpace  = 0;
    int blackSpace  = 0;

    // Evaluate black pawns
    BitBoard bb = blackPawns;

    doublePawnCounter = 0;

    while( bb.isNotZero() ) {
        sq = bitSearchAndReset( bb );

        int zmax = blackScoreMax;
        int zmin = blackScoreMin;

        blackScoreMax += BlackPawn_Max[ sq ];
        blackScoreMin += BlackPawn_Min[ sq ];

        PRINT(( "Black pawn %s piece/square: %d..%d\n", 
            sqname(sq), BlackPawn_Max[ sq ], BlackPawn_Min[ sq ] ));

        int file = FileOfSquare( sq );
        int rank = RankOfSquare( sq );

        blackSpace += 7-rank;

        int fileIsOpen  = !(Mask::File[file] & whitePawns);

        // There's a black pawn on this file, so it's not half-open for black
        pawnFlags1 |= BlackOpenFileBase << file;

        if( Mask::BlackDoublePawn[sq] & blackPawns ) {
            // Doubled pawn
            unsigned f = 1 << file;

#ifndef SLIM_EVALUATION
            if( doublePawnCounter & (0x06 << (file << 2)) ) {
                PRINT(( "The black pawn %s is tripled or worse\n", sqname(sq) ));

                blackScoreMax -= TripledPawn_Max[ file ];
                blackScoreMin -= TripledPawn_Min[ file ];
            }
            else {
                PRINT(( "The black pawn %s is doubled\n", sqname(sq) ));

                blackScoreMax -= DoubledPawn_Max[ file ];
                blackScoreMin -= DoubledPawn_Min[ file ];

                doublePawnCounter += (0x01 << (file << 2));
            }
#else
            blackScoreMax -= 10;
            blackScoreMin -= 20;

            doublePawnCounter += (0x01 << (file << 2));
#endif
        }

        if( Mask::FilesAround[file] & blackPawns ) {
            // Not isolated, is it backward?
            if( fileIsOpen && (Mask::BlackBackwardPawn[sq] & blackPawns).isZero() ) {
                // Backward pawn
                if( rank == 6 && Attacks::BlackPawn[ sq-8 ] & whitePawns ) {
                    PRINT(( "The black pawn %s is backward and blocked by an enemy pawn!\n", sqname(sq) ));

                    blackScoreMax -= Blocked_BackwardPawn_Max;
                    blackScoreMin -= Blocked_BackwardPawn_Min;

                    if( Mask::BlackDoublePawn[sq] & blackPawns ) {
                        PRINT(( "...to make things worse it is doubled too!\n", sqname(sq) ));
                        blackScoreMax -= Blocked_BackwardPawn_Doubled_Max;
                        blackScoreMin -= Blocked_BackwardPawn_Doubled_Min;
                    }
                }
                else if( (rank == 6) && (Attacks::BlackPawn[ sq ] & blackPawns) ) {
                    // Can save itself by jumping two squares...
                    PRINT(( "The black pawn %s is semi-backward\n", sqname(sq) ));
                }
                else {
                    PRINT(( "The black pawn %s is backward on a open file\n", sqname(sq) ));

                    blackScoreMax -= BackwardPawn_Max;
                    blackScoreMin -= BackwardPawn_Min;
                }
            }
        }
        else {
            // Isolated pawn
            if( (doublePawnCounter & (0x07 << (file << 2))) == 0 ) {
                PRINT(( "The black pawn %s is isolated (isolani=%d)\n", sqname(sq), blackIsolani ));

                blackScoreMax -= IsolatedPawn_Max[blackIsolani];
                blackScoreMin -= IsolatedPawn_Min[blackIsolani];

                if( fileIsOpen ) {
                    // Isolated on open file, penalize it more
                    PRINT(( "...and placed on a open file\n" ));

                    blackScoreMax -= IsolatedOnOpenFile;
                }

                blackIsolani++;
            }
            else {
                PRINT(( "The black pawn %s is isolated and part of a column\n", sqname(sq) ));

                blackScoreMax -= IsolatedPawn_Max[0] / 2;
                blackScoreMin -= IsolatedPawn_Min[0] / 2;
            }
        }

        PRINT(( "Black pawn %s score: %d..%d\n", sqname(sq), blackScoreMax-zmax, blackScoreMin-zmin ));
    }

    PRINT(( "\n" ));

    // Evaluate white pawns
    bb = whitePawns;

    doublePawnCounter = 0;

    while( bb.isNotZero() ) {
        sq = bitSearchAndReset( bb );

        int zmax = whiteScoreMax;
        int zmin = whiteScoreMin;

        whiteScoreMax += WhitePawn_Max[ sq ];
        whiteScoreMin += WhitePawn_Min[ sq ];

        PRINT(( "White pawn %s piece/square: %d..%d\n", 
            sqname(sq), WhitePawn_Max[ sq ], WhitePawn_Min[ sq ] ));

        int file = FileOfSquare( sq );
        int rank = RankOfSquare( sq );

        whiteSpace += rank;

        int fileIsOpen  = ! (Mask::File[file] & blackPawns);

        // There's a white pawn on this file, so it's not half-open for white
        pawnFlags1 |= WhiteOpenFileBase << file;

        if( Mask::WhiteDoublePawn[sq] & whitePawns ) {
            // Doubled pawn
#ifndef SLIM_EVALUATION
            if( doublePawnCounter & (0x06 << (file << 2)) ) {
                PRINT(( "The white pawn %s is tripled or worse\n", sqname(sq) ));

                whiteScoreMax -= TripledPawn_Max[ file ];
                whiteScoreMin -= TripledPawn_Min[ file ];
            }
            else {
                PRINT(( "The white pawn %s is doubled\n", sqname(sq) ));

                whiteScoreMax -= DoubledPawn_Max[ file ];
                whiteScoreMin -= DoubledPawn_Min[ file ];

                doublePawnCounter += 0x01 << (file << 2);
            }
#else
            whiteScoreMax -= 10;
            whiteScoreMin -= 20;

            doublePawnCounter += 0x01 << (file << 2);
#endif
        }

        if( Mask::FilesAround[file] & whitePawns ) {
            // Not isolated, is it backward?
            if( fileIsOpen && (Mask::WhiteBackwardPawn[sq] & whitePawns).isZero() ) {
                // Backward pawn
                if( rank == 1 && Attacks::WhitePawn[ sq+8 ] & blackPawns ) {
                    PRINT(( "The white pawn %s is backward and blocked by an enemy pawn!\n", sqname(sq) ));

                    whiteScoreMax -= Blocked_BackwardPawn_Max;
                    whiteScoreMin -= Blocked_BackwardPawn_Min;

                    if( Mask::WhiteDoublePawn[sq] & whitePawns ) {
                        PRINT(( "...to make things worse it is doubled too!\n", sqname(sq) ));
                        whiteScoreMax -= Blocked_BackwardPawn_Doubled_Max;
                        whiteScoreMin -= Blocked_BackwardPawn_Doubled_Min;
                    }
                }
                else if( (rank == 1) && (Attacks::WhitePawn[ sq ] & whitePawns) ) {
                    // Can save itself by jumping two squares...
                    PRINT(( "The white pawn %s is semi-backward\n", sqname(sq) ));
                }
                else {
                    PRINT(( "The white pawn %s is backward on a open file\n", sqname(sq) ));

                    whiteScoreMax -= BackwardPawn_Max;
                    whiteScoreMin -= BackwardPawn_Min;
                }
            }
        }
        else {
            // Isolated pawn
            if( (doublePawnCounter & (0x07 << (file << 2))) == 0 ) {
                PRINT(( "The white pawn %s is isolated (isolani=%d)\n", sqname(sq), whiteIsolani ));

                whiteScoreMax -= IsolatedPawn_Max[whiteIsolani];
                whiteScoreMin -= IsolatedPawn_Min[whiteIsolani];

                if( fileIsOpen ) {
                    // Isolated on open file, penalize it more
                    PRINT(( "...and placed on a open file\n" ));

                    whiteScoreMax -= IsolatedOnOpenFile;
                }

                whiteIsolani++;
            }
            else {
                PRINT(( "The white pawn %s is isolated and part of a column\n", sqname(sq) ));

                whiteScoreMax -= IsolatedPawn_Max[0] / 2;
                whiteScoreMin -= IsolatedPawn_Min[0] / 2;
            }
        }

        PRINT(( "White pawn %s score: %d..%d\n", sqname(sq), whiteScoreMax-zmax, whiteScoreMin-zmin ));
    }

    PRINT(( "Black pawn score: max=%d, min=%d\n", blackScoreMax, blackScoreMin ));
    PRINT(( "White pawn score: max=%d, min=%d\n", whiteScoreMax, whiteScoreMin ));

    // Flag some features of the pawn structure that might be useful later
    if( (whitePawns & Mask::SweetCenterLiteSquares) == Mask::SweetCenterLiteSquares ) {
        PRINT(( "White's lite bishop is bad\n" ));
        pawnFlags2 |= WhiteBadLiteBishop;
    }

    if( (whitePawns & Mask::SweetCenterDarkSquares) == Mask::SweetCenterDarkSquares ) {
        PRINT(( "White's dark bishop is bad\n" ));
        pawnFlags2 |= WhiteBadDarkBishop;
    }

    if( (blackPawns & Mask::SweetCenterLiteSquares) == Mask::SweetCenterLiteSquares ) {
        PRINT(( "Black's lite bishop is bad\n" ));
        pawnFlags2 |= BlackBadLiteBishop;
    }

    if( (blackPawns & Mask::SweetCenterDarkSquares) == Mask::SweetCenterDarkSquares ) {
        PRINT(( "Black's dark bishop is bad\n" ));
        pawnFlags2 |= BlackBadDarkBishop;
    }

    if( numOfWhitePawns() >= 4 && numOfBlackPawns() >= 4 ) {
        whiteSpace <<= 4;
        whiteSpace /= numOfWhitePawns();
        blackSpace <<= 4;
        blackSpace /= numOfBlackPawns();

        PRINT(( "Space: %d\n", whiteSpace - blackSpace ));

        // whiteScoreMax += whiteSpace << 1;
        // blackScoreMax += blackSpace << 1;
    }

    // Return evaluation info
    PawnHashEntry * entry = Engine::getPawnHashTable()->store( *this,
        blackScoreMax,
        blackScoreMin,
        whiteScoreMax,
        whiteScoreMin,
        pawnFlags1,
        pawnFlags2 );

    return entry;
}

/*
    This function has been removed because I can't make it work.

    Some critical positions:

    8/p7/8/PP2k2p/8/8/8/K7 w - - 0 1
    ...here white doesn't even have a passed pawn, and is out of black's passed square,
    yet it easily wins with b6!

    8/8/2k5/6P1/8/4p2K/8/8 w - - 0 1
    ...white enters into the square with Kf3, but then also does black with Kd5 so a draw

    8/k/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1
    ...a famous endgame, Rb1! wins for white
    
    And positions where it would be nice to see trouble coming:

    8/8/p5pk/q7/4pP2/5Q1K/6P1/8 w - - 0 67
    ...after Qxe4 then Qf5! wins because the "a" pawn is unstoppable
*/
int Position::evaluatePawnRaces( const BitBoard & blackPassed, const BitBoard & whitePassed ) const
{
    return 0;
}
