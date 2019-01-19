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
#include "engine.h"
#include "log.h"
#include "undoinfo.h"
#include "san.h"

static unsigned perft_nodes[Engine::MaxSearchPly];

static void perft_search( Position & pos, int depth )
{
    if( depth >= 0 ) {
        MoveList    movelist;
        UndoInfo    undoinfo( pos );

        if( pos.boardFlags & Position::SideToPlayInCheck ) {
            pos.generateCheckEscapes( movelist );
        }
        else {
            pos.generateMoves( movelist );
        }

        for( int i=0; i<movelist.count(); i++ ) {
            Move    m = movelist.get(i);

            if( pos.doMove( m ) == 0 ) {
                ++perft_nodes[depth];

                perft_search( pos, depth-1 );
            }

            pos.undoMove( m, undoinfo );
        }
    }
}

/*
    The perft() function performs a full traversal of the move tree down to the
    specified depth.

    It is used to test and benchmark the move generation code, and can be verified
    against known results.

    *** Position::startPosition
    1) = 20
    2) = 400
    3) = 8902
    4) = 197281
    5) = 4865609
    6) = 119060324

    *** "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
    1) = 48
    2) = 2039
    3) = 97862
    4) = 4085603
    5) = 193690690


    *** "8/PPP4k/8/8/8/8/4Kppp/8 w - -"
    1) = 18
    2) = 290
    3) = 5044
    4) = 89363
    5) = 1745545
    6) = 34336777
    7) = 749660761

    *** "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"

    1) = 14
    2) = 191
    3) = 2812
    4) = 43238
    5) = 674624
    6) = 11030083
    7) = 178633661
*/
int Engine::perft( const char * fen, int max_depth )
{
    printf( "perft: depth=%d, FEN=%s\n", max_depth, fen );

    Position pos;

    pos.setBoard( fen );

    int i;

    for( i=0; i<8; i++ ) {
        perft_nodes[i] = 0;
    }

    max_depth--;

    unsigned t = System::getTickCount();

    perft_search( pos, max_depth );

    t = System::getTickCount() - t;

    unsigned c = 0;

    for( i=max_depth; i>=0; i-- ) {
        c += perft_nodes[i];
        printf( "  depth=%d, nodes=%d\n", max_depth-i+1, perft_nodes[i] );
    }

    if( t == 0 ) t = 1;

    printf( "perft complete: total=%d nodes in %d.%03d seconds (%d KNps)\n\n", c, t / 1000, t % 1000, c / t );

    return 0;
}

int Engine::perftRunSuite()
{
    // Start position
    perft( Position::startPosition, 6 );

    // Interesting position, helped me catch a nasty bug in the check evasion generator
    // (caused by a wrongly computed Attacks::SquaresBetween array)
    perft( "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5 );

    // Endgame position
    perft( "8/PPP4k/8/8/8/8/4Kppp/8 w - -", 6 );

    // This spotted another (last?!?) bug in the check evasion generator, when the
    // piece that gives check is a pawn that can be captured en-passant...
    perft( "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 6 );

    return 0;
}
