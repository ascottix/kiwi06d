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
#include <string.h>

#include "attacks.h"
#include "bitbase.h"
#include "metrics.h"
#include "log.h"
#include "packed_array.h"
#include "recognizer.h"
#include "score.h"

#ifdef HAVE_RECOGNIZER_STATS
Recognizer::Stats   Recognizer::stats_[ 1 << SignatureBits ];       
#endif

unsigned            Recognizer::knownHandlersSignature_;
unsigned char       Recognizer::knownHandlers_[ (1 << SignatureBits) / 8 ];
RecognizerHandler   Recognizer::handler_[ 1 << SignatureBits ];

static bool recognizerForKPK_White( const Position & pos, RecognizerInfo & result ); // KPK
static bool recognizerForKPK_Black( const Position & pos, RecognizerInfo & result ); // KPK

static bool recognizerForKBPK_White( const Position & pos, RecognizerInfo & result ); // KBPK
static bool recognizerForKBPK_Black( const Position & pos, RecognizerInfo & result ); // KBPK

static bool recognizerForKNK_White( const Position & pos, RecognizerInfo & result ); // KN*K
static bool recognizerForKNK_Black( const Position & pos, RecognizerInfo & result ); // KN*K

static bool recognizerForKBK_White( const Position & pos, RecognizerInfo & result ); // KB*K
static bool recognizerForKBK_Black( const Position & pos, RecognizerInfo & result ); // KB*K

static bool recognizerForKBNK_White( const Position & pos, RecognizerInfo & result ); // KBNK
static bool recognizerForKBNK_Black( const Position & pos, RecognizerInfo & result ); // KBNK

void Recognizer::initialize()
{
    unsigned i;

    knownHandlersSignature_ = 0;

    for( i=0; i<sizeof(knownHandlers_); i++ ) {
        knownHandlers_[i] = 0;
    }

    for( i=0; i<(1 << SignatureBits); i++ ) {
        handler_[i] = 0;
    }

    clearStats();

    // Register known handlers
    registerHandler( SignatureWhitePawn, recognizerForKPK_White );
    registerHandler( SignatureBlackPawn, recognizerForKPK_Black );

    registerHandler( SignatureWhitePawn | SignatureWhiteBishop, recognizerForKBPK_White );
    registerHandler( SignatureBlackPawn | SignatureBlackBishop, recognizerForKBPK_Black );

    registerHandler( SignatureWhiteKnight, recognizerForKNK_White );
    registerHandler( SignatureBlackKnight, recognizerForKNK_Black );

    registerHandler( SignatureWhiteBishop, recognizerForKBK_White );
    registerHandler( SignatureBlackBishop, recognizerForKBK_Black );

    registerHandler( SignatureWhiteBishop | SignatureWhiteKnight, recognizerForKBNK_White );
    registerHandler( SignatureBlackBishop | SignatureBlackKnight, recognizerForKBNK_Black );

    // Load bitbases
    initBitBases();
    loadBitBases();
}

void Recognizer::registerHandler( unsigned signature, RecognizerHandler handler )
{
    signature >>= SignatureOffset;

    unsigned short_sig = (signature & 0x1F) | ((signature >> 5) & 0x1F);

    knownHandlersSignature_ |= 1 << short_sig;

    knownHandlers_[ signature >> 3 ] |= 1 << (signature & 7);

    handler_[ signature ] = handler;
}

bool Recognizer::clearStats()
{
#ifdef HAVE_RECOGNIZER_STATS
    for( int i=0; i<(1 << SignatureBits); i++ ) {
        stats_[i].probes_ = 0;
        stats_[i].hits_ = 0;
    }

    return true;
#else
    return false;
#endif
}

