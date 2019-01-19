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
#include <limits.h>
#include <string.h>

#include "counters.h"
#include "hash.h"
#include "log.h"
#include "position.h"
#include "score.h"

#define HASH_ENTRY( pos )   table + (pos.hashCode.toUnsigned() & mask)

HashTable::HashTable( unsigned n )
{
    assert( (n & (n-1)) == 0 );     // Make sure size is a power of two

#ifdef TEST_HASH
    n &= ~3;
    printf( "There are %d entries in the hash table\n", n );
#endif

    size = n;
    mask = n-1;
    mask &= ~3; // Store four positions per entry, so there is one fourth of all entries
    search_id = 0;
    table = new Entry[ size ];
}

HashTable::~HashTable()
{
    delete [] table;
}

void HashTable::reset()
{
    memset( table, 0, sizeof(Entry)*size );

    memset( evalCache, 0, sizeof(evalCache) );
}

#ifdef TEST_HASH

static void comp( const char * header, const BitBoard & b1, const BitBoard & b2 )
{
    if( b1 != b2 ) {
        printf( "\n%s\n", header );
        b1.dump();
        printf( "\n" );
        b2.dump();
        printf( "\n" );
    }
}

#undef HASH_ENTRY

#define HASH_ENTRY( pos )   table + ((pos.hashCode.toUnsigned() % size) & ~3);

#endif

HashTable::Entry * HashTable::probe( const Position & pos ) const
{
    Counters::hashProbes++;

#ifdef TEST_HASH
    pos.verifyHashCode();
#endif

    Entry * entry = HASH_ENTRY(pos);

    if( entry->code == pos.hashCode ) {
    }
    else {
        entry++; // Probe the second slot
        if( entry->code == pos.hashCode ) {
        }
        else {
            entry++; // Probe the third slot
            if( entry->code == pos.hashCode ) {
            }
            else {
            }

            entry++; // Probe the fourth slot
            if( entry->code == pos.hashCode ) {
            }
            else {
                entry = 0;
                Counters::hashProbesFailed++;
            }
        }
    }

#ifdef TEST_HASH
    if( entry != 0 ) {
        bool ok = 
            entry->whitePieces == pos.whitePieces &&
            entry->blackPieces == pos.blackPieces &&
            entry->whitePawns == pos.whitePawns &&
            entry->blackPawns == pos.blackPawns &&
            entry->whiteKnights == pos.whiteKnights &&
            entry->blackKnights == pos.blackKnights &&
            entry->whiteQueensBishops == pos.whiteQueensBishops &&
            entry->blackQueensBishops == pos.blackQueensBishops &&
            entry->whiteQueensRooks == pos.whiteQueensRooks &&
            entry->blackQueensRooks == pos.blackQueensRooks &&
            (entry->boardFlags & 0xF00FF) == (pos.boardFlags & 0xF00FF) &&
            entry->sideToPlay == pos.sideToPlay;

        if( ! ok ) {
            Log::assign(0);
            pos.dump();

            comp( "wpcs", entry->whitePieces , pos.whitePieces );
            comp( "bpcs", entry->blackPieces , pos.blackPieces );
            comp( "wp", entry->whitePawns , pos.whitePawns );
            comp( "bp", entry->blackPawns , pos.blackPawns );
            comp( "wn", entry->whiteKnights , pos.whiteKnights );
            comp( "bn", entry->blackKnights , pos.blackKnights );
            comp( "wqb", entry->whiteQueensBishops , pos.whiteQueensBishops );
            comp( "bqb", entry->blackQueensBishops , pos.blackQueensBishops );
            comp( "wqr", entry->whiteQueensRooks , pos.whiteQueensRooks );
            comp( "bqr", entry->blackQueensRooks , pos.blackQueensRooks );
            printf( "F: %08x  %08x\n", entry->boardFlags, pos.boardFlags );
            printf( "S: %d  %d\n", entry->sideToPlay, pos.sideToPlay );

            __asm int 3;
        }
    }
#endif

    return entry;
}

void HashTable::store( const Position & pos, const Move & move, int value, unsigned flags, int depth )
{
    Counters::hashStores++;

#ifdef TEST_HASH
    pos.verifyHashCode();
#endif

    Entry * entry = HASH_ENTRY(pos);

    /*
        Replacement rules (in order of enforcement):
        1) entries with the same hash code;
        2) entries from a different search;
        3) entries with a smaller search depth.
    */
    int index = 0;

    for( int i=0; i<4; i++ ) {
        if( entry[i].code == pos.hashCode ) {
            // Position is already in the table

            /*
            if( entry[i].getSearchId() == search_id && entry->getDepth() > depth ) {
                return;
            }
            */
            break;
        }

        if( entry[i].getSearchId() != search_id ) {
            // Entry from different search: replace if current choice is from this search or less deep
            if( entry[index].getSearchId() == search_id || entry[index].getDepth() > entry[i].getDepth() ) {
                index = i;
            }
        }
        else {
            // Entry from same search: replace only if current choice is from this search and less deep
            if( entry[index].getSearchId() == search_id && entry[index].getDepth() > entry[i].getDepth() ) {
                index = i;
            }
        }
    }

    entry[index].packData1( move, flags, search_id );
    entry[index].packData2( value, depth );
    entry[index].code = pos.hashCode;

#ifdef TEST_HASH
    entry[index].whitePieces = pos.whitePieces;
    entry[index].blackPieces = pos.blackPieces;
    entry[index].whitePawns = pos.whitePawns;
    entry[index].blackPawns = pos.blackPawns;
    entry[index].whiteKnights = pos.whiteKnights;
    entry[index].blackKnights = pos.blackKnights;
    entry[index].whiteQueensBishops = pos.whiteQueensBishops;
    entry[index].blackQueensBishops = pos.blackQueensBishops;
    entry[index].whiteQueensRooks = pos.whiteQueensRooks;
    entry[index].blackQueensRooks = pos.blackQueensRooks;
    entry[index].boardFlags = pos.boardFlags;
    entry[index].sideToPlay = pos.sideToPlay;
#endif
}

void HashTable::clean( const Position & pos )
{
    Entry * entry = HASH_ENTRY(pos);

    entry[0].reset();
    entry[1].reset();
    entry[2].reset();
    entry[3].reset();
}
