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
#include "position_enum.h"
#include "recognizer.h"

#ifdef HAVE_EGTB

int EGTBProbe( const Position & pos, int * score );

/*
    Keeps useful info about the recognizer performance, 
    so we can compare it with EGTB.
*/
struct RecognizerStats
{
    int all_positions_;
    int recog_probe_hits_;
    int recog_hits_;
    int recog_bad_lower_bound_;
    int recog_bad_upper_bound_;
    int recog_bad_exact_;
    int recog_exact_;
    int recog_wins_;
    int recog_losses_;
    int egtb_failures_;
    int egtb_wins_;
    int egtb_losses_;

    RecognizerStats() {
        clear();
    }

    void clear() {
        all_positions_ = 0;
        recog_probe_hits_ = 0;
        recog_hits_ = 0;
        recog_bad_lower_bound_ = 0;
        recog_bad_upper_bound_ = 0;
        recog_bad_exact_ = 0;
        recog_exact_ = 0;
        recog_wins_ = 0;
        recog_losses_ = 0;
        egtb_failures_ = 0;
        egtb_wins_ = 0;
        egtb_losses_ = 0;
    }

    void dump( const char * header = 0 );
};

void RecognizerStats::dump( const char * header )
{
    if( header != 0 ) {
        printf( "%s\n", header );

        while( *header++ ) {
            printf( "-" );
        }

        printf( "\n" );
    }

    printf( "Positions examined: %d\n", all_positions_ );

    if( all_positions_ > 0 ) {
        printf( "Probe hits        : %d (%05.2f)\n", recog_probe_hits_, (recog_probe_hits_ * 100.0) / all_positions_ );
        printf( "Recognizer hits   : %d (%05.2f)\n", recog_hits_, (recog_hits_ * 100.0) / all_positions_ );

        if( recog_hits_ > 0 ) {
            printf( "Bad exact         : %d (%05.2f)\n", recog_bad_exact_, (recog_bad_exact_ * 100.0) / recog_hits_ );
            printf( "Bad lower bound   : %d (%05.2f)\n", recog_bad_lower_bound_, (recog_bad_lower_bound_ * 100.0) / recog_hits_ );
            printf( "Bad upper bound   : %d (%05.2f)\n", recog_bad_upper_bound_, (recog_bad_upper_bound_ * 100.0) / recog_hits_ );

            int successful = recog_hits_ - recog_bad_exact_ - recog_bad_lower_bound_ - recog_bad_upper_bound_;

            printf( "Successful        : %d (%05.2f)\n", successful, (successful * 100.0) / recog_hits_ );
            printf( "Exact recognitions: %d (%05.2f)\n", recog_exact_, (recog_exact_ * 100.0) / recog_hits_ );
            printf( "Recognized wins   : %d / %d (%05.2f)\n", recog_wins_, egtb_wins_, egtb_wins_ > 0 ? (recog_wins_ * 100.0) / egtb_wins_ : 0.0 );
            printf( "Recognized losses : %d / %d (%05.2f)\n", recog_losses_, egtb_losses_, egtb_losses_ > 0 ? (recog_losses_ * 100.0) / egtb_losses_ : 0.0 );
        }
    }

    if( egtb_failures_ > 0 ) {
        printf( "EGTB failures     : %d\n", egtb_failures_ );
    }

    printf( "\n" );
}