bool Recognizer::dumpStats()
{
#ifdef HAVE_RECOGNIZER_STATS
    bool dumpHeader = true;

    for( int i=0; i<(1 << SignatureBits); i++ ) {
        if( stats_[i].probes_ > 0 ) {
            if( dumpHeader ) {
                dumpHeader = false;
                Log::write( "Recognizer statistics\n" );
                Log::write( "---------------------\n" );
            }

            // Print the recognizer name...
            unsigned sign = (unsigned) i << SignatureOffset;
            char     name[16];
            unsigned nlen = 0;

            if( sign & SignatureWhiteQueen ) name[ nlen++ ] = 'Q';
            if( sign & SignatureWhiteRook  ) name[ nlen++ ] = 'R';
            if( sign & SignatureWhiteBishop) name[ nlen++ ] = 'B';
            if( sign & SignatureWhiteKnight) name[ nlen++ ] = 'N';
            if( sign & SignatureWhitePawn  ) name[ nlen++ ] = 'P';

            name[ nlen++ ] = '.';

            name[ nlen++ ] = 'K';
            if( sign & SignatureBlackQueen ) name[ nlen++ ] = 'Q';
            if( sign & SignatureBlackRook  ) name[ nlen++ ] = 'R';
            if( sign & SignatureBlackBishop) name[ nlen++ ] = 'B';
            if( sign & SignatureBlackKnight) name[ nlen++ ] = 'N';
            if( sign & SignatureBlackPawn  ) name[ nlen++ ] = 'P';

            while( nlen < (sizeof(name)-1) ) {
                name[ nlen++ ] = ' ';
            }

            name[ nlen ] = '\0';

            Log::write( "K%s = %u / %u (%05.2f)\n", name, stats_[i].hits_, stats_[i].probes_, (stats_[i].hits_ * 100.0) / stats_[i].probes_ );
        }
    }

    return true;
#else
    return false;
#endif
}

/*
    Common evaluation stuff.
*/
enum {
    // Evaluator return codes
    kpkDraw         = 0,    // At least draw (i.e. draw or better)
    kpkExactDraw,
    kpkWin,
    kpkUnknown,

    // Scores
    kpkWinScore         = 700,
    kppkWinScore        = 750,
    kbbkWinScore        = 800,
    kbnkWinScore        = 800,
};

