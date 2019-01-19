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

#include "bitbase.h"
#include "log.h"
#include "metrics.h"
#include "packed_array.h"
#include "position.h"
#include "position_enum.h"
#include "recognizer.h"

#ifdef HAVE_EGTB

int EGTBProbe( const Position & pos, int * score );

static bool initializeBoard( Board & board, int wk, int bk )
{
    if( distance( bk, wk ) <= 1 ) {
        return true;
    }

    board.clear();

    board.piece[ wk ] = WhiteKing;
    board.piece[ bk ] = BlackKing;

    return false;
}

static bool initializeBoard( Board & board, int wk, int bk, int type1, int sq1 )
{
    if( ! initializeBoard( board, wk, bk ) ) {
        if( wk == sq1 || bk == sq1 ) {
            return true;
        }

        board.piece[ sq1 ] = type1;

        return false;
    }

    return true;
}

static bool initializeBoard( Board & board, int wk, int bk, int type1, int sq1, int type2, int sq2 )
{
    if( ! initializeBoard( board, wk, bk, type1, sq1 ) ) {
        if( wk == sq2 || bk == sq2 || sq1 == sq2 ) {
            return true;
        }

        board.piece[ sq2 ] = type2;

        return false;
    }

    return true;
}

static int lookupBoard( Board & board, bool wtm, int * score )
{
    Position pos;

    pos.setBoard( board, wtm );

    if( wtm ) {
        if( pos.isSideInCheck(Black) )
            return -1;
    }
    else {
        if( pos.isSideInCheck(White) )
            return -1;
    }

    if( ! EGTBProbe(pos, score) ) {
        return -2;
    }

    return 0;
}

void generateBitbase( int id )
{
    int i;
    unsigned value;
    unsigned max_value;
    unsigned progress = 0;
    int score;
    PositionEnumerator posEnum;
    PackedArray * pa_wtm;
    PackedArray * pa_btm;
    Position pos;
    const BitBaseInfo * info = getBitBaseInfo( id );

    if( info == 0 ) {
        printf( "Uh-oh... no info for bitbase (id = %d)!\n", id );
        return;
    }

    printf( "Generating '%s' bitbase... 00%%", info->filename );

    // Setup position enumerator
    for( i=0; i<info->num_pieces; i++ ) {
        posEnum.addPiece( info->pieces[i] );
    }

    // Prepare bitbase
    pa_wtm = new PackedArray( getBitBaseIndexRange(id), info->bits );
    pa_btm = new PackedArray( getBitBaseIndexRange(id), info->bits );

    assert( pa_wtm != 0 );
    assert( pa_btm != 0 );

    if( info->flags & bb_DefaultIs1 ) {
        pa_wtm->setAll();
        pa_btm->setAll();
    }
    else {
        pa_wtm->clear();
        pa_btm->clear();
    }

    max_value = 1 << info->bits;

    // Loop thru all positions and generate bitbase for white to move
    while( posEnum.hasMorePositions() ) {
        if( FileOfSquare( posEnum.getWhiteKingPos() ) >= 4 ) {
            // Skip this position (simmetry)
            posEnum.gotoNextPosition();
            continue;
        }

        progress++;

        if( (progress & 1023) == 0 ) {
            printf( "\b\b\b%02u%%", (progress * 100) / (2*getBitBaseIndexRange(id)) );
        }

        int index = getBitBaseIndex( id, posEnum );

        // White to move
        if( posEnum.getCurrentPosition( pos, true ) ) {
            value = bb_Draw;

            if( EGTBProbe( pos, &score ) ) {
                if( score > 0 ) {
                    value = bb_WhiteWins;
                }
                else if( score < 0 ) {
                    value = bb_BlackWins;
                }
            }
            else {
                assert( 0 );
            }

            if( value >= max_value ) {
                printf( " *** Fatal: value out of range (wtm: %u)\n", value );
                return;
            }

            pa_wtm->set( index, value );
        }

        posEnum.gotoNextPosition();
    }

    // Now loop again thru all positions and generate bitbase for black to move
    // (this seems to be a *lot* faster than doing both sides in the same iteration,
    // at least for some piece sets)
    posEnum.gotoFirstPosition();

    while( posEnum.hasMorePositions() ) {
        if( FileOfSquare( posEnum.getWhiteKingPos() ) >= 4 ) {
            // Skip this position (simmetry)
            posEnum.gotoNextPosition();
            continue;
        }

        progress++;

        if( (progress & 1023) == 0 ) {
            printf( "\b\b\b%02u%%", (progress * 100) / (2*getBitBaseIndexRange(id)) );
        }

        int index = getBitBaseIndex( id, posEnum );

        // Black to move
        if( posEnum.getCurrentPosition( pos, false ) ) {
            value = bb_Draw;

            if( EGTBProbe( pos, &score ) ) {
                if( score < 0 ) {
                    value = bb_WhiteWins;
                }
                else if( score > 0 ) {
                    value = bb_BlackWins;
                }
            }
            else {
                assert( 0 );
            }

            if( value >= max_value ) {
                printf( " *** Fatal: value out of range (btm: %u)\n", value );
                return;
            }

            pa_btm->set( index, value );
        }

        //
        posEnum.gotoNextPosition();
    }

    printf( "\n" );

    // Save bitbases
    saveBitBase( id, 0, pa_btm );
    saveBitBase( id, 1, pa_wtm );
}

void generateBitbases()
{
    generateBitbase( bb_KPK );
    generateBitbase( bb_KPPK );
    generateBitbase( bb_KBPK );
    generateBitbase( bb_KBNK );
}

#else

void generateBitbases()
{
    printf( "Alessandro, you need to compile with HAVE_EGTB in order to generate bitbases...\n" );
}

#endif