void testPosition( Position & pos, RecognizerStats & stats )
{
    RecognizerInfo  result;

    stats.all_positions_++;

    if( Recognizer::probe( pos, result ) ) {
        stats.recog_probe_hits_++;

        if( result.type() != rtUnknown ) {
            stats.recog_hits_++;

            if( result.type() == rtExact ) {
                stats.recog_exact_++;
            }

            if( result.value() > 500 ) {
                stats.recog_wins_++;
            }
            else if( result.value() < -500 ) {
                stats.recog_losses_++;
            }

            // Use EGTB to validate the recognizer
            int score;

            if( EGTBProbe(pos, &score) ) {
                bool hit = false;

                int bp = pos.blackPawns.bitScanForward();
                int bb = pos.blackQueensBishops.bitScanForward();

                if( pos.whiteKingSquare == 0 && pos.blackKingSquare == 16 && bp == 8 && (bb == 3 || bb == 4) ) {
                    hit = true;
                }

                if( hit ) {
                    Log::assign(0);
                    Log::write( "[X] EGTB=%d, rec=%d\n", score, result.value() );
                    pos.dump();
                    __asm int 3;
                }

                if( score > 5000 ) {
                    stats.egtb_wins_++;
                }
                else if( score < -5000 ) {
                    stats.egtb_losses_++;
                }

                // Compare EGTB with recognizer
                switch( result.type() ) {
                case rtExact:
                    if( result.value() != score ) {
                        stats.recog_bad_exact_++;

                        if( 0 ) {
                            static int n = 0;

                            n++;

                            Log::assign( 0 );
                            Log::write( "\nN=%d\n", n );
                            Log::write( "[E] EGTB=%d, rec=%d\n", score, result.value() );
                            pos.dump();

                            Recognizer::probe( pos, result );
                        }
                    }
                    break;
                case rtLowerBound:
                    if( result.value() > (score + 10) ) {
                        stats.recog_bad_lower_bound_++;

                        if( pos.whiteToMove() && (FileOfSquare(pos.whiteKingSquare) >= RankOfSquare(pos.whiteKingSquare)) ) {
                            static int n = 0;

                            n++;
                            //Log::assign( 0 );
                            Log::write( "\nN=%d\n", n );
                            Log::write( "[L] EGTB=%d, rec=%d\n", score, result.value() );
                            pos.dump();

                            Recognizer::probe( pos, result );
                        }
                    }
                    break;
                case rtUpperBound:
                    if( result.value() < (score - 10) ) {
                        stats.recog_bad_upper_bound_++;
                        
                        if( 0 ) {
                            static int n = 0;

                            n++;

                            Log::assign( 0 );
                            Log::write( "\nN=%d\n", n );
                            Log::write( "[U] EGTB=%d, rec=%d\n", score, result.value() );
                            pos.dump();

                            Recognizer::probe( pos, result );
                        }
                    }
                    break;
                }
            }
            else {
                stats.egtb_failures_++;
            }
        }
    }
}

void testPosition( PositionEnumerator & positions, RecognizerStats & stats )
{
    // Not good for multithreading, but that's not an expected use of this function
    static Position pos;

    // Test both white to move and black to move
    if( positions.getCurrentPosition( pos, true ) ) {
        testPosition( pos, stats );
    }

    if( positions.getCurrentPosition( pos, false ) ) {
        testPosition( pos, stats );
    }

    positions.gotoNextPosition();
}

struct Test {
    int enabled;            // To easily skip tests (can be overriden by a global flag)
    const char * name;      // Name
    int piece_count;        // Number of pieces
    int piece[6];           // Pieces
};

// 3 pieces
Test testKPK_W = { 0, "KPK (white)", 1, { WhitePawn } };
Test testKPK_B = { 0, "KPK (black)", 1, { BlackPawn } };
Test testKNK_W = { 0, "KNK (white)", 1, { WhiteKnight } };
Test testKNK_B = { 0, "KNK (black)", 1, { BlackKnight } };
Test testKBK_W = { 0, "KBK (white)", 1, { WhiteBishop } };
Test testKBK_B = { 0, "KBK (black)", 1, { BlackBishop } };

// 4 pieces without pawns
Test testKNNK_W = { 0, "KNNK (white)", 2, { WhiteKnight, WhiteKnight } };
Test testKNNK_B = { 0, "KNNK (black)", 2, { BlackKnight, BlackKnight } };
Test testKBBK_W = { 0, "KBBK (white)", 2, { WhiteBishop, WhiteBishop } };
Test testKBBK_B = { 0, "KBBK (black)", 2, { BlackBishop, BlackBishop } };
Test testKBNK_W = { 0, "KBNK (white)", 2, { WhiteBishop, WhiteKnight } };
Test testKBNK_B = { 0, "KBNK (black)", 2, { BlackBishop, BlackKnight } };
Test testKQKQ_X = { 0, "KQKQ (white/black)", 2, { WhiteQueen, BlackQueen } };