/*
    KP vs. K evaluator.

    Note: all scores returned are from the white (i.e. winning) side.

    All info taken from "Il finale negli scacchi" by Enrico Paoli (Ed. Mursia).
*/
static int evaluatorForKPK( int wk, int wp, int bk, bool wtm )
{
    // Probe bitbase
    PackedArray * bb = getBitBase( bb_KPK, wtm );

    if( bb != 0 ) {
        mirror_wk( wk, bk, wp );

        return bb->get( getBitBaseIndex(bb_KPK,wk,bk,wp) ) != 0 ? kpkWin : kpkDraw;
    }

    /* 
        If full coverage is enabled, the recognizer will always report a score
        (falling to draw/lower bound if there's nothing better), otherwise it will
        only try to evaluate "known" positions.
        In the first case its success rate is about 90%, while in the latter it
        is almost 98% but it will be able to cover only about 83% of all positions.
    */
    const bool fullCoverage = false;

    int wkr = RankOfSquare(wk);
    int wpr = RankOfSquare(wp);
    int bkr = RankOfSquare(bk);

    // Mirror the position for pawn files E, F, G and H
    if( FileOfSquare(wp) >= 4 ) {
        wk = FileRankToSquare( 7-FileOfSquare(wk), wkr );
        wp = FileRankToSquare( 7-FileOfSquare(wp), wpr );
        bk = FileRankToSquare( 7-FileOfSquare(bk), bkr );
    }

    int wkf = FileOfSquare(wk);
    int wpf = FileOfSquare(wp);
    int bkf = FileOfSquare(bk);

    // Compute the distance of kings from the pawn
    int bk_distance = distance( wp, bk );
    int wk_distance = distance( wp, wk );

    if( wtm ) {
        wk_distance--;
    }
    else {
        bk_distance--;
    }

    // If black can capture the pawn right now, it's a draw
    if( ! wtm && bk_distance <= 0 && wk_distance > 1 ) {
        return kpkExactDraw;
    }
    
    // Pawn on 7th rank
    if( wpr == 6) {
        int wp_fl = wpf > 0 ? wp + 7 : 999;
        int wp_fr = wpf < 7 ? wp + 9 : 999;

        if( wtm ) {
            if( wpf != 0 && bk == (wp+8) && ((wpf > 0 && distance(wk,wp-9) == 1) || (wpf < 7 && distance(wk,wp-7) == 1)) ) {
                return kpkWin;
            }
            if( wpf > 0 && bk == (wp+1) && (distance(wk,wp-1) <= 1) ) {
                return kpkWin;
            }
            if( wpf < 7 && bk == (wp-1) && (distance(wk,wp+1) <= 1) ) {
                return kpkWin;
            }
        }
        else {
            if( wk == wp_fl || wk == wp_fr ) {
                return kpkWin;
            }
        }
    }

    // If black king is blocking the pawn file and is closer than the white king, it can capture
    // the pawn even if white is on move
    if( wtm && (bkf == wpf) && (bkr > wpr) && (wkr > bkr) ) {
        return kpkExactDraw;
    }

    // Check for stalemate (this covers only one case but is needed because it will
    // report a false win otherwise)
    if( bk == A8 && wp == B6 && (wk == C8 || wk == C7) ) {
        return wtm ? kpkWin : kpkExactDraw;
    }

    // Rook pawn needs some special attention before using general rules
    if( FileOfSquare(wp) == 0 ) {
        if( (FileOfSquare(bk) <= 1) && (bkr > wpr) ) {
            return kpkExactDraw;
        }

        // Check for a typical stalemate
        if( (wk == A8) && (wp == A7) ) {
            if( (bk == C8) || (bk == C7) || (! wtm && (Attacks::KingDistance[bk][C7] <= 1)) ) {
                return kpkExactDraw;
            }
        }
    }

    // Check for pawn advance from start square, good for some extra hits
    bool eff = (wkf >= (wpf-1)) && (wkf <= (wpf+1)) && (wkr >= (wpr + 1 + (wpr < 4 ? 1 : 0))) && (wpf != 0);
    
    // TODO!!!
    if( wtm && wpr == 1 && wk != (wp+8) && bk != (wp+8) && wk != (wp+16) && bk != (wp+16) ) {
        int wp1 = wp+16;
        int bk_d = distance( wp1, bk );
        int wk_d = distance( wp1, wk );

        if( bk_d >= wk_d && eff ) {
            return kpkWin;
        }
    }

    // White wins if black king is out of the pawn square...
    int wp_to_goal = 7 - wpr - (wpr == 1 ? 1 : 0);
    int bk_to_goal = distance( bk, FileRankToSquare( wpf, 7 ) ) - (wtm ? 0 : 1);

    if( bk_to_goal > wp_to_goal ) {
        return kpkWin;
    }

    // ...or if white king is placed between the pawn and the black king (not many hits here)...
    if( (bkf < wkf && wkf < wpf) || (bkf > wkf && wkf > wpf) ) {
        if( (((bkr == 7) || (bkr == 6 && wtm)) && (wkr >= 6)) ||
            (wtm && linear_distance(bkr,wkr) <= 1) || 
            ((wkr >= bkr) && (wpr >= (wkr-1))) ) 
        {
            return kpkWin;
        }
    }

    // ...but if black is in the square and closer to the pawn, it's a draw
    if( bk_distance < wk_distance ) {
        return kpkDraw;
    }

    // If white king is in the pawn absolute effective area, it's a win
    if( eff ) {
        return kpkWin;
    }

    // Check for "easy" cases where the white king can reach the pawn absolute effective
    // area before the black king (many hits here)
    if( wtm && wkr > wpr && (bk != A8 || wp != B6) ) {
        bool win = false;

        win |= (wkf < wpf) && (bkf > wpf) && (linear_distance( wkf, wpf-2 ) <= linear_distance( bkf, wpf+1 ));
        win |= (wkf > wpf) && (bkf < wpf) && (linear_distance( wkf, wpf+2 ) <= linear_distance( bkf, wpf-1 ));
        win |= (wkf < wpf) && (bkf < wkf);
        win |= (wkf > wpf) && (bkf > wkf) && ((wpf != 0) || ((7 - wpr) <= bkf));

        if( win ) {
            return kpkWin;
        }
    }

    // If white king is in a zone "relatively" effective, it needs opposition to win
    int eff_rank = wpr + (wpr < 4 ? 1 : 0);

    if( ! wtm && (wpr != 4) && (wkf >= (wpf-1)) && (wkf <= (wpf+1)) && (wkr >= eff_rank) && (wpf != 0) ) {
        int df = linear_distance( wkf, bkf );
        int dr = linear_distance( wkr, bkr );

        if( (df == 0 || df == 2) && (dr == 0 || dr == 2 || dr == 4) ) {
            return kpkWin;
        }
    }

    return fullCoverage ? kpkDraw : kpkUnknown;
}

