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
#include "bitbase.h"
#include "bitbase_adjust.h"
#include "log.h"
#include "metrics.h"

static void initReport( int id, int wtm, int op )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );

    if( op == AdjustBb_Encode ) {
        printf( "Adjusting %s/%s...", info->filename, wtm ? "wtm" : "btm" );
    }
    else {
        Log::write( "Decoding %s/%d...", info->filename, wtm );
    }
}

static void termReport( int bb, int op, unsigned good )
{
    if( op == AdjustBb_Encode ) {
        printf( " done, accuracy = %05.2f\n", (good * 100.0) / getBitBaseIndexRange(bb) );
    }
    else {
        Log::write( " done\n" );
    }
}

static void setupEnum( PositionEnumerator & posEnum, int id )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );

    for( int i=0; i<info->num_pieces; i++ ) {
        posEnum.addPiece( info->pieces[i] );
    }
}

int bbAdjustKPPK( PackedArray * pa, int id, int wtm, int op )
{
    initReport( id, wtm, op );

    assert( id == bb_KPPK );

    // Predictions are based on the KPK bitbases, get them
    PackedArray * kpk_wtm = getBitBase( bb_KPK, bb_WtM );
    PackedArray * kpk_btm = getBitBase( bb_KPK, bb_BtM );

    if( kpk_wtm == 0 || kpk_btm == 0 ) {
        Log::write( "*** Warning: KPPK adjuster can't find KPK bitbase\n" );
        return -1;
    }

    // Use also the KPPK table with black to move
    PackedArray * kppk_btm = 0;
    
    if( wtm ) {
        kppk_btm = getBitBase( id, bb_BtM );
    }

    PositionEnumerator posEnum;

    posEnum.addPiece( WhitePawn );
    posEnum.addPiece( WhitePawn );

    unsigned good_predictions = 0;

    while( posEnum.hasMorePositions() ) {
        if( FileOfSquare( posEnum.getWhiteKingPos() ) >= 4 ) {
            // Skip this position (simmetry)
            posEnum.gotoNextPosition();
            continue;
        }

        if( ! posEnum.isValidPosition( wtm ? true : false ) ) {
            good_predictions++;
            posEnum.gotoNextPosition();
            continue;
        }

        int index = getBitBaseIndex( id, posEnum );

        // Predict and fix the bitbase
        int wk = posEnum.getWhiteKingPos();
        int bk = posEnum.getBlackKingPos();
        int p1 = posEnum.getPiecePos( 2 );
        int p2 = posEnum.getPiecePos( 3 );

        if( p1 > p2 ) {
            int x = p1;
            p1 = p2;
            p2 = x;
        }

        unsigned predicted_value = 
            kpk_wtm->get( getBitBaseIndex(bb_KPK,wk,bk,p1) ) |
            kpk_wtm->get( getBitBaseIndex(bb_KPK,wk,bk,p2) ) |
            kpk_btm->get( getBitBaseIndex(bb_KPK,wk,bk,p1) ) |
            kpk_btm->get( getBitBaseIndex(bb_KPK,wk,bk,p2) );

        if( RankOfSquare(p1) == 1 || RankOfSquare(p2) == 1 ) {
            predicted_value = 1;
        }

        if( FileOfSquare(p1) < 7 && p2 == (p1+1) ) predicted_value = 1;
        if( FileOfSquare(p1) < 6 && p2 == (p1+2) ) predicted_value = 1;
        if( FileOfSquare(p1) < 5 && p2 == (p1+3) ) predicted_value = 1;

        if( kppk_btm != 0 ) {
            predicted_value |= kppk_btm->get(index);
        }

        if( op == AdjustBb_Encode && predicted_value == pa->get(index) ) {
            good_predictions++;
        }

        pa->set( index, pa->get(index) ^ predicted_value );

        posEnum.gotoNextPosition();
    }

    termReport( id, op, good_predictions );

    return 0;
}

int bbAdjustKBPK( PackedArray * pa, int id, int wtm, int op )
{
    initReport( id, wtm, op );

    assert( id == bb_KBPK );

    PositionEnumerator posEnum;

    posEnum.addPiece( WhiteBishop );
    posEnum.addPiece( WhitePawn );

    unsigned good_predictions = 0;

    while( posEnum.hasMorePositions() ) {
        if( FileOfSquare( posEnum.getWhiteKingPos() ) < 4 ) {
            int index = getBitBaseIndex( id, posEnum );

            unsigned predicted_value = 1;

            if( posEnum.isValidPosition( wtm ? true : false ) ) {
                // Predict and fix the bitbase
                int wk = posEnum.getWhiteKingPos();
                int bk = posEnum.getBlackKingPos();
                int wb = posEnum.getPiecePos( 2 );
                int wp = posEnum.getPiecePos( 3 );

                if( ! wtm ) {
                    if( FileOfSquare(wp) == 0 && ! is_light_square(wb) ) {
                        if( distance( bk, A8 ) < distance( wk, A8 ) && distance(bk,A8) <= distance( wp,A8) ) {
                            predicted_value = 0;
                        }
                    }
            
                    if( distance( bk, wp ) == 1 && ! Attacks::Bishop[wb].getBit(wp) && ! Attacks::King[wk].getBit(wp) ) {
                        predicted_value = 0;
                    }
                }
                else {
                    if( FileOfSquare(wp) == 0 && ! is_light_square(wb) ) {
                        if( distance( bk, A8 ) < (distance( wk, A8 )-1) && distance(bk,A8) < distance( wp,A8) ) {
                            predicted_value = 0;
                        }
                    }
                }
            }

            if( op == AdjustBb_Encode && predicted_value == pa->get(index) ) {
                good_predictions++;
            }

            pa->set( index, pa->get(index) ^ predicted_value );
        }

        posEnum.gotoNextPosition();
    }

    termReport( id, op, good_predictions );

    return 0;
}

int bbAdjustKBNK( PackedArray * pa, int id, int wtm, int op )
{
    initReport( id, wtm, op );

    assert( id == bb_KBNK );

    PositionEnumerator posEnum;

    setupEnum( posEnum, id );

    unsigned good_predictions = 0;

    while( posEnum.hasMorePositions() ) {
        if( FileOfSquare( posEnum.getWhiteKingPos() ) < 4 ) {
            int index = getBitBaseIndex( id, posEnum );

            unsigned predicted_value = 1;

            if( ! wtm && posEnum.isValidPosition( false ) ) {
                // Predict and fix the bitbase
                int wk = posEnum.getWhiteKingPos();
                int bk = posEnum.getBlackKingPos();
                int wb = posEnum.getPiecePos( 2 );
                int wn = posEnum.getPiecePos( 3 );

                // Check whether the black king can immediately capture a piece
                if( distance( bk, wb ) <= 1 && distance( wk, wb ) > 1 && ! Attacks::Knight[wn].getBit(wb) ) {
                    predicted_value = 0;
                }

                if( distance( bk, wn ) <= 1 && distance( wk, wn ) > 1 && ! Attacks::Bishop[wb].getBit(wn) ) {
                    predicted_value = 0;
                }
            }

            if( op == AdjustBb_Encode && predicted_value == pa->get(index) ) {
                good_predictions++;
            }

            pa->set( index, pa->get(index) ^ predicted_value );
        }

        posEnum.gotoNextPosition();
    }

    termReport( id, op, good_predictions );

    return 0;
}