// 4 pieces with pawns
Test testKPPK_W = { 0, "KPPK (white)", 2, { WhitePawn, WhitePawn } };
Test testKPPK_B = { 0, "KPPK (black)", 2, { BlackPawn, BlackPawn } };
Test testKBPK_W = { 0, "KBPK (white)", 2, { WhiteBishop, WhitePawn } };
Test testKBPK_B = { 1, "KBPK (black)", 2, { BlackBishop, BlackPawn } };
Test testKNPK_W = { 0, "KNPK (white)", 2, { WhiteKnight, WhitePawn } };
Test testKNPK_B = { 0, "KNPK (white)", 2, { BlackKnight, BlackPawn } };

Test testKPKP_X = { 0, "KPKP (white/black)", 2, { WhitePawn, BlackPawn } };
Test testKBKP_W = { 0, "KBKP (white)", 2, { WhiteBishop, BlackPawn } };
Test testKNKP_W = { 0, "KNKP (white)", 2, { WhiteKnight, BlackPawn } };
Test testKRKP_W = { 0, "KRKP (white)", 2, { WhiteRook, BlackPawn } };
Test testKQKP_W = { 0, "KQKP (white)", 2, { WhiteQueen, BlackPawn } };

Test * testSuite[] = {
    &testKPK_W,
    &testKPK_B,

    &testKNK_W,
    &testKNK_B,

    &testKBK_W,
    &testKBK_B,

    &testKNNK_W,
    &testKNNK_B,

    &testKBBK_W,
    &testKBBK_B,

    &testKPPK_W,
    &testKPPK_B,

    &testKBPK_W,
    &testKBPK_B,

    &testKBNK_W,

    // Terminator line
    0
};

void getTestStatsWithEGTB( const Test & test )
{
    PositionEnumerator  positions;
    Position            pos;
    unsigned            progress = 0;

    printf( "Getting stats for %s with EGTB...", test.name );

    // Setup the position
    for( int i=0; i<test.piece_count; i++ ) {
        int pmin = A1;
        int pmax = H8;
        int piece = test.piece[i];

        if( piece == WhitePawn || piece == BlackPawn ) {
            pmin = A2;
            pmax = H7;
        }
        
        positions.addPiece( piece, pmin, pmax );
    }

    // Test each position
    unsigned    wtm_failed  = 0;
    unsigned    wtm_won     = 0;
    unsigned    wtm_draw    = 0;
    unsigned    wtm_lost    = 0;
    unsigned    wtm_invalid = 0;
    unsigned    btm_failed  = 0;
    unsigned    btm_won     = 0;
    unsigned    btm_draw    = 0;
    unsigned    btm_lost    = 0;
    unsigned    btm_invalid = 0;

    int score;

    while( positions.hasMorePositions() ) {
        progress++;

        if( (progress & 65535) == 0 ) {
            printf( "." );
        }

        // Test white to move
        if( positions.getCurrentPosition( pos, true ) ) {
            if( EGTBProbe( pos, &score ) ) {
                if( score > 0 ) {
                    wtm_won++;
                }
                else if( score == 0 ) {
                    wtm_draw++;
                }
                else {
                    wtm_lost++;
                }
            }
            else {
                wtm_failed++;
            }
        }
        else {
            wtm_invalid++;
        }

        positions.gotoNextPosition();
    }

    printf( "/" );

    positions.gotoFirstPosition();

    while( positions.hasMorePositions() ) {
        progress++;

        if( (progress & 65535) == 0 ) {
            printf( "." );
        }

        // Test black to move
        if( positions.getCurrentPosition( pos, false ) ) {
            if( EGTBProbe( pos, &score ) ) {
                if( score > 0 ) {
                    btm_won++;
                }
                else if( score == 0 ) {
                    btm_draw++;
                }
                else {
                    btm_lost++;
                }
            }
            else {
                btm_failed++;
            }
        }
        else {
            btm_invalid++;
        }

        positions.gotoNextPosition();
    }

    printf( " done.\n\n" );

    unsigned wtm_all_valid = wtm_won + wtm_draw + wtm_lost;
    unsigned wtm_all = wtm_all_valid + wtm_invalid;
    unsigned btm_all_valid = btm_won + btm_draw + btm_lost;
    unsigned btm_all = btm_all_valid + btm_invalid;

    printf( "White to move:\n" );
    printf( "  failed = %u\n", wtm_failed );
    printf( "  won    = %9u = %05.2f%% (%05.2f%% of valid)\n", wtm_won, (wtm_won * 100.0) / wtm_all, (wtm_won * 100.0) / wtm_all_valid );
    printf( "  draw   = %9u = %05.2f%% (%05.2f%% of valid)\n", wtm_draw, (wtm_draw * 100.0) / wtm_all, (wtm_draw * 100.0) / wtm_all_valid );
    printf( "  lost   = %9u = %05.2f%% (%05.2f%% of valid)\n", wtm_lost, (wtm_lost * 100.0) / wtm_all, (wtm_lost * 100.0) / wtm_all_valid );
    printf( "  invalid= %9u = %05.2f%%\n", wtm_invalid, (wtm_invalid * 100.0) / wtm_all );

    printf( "Black to move (score is relative to black):\n" );
    printf( "  failed = %u\n", btm_failed );
    printf( "  won    = %9u = %05.2f%% (%05.2f%% of valid)\n", btm_won, (btm_won * 100.0) / btm_all, (btm_won * 100.0) / btm_all_valid );
    printf( "  draw   = %9u = %05.2f%% (%05.2f%% of valid)\n", btm_draw, (btm_draw * 100.0) / btm_all, (btm_draw * 100.0) / btm_all_valid );
    printf( "  lost   = %9u = %05.2f%% (%05.2f%% of valid)\n", btm_lost, (btm_lost * 100.0) / btm_all, (btm_lost * 100.0) / btm_all_valid );
    printf( "  invalid= %9u = %05.2f%%\n", btm_invalid, (btm_invalid * 100.0) / btm_all );
}