static bool evaluatorForKPK( int wk, int wp, int bk, bool wtm, RecognizerInfo & score )
{
    int r = evaluatorForKPK( wk, wp, bk, wtm );

    switch( r ) {
    case kpkDraw:
        score.set( 0, rtLowerBound );
        break;
    case kpkExactDraw:
        score.set( 0, rtExact );
        break;
    case kpkWin:
        score.set( kpkWinScore - 12*(7 - RankOfSquare(wp)) - 4*distance(wk,wp) + 2*linear_distance(FileOfSquare(bk),FileOfSquare(wp)), rtLowerBound );
        break;
    }

    return r != kpkUnknown;
}

static bool evaluatorForKPPK( int wk, int bk, BitBoard wps, int wtm, int mirror, RecognizerInfo & score )
{
    bool handled = false;

    PackedArray * bb = getBitBase( bb_KPPK, wtm );

    if( bb != 0 ) {
        int p1 = bitSearchAndReset( wps );
        int p2 = bitSearch( wps );

        if( mirror ) {
            wk = flip_rank( wk );
            bk = flip_rank( bk );
            p1 = flip_rank( p1 );
            p2 = flip_rank( p2 );
        }

        mirror_wk( wk, bk, p1, p2 );

        if( bb->get( getBitBaseIndex(bb_KPPK,wk,bk,p1,p2) ) != 0 ) {
            score.set( kppkWinScore + 20*imax(p1-1,p2-1) - 12*edge_distance(bk) - 6*distance(wk,bk), rtLowerBound );
        }
        else {
            score.set( 0, rtExact );
        }

        handled = true;
    }

    return handled;
}

static bool recognizerForKPK_White( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfWhitePawns() == 1 ) {
        int pawn = pos.whitePawns.bitScanForward();

        handled = evaluatorForKPK( pos.whiteKingSquare, pawn, pos.blackKingSquare, pos.whiteToMove(), result );
    }
    else if( pos.numOfWhitePawns() == 2 ) {
        // Use bitboard
        handled = evaluatorForKPPK( pos.whiteKingSquare, pos.blackKingSquare, pos.whitePawns, pos.whiteToMove(), 0, result );
    }

    if( pos.blackToMove() ) {
        result.negate();
    }

    return handled;
}

static bool evaluatorForKBPK( int wk, int bk, int wb, int wp, bool wtm, RecognizerInfo & result )
{
    bool handled = false;

    // Probe bitbase
    PackedArray * bb = getBitBase( bb_KBPK, wtm );

    if( bb != 0 ) {
        mirror_wk( wk, bk, wb, wp );

        if( bb->get( getBitBaseIndex(bb_KBPK,wk,bk,wb,wp) ) != 0 ) {
            // It's a win
            result.set( kpkWinScore - 12*(7 - RankOfSquare(wp)) - 4*distance(wk,wp) + 2*linear_distance(FileOfSquare(bk),FileOfSquare(wp)), rtLowerBound );
        }
        else {
            // Draw
            result.set( 0, rtExact );
        }

        handled = true;
    }

    return handled;
}

static bool recognizerForKBPK_White( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfWhiteBishops() == 1 && pos.numOfWhitePawns() == 1 ) {
        int wb = pos.whiteQueensBishops.bitScanForward();
        int wp = pos.whitePawns.bitScanForward();

        handled = evaluatorForKBPK( pos.whiteKingSquare, pos.blackKingSquare, wb, wp, pos.whiteToMove(), result );

        if( pos.blackToMove() ) {
            result.negate();
        }
    }

    return handled;
}

static bool recognizerForKBPK_Black( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfBlackBishops() == 1 && pos.numOfBlackPawns() == 1 ) {
        int bb = pos.blackQueensBishops.bitScanForward();
        int bp = pos.blackPawns.bitScanForward();

        handled = evaluatorForKBPK( 
            flip_rank(pos.blackKingSquare), 
            flip_rank(pos.whiteKingSquare), 
            flip_rank(bb), 
            flip_rank(bp), 
            pos.blackToMove(), 
            result );

        if( pos.whiteToMove() ) {
            result.negate();
        }
    }

    return handled;
}

static bool recognizerForKPK_Black( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfBlackPawns() == 1 ) {
        // Note that recognizer only works for white, so we need to mirror the board here
        int pawn = pos.blackPawns.bitScanForward();

        handled = evaluatorForKPK( flip_rank(pos.blackKingSquare), flip_rank(pawn), flip_rank(pos.whiteKingSquare), pos.blackToMove(), result );
    }
    else if( pos.numOfBlackPawns() == 2 ) {
        handled = evaluatorForKPPK( pos.blackKingSquare, pos.whiteKingSquare, pos.blackPawns, pos.whiteToMove(), 1, result );
    }

    if( pos.whiteToMove() ) {
        result.negate();
    }

    return handled;
}

static void blackKingToA1D1( int & bk, int & wk, int & w1, int & w2 )
{
    // If the black king is on the edge, bring it to the first rank
    if( FileOfSquare( bk ) == 0 ) {
        bk = flip_a1h8( bk );
        wk = flip_a1h8( wk );
        w1 = flip_a1h8( w1 );
        w2 = flip_a1h8( w2 );
    }
    else if( FileOfSquare( bk ) == 7 ) {
        bk = flip_a8h1( bk );
        wk = flip_a8h1( wk );
        w1 = flip_a8h1( w1 );
        w2 = flip_a8h1( w2 );
    }
    else if( RankOfSquare( bk ) == 7 ) {
        bk = flip_rank( bk );
        wk = flip_rank( wk );
        w1 = flip_rank( w1 );
        w2 = flip_rank( w2 );
    }

    if( RankOfSquare( bk ) == 0 ) {
        // Bring black king in the A1..D1 range
        if( bk >= E1 ) {
            bk = flip_file( bk );
            wk = flip_file( wk );
            w1 = flip_file( w1 );
            w2 = flip_file( w2 );
        }

        if( bk == A1 ) {
            // Bring white king in the A1..D1..D4 octant
            if( RankOfSquare(wk) > FileOfSquare(wk) ) {
                bk = flip_a1h8( bk );
                wk = flip_a1h8( wk );
                w1 = flip_a1h8( w1 );
                w2 = flip_a1h8( w2 );
            }
        }
    }
}