static void testRecognizer( const Test & test )
{
    PositionEnumerator  positions;
    RecognizerStats     stats;
    unsigned            progress = 0;
    unsigned            range = 64*64;

    printf( "Testing %s... 00%%", test.name );

    // Setup the position
    for( int i=0; i<test.piece_count; i++ ) {
        int pmin = A1;
        int pmax = H8;
        int piece = test.piece[i];

        if( piece == WhitePawn || piece == BlackPawn ) {
            pmin = A2;
            pmax = H7;
            range *= 48;
        }
        else {
            range *= 64;
        }
        
        positions.addPiece( piece, pmin, pmax );
    }

    // Test each position
    while( positions.hasMorePositions() ) {
        // TODO: should separate loops for wtm and btm, EGTB probe is much faster
        testPosition( positions, stats );

        progress++;

        if( (progress & 1023) == 0 ) {
            printf( "\b\b\b%02d%%", (progress * 100) / range );
        }
    }

    if( (stats.recog_hits_ == 0) || ((stats.recog_bad_exact_ + stats.recog_bad_lower_bound_ + stats.recog_bad_upper_bound_) > 0) ) {
        printf( "\b\b\b\bdone.\n\n" );

        stats.dump( test.name );
    }
    else {
        printf( "\b\b\b\bok.  \n" );
    }
}

static void testRecognizers( Test ** suite )
{
    bool enable_all = false;
    int skipped = 0;

    for( int i=0; suite[i] != 0; i++ ) {
        Test * test = suite[i];

        if( enable_all || test->enabled ) {
            testRecognizer( *test );
        }
        else {
            skipped++;
        }
    }

    if( skipped > 0 ) {
        printf( "[NOTE] The following tests were skipped:\n" );

        while( *suite != 0 ) {
            Test * test = *suite;

            if( ! test->enabled ) {
                printf( "- %s\n", test->name );
            }

            suite++;
        }
    }
}

void testRecognizers()
{
    // getTestStatsWithEGTB( testKBNK_W );
    // testRecognizers( testSuite );
}

#else

void testRecognizers()
{
    printf( "Alessandro, you need to compile with HAVE_EGTB in order to test recognizers...\n" );
}

#endif