/*
    A perfect evaluator for KNNK... probably quite useless but very fun to program!

    Note: there are more than 25 million positions and they are all draw except for
    1232 wins (mate in one, which can't usually be forced) and 240 checkmates.

    All the non-draw positions have been manually coded (hey, I told you it was fun!)
    and are correctly recognized.
*/
static bool evaluatorForKNNK( int wk, int bk, BitBoard wn, bool wtm, RecognizerInfo & result )
{
    int n1 = bitScanAndResetForward(wn);
    int n2 = bitScanAndResetForward(wn);

    // Assume result is draw... only very few positions are not so!
    result.set( 0, rtExact );

    // If black king is on the edge it could be mated or there could be a mate in one...
    if( is_edge_square( bk ) ) {
        // First, bring it into the A1..D1 range
        blackKingToA1D1( bk, wk, n1, n2 );
    
        if( bk != B1 ) {
            if( bk == C1 && ((FileOfSquare(n1) == 7) || (FileOfSquare(n2) == 7)) ) {
                // This is just to skip a case where the "relative" flip below
                // (which isn't always a safe operation) produces a couple of false positives
            }
            else if( bk >= C1 && (FileOfSquare(wk) < FileOfSquare(bk)) ) {
                // Bring white king to the right of the black king
                wk = flip_file_relative( wk, bk );
                n1 = flip_file_relative( n1, bk );
                n2 = flip_file_relative( n2, bk );
            }

            // Make sure knights are in a known order
            if( n1 > n2 ) {
                iswap( n1, n2 );
            }

            if( ! wtm ) {
                // Black could be mated...
                bool mated = false;

                if( bk == A1 ) {
                    bool nb3b4 = (n1 == B3) && (n2 == B4);
                    bool nb3c3 = (n1 == B3) && (n2 == C3);

                    mated = ((wk == C2) && (nb3b4 || nb3c3 || ((n1 == C1) && (n2 == B3)))) ||
                            ((wk == C1) && (nb3b4 || nb3c3 || ((n1 == C2) && (n2 == C3 || n2 == B4))));
                }
                else {
                    mated = ((bk == C1) && (wk == E1 || wk == E2) && (n1 == A3 && n2 == D3)) ||
                            ((bk == D1) && (wk == F1 || wk == F2) && (n1 == B3 && n2 == E3));
                }

                if( mated ) {
                    result.set( -Score::Mate, rtExact );
                }
            }
            else {
                // There could be a mate in one for white...
                bool win = false;

                if( bk == A1 ) {
                    bool n2_c3b4 = (n2 == C3) || (n2 == B4);

                    bool nn_both = ((n1 == D2) && n2_c3b4) ||
                                   ((n1 == C3) && (n2 == A5 || n2 == C5 || n2 == D4)) ||
                                   ((n1 == B4) && (n2 == D4 || n2 == A5 || n2 == C5));

                    if( wk == C1 ) {
                        win = nn_both || ((n1 == A3 || n1 == E1) && n2_c3b4) || ((n1 == C3) && (n2 == E3 || n2 == B4)) || (n1 == E3 && n2 == B4);
                    }
                    else if( wk == C2 ) {
                        win = nn_both || ((n1 == C1) && (n2 == D2 || n2_c3b4 || n2 == D4 || n2 == A5 || n2 == C5));
                    }
                }
                else if( bk == C1 ) {
                    bool nn_both = ((n1 == B2 || n1 == F2) && (n2 == A3)) ||
                                   ((n1 == A3) && (n2 == B4 || n2 == C5 || n2 == E5 || n2 == F4));

                    win = ((wk == E1) && nn_both) || ((wk == E2) && (nn_both || (n1 == E1 && n2 == A3)));
                }
                else if( bk == D1 ) {
                    bool nn_both = ((n1 == C2 || n1 == G2) && (n2 == B3)) ||
                        ((n1 == B3) && (n2 == C4 || n2 == D5 || n2 == F5 || n2 == G4));

                    win = ((wk == F1) && nn_both) || ((wk == F2) && (nn_both || (n1 == F1 && n2 == B3)));
                }

                if( win ) {
                    result.set( Score::Mate - 2, rtExact );
                }
            }
        } // ...black king not in B1
    }

    return true;
}

static bool recognizerForKNK_White( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    switch( pos.numOfWhiteKnights() ) {
    case 1:
        result.set( 0, rtExact );
        handled = true;
        break;
    case 2:
        handled = evaluatorForKNNK( pos.whiteKingSquare, pos.blackKingSquare, pos.whiteKnights, pos.whiteToMove(), result );
        break;
    default:
        // Three or more knights
        break;
    }

    return handled;
}

static bool recognizerForKNK_Black( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    switch( pos.numOfBlackKnights() ) {
    case 1:
        result.set( 0, rtExact );
        handled = true;
        break;
    case 2:
        handled = evaluatorForKNNK( pos.blackKingSquare, pos.whiteKingSquare, pos.blackKnights, pos.blackToMove(), result );
        break;
    default:
        // Three or more knights
        break;
    }

    return handled;
}

/*
    A near-perfect recognizer for KBBK.
*/
static int getScoreForKBBK( int wk, int bk )
{
    int result = kbbkWinScore;

    int edge_distance = imin( 
        imin(FileOfSquare(bk),7-FileOfSquare(bk)), 
        imin(RankOfSquare(bk),7-RankOfSquare(bk)) );

    result -= 16*edge_distance + 8*distance(bk,wk);

    if( edge_distance == 0 ) {
        result += 100 - 16*imin( 
            imin(distance(bk,A1),distance(bk,A8)), 
            imin(distance(bk,H1),distance(bk,H8)) );
    }

    return result;
}

static bool evaluatorForKBBK( int wk, int bk, BitBoard bb, bool wtm, RecognizerInfo & result )
{
    int b1 = bitScanAndResetForward(bb);
    int b2 = bitScanAndResetForward(bb);

    result.set( 0, rtExact );

    if( color_of(b1) != color_of(b2) ) {
        // Different color bishops
        BitBoard d_b2g8( MK_U64(0x4020100804020000) );

        if( wtm ) {
            bool draw = false;

            if( is_edge_square( bk ) ) {
                blackKingToA1D1( bk, wk, b1, b2 );

                if( bk == A1 ) {
                    draw = (wk == C1 || wk == C2) && (b1 == A2 || b2 == A2);
                }
                else if( bk == B1 ) {
                    draw = (wk == D1 || wk == D2 || wk == C3) && (b1 == A1 || b2 == A1) && (d_b2g8.getBit(b1) || d_b2g8.getBit(b2));
                }
            }

            if( ! draw ) {
                result.set( +getScoreForKBBK(wk,bk), rtLowerBound );
            }
        }
        else {
            bool draw = false;

            if( distance(bk,b1) <= 1 && distance(wk,b1) > 1 ) {
                draw = true;
            }

            if( distance(bk,b2) <= 1 && distance(wk,b2) > 1 ) {
                draw = true;
            }

            if( ! draw && is_edge_square( bk ) && distance(bk,wk) == 2 ) {
                // We could have a stalemate here, so we need to look better
                blackKingToA1D1( bk, wk, b1, b2 );

                // Compute the squares attacked by white (slow, but not done very often...)
                BitBoard w_atk( 0 );

                int b;

                for( b = b1; (b!=wk)&&(FileOfSquare(b)>0)&&(RankOfSquare(b)>0); b -= 9 ) w_atk.setBit( b-9 );
                for( b = b1; (b!=wk)&&(FileOfSquare(b)<7)&&(RankOfSquare(b)>0); b -= 7 ) w_atk.setBit( b-7 );
                for( b = b1; (b!=wk)&&(FileOfSquare(b)>0)&&(RankOfSquare(b)<7); b += 7 ) w_atk.setBit( b+7 );
                for( b = b1; (b!=wk)&&(FileOfSquare(b)<7)&&(RankOfSquare(b)<7); b += 9 ) w_atk.setBit( b+9 );
                
                for( b = b2; (b!=wk)&&(FileOfSquare(b)>0)&&(RankOfSquare(b)>0); b -= 9 ) w_atk.setBit( b-9 );
                for( b = b2; (b!=wk)&&(FileOfSquare(b)<7)&&(RankOfSquare(b)>0); b -= 7 ) w_atk.setBit( b-7 );
                for( b = b2; (b!=wk)&&(FileOfSquare(b)>0)&&(RankOfSquare(b)<7); b += 7 ) w_atk.setBit( b+7 );
                for( b = b2; (b!=wk)&&(FileOfSquare(b)<7)&&(RankOfSquare(b)<7); b += 9 ) w_atk.setBit( b+9 );

                w_atk |= Attacks::King[wk];

                int stale  = w_atk.getBit( bk+8 ) & w_atk.getBit( bk+9 ) & w_atk.getBit( bk+1 );

                if( bk > A1 ) {
                    stale &= w_atk.getBit( bk+7 ) & w_atk.getBit( bk-1 );
                }

                if( stale && ! w_atk.getBit(bk) ) {
                    draw = true;
                }
                else if( bk == C1 && wk == C3 && (b1 == A1 || b2 == A1) && (d_b2g8.getBit(b1) || d_b2g8.getBit(b2)) ) {
                    draw = true;
                }
            }

            if( ! draw ) {
                result.set( -getScoreForKBBK(wk,bk), rtUpperBound );
            }
        }
    }

    return true;
}

static bool recognizerForKBK_White( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    switch( pos.numOfWhiteBishops() ) {
    case 1:
        result.set( 0, rtExact );
        handled = true;
        break;
    case 2:
        handled = evaluatorForKBBK( pos.whiteKingSquare, pos.blackKingSquare, pos.whiteQueensBishops, pos.whiteToMove(), result );
        break;
    default:
        break;
    }

    return handled;
}

static bool recognizerForKBK_Black( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    switch( pos.numOfBlackBishops() ) {
    case 1:
        result.set( 0, rtExact );
        handled = true;
        break;
    case 2:
        handled = evaluatorForKBBK( pos.blackKingSquare, pos.whiteKingSquare, pos.blackQueensBishops, pos.blackToMove(), result );
        break;
    default:
        break;
    }

    return handled;
}

static bool evaluatorForKBNK( int wk, int bk, int wb, int wn, bool wtm, RecognizerInfo & result )
{
    bool handled = false;

    // Probe bitbase
    PackedArray * bb = getBitBase( bb_KBNK, wtm );

    if( bb != 0 ) {
        mirror_wk( wk, bk, wb, wn );

        if( bb->get( getBitBaseIndex(bb_KBNK,wk,bk,wb,wn) ) != 0 ) {
            // It's a win
            result.set( kbnkWinScore - 8*distance(wk,bk) - 16*b_corner_distance(bk,color_of(wb)), rtLowerBound );
        }
        else {
            // Draw
            result.set( 0, rtExact );
        }

        handled = true;
    }

    return handled;
}

static bool recognizerForKBNK_White( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfWhiteBishops() == 1 && pos.numOfWhiteKnights() == 1 ) {
        int wb = pos.whiteQueensBishops.bitScanForward();
        int wn = pos.whiteKnights.bitScanForward();

        handled = evaluatorForKBNK( pos.whiteKingSquare, pos.blackKingSquare, wb, wn, pos.whiteToMove(), result );

        if( pos.blackToMove() ) {
            result.negate();
        }
    }

    return handled;
}

static bool recognizerForKBNK_Black( const Position & pos, RecognizerInfo & result )
{
    bool handled = false;

    if( pos.numOfBlackBishops() == 1 && pos.numOfBlackKnights() == 1 ) {
        int bb = pos.blackQueensBishops.bitScanForward();
        int bn = pos.blackKnights.bitScanForward();

        handled = evaluatorForKBNK( pos.blackKingSquare, pos.whiteKingSquare, bb, bn, pos.blackToMove(), result );

        if( pos.whiteToMove() ) {
            result.negate();
        }
    }

    return handled;
}
