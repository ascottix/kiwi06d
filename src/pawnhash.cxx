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
#include <string.h>

#include "counters.h"
#include "log.h"
#include "pawnhash.h"
#include "position.h"

PawnHashTable::PawnHashTable( unsigned n )
{
    // Make sure the number of entries is a power of two
    if( (n & (n-1)) != 0 ) {
        while( (n & (n-1)) != 0 ) {
            n &= n-1;
        }

        Log::write( "Pawn hash entries adjusted to %d, overall size is %dK\n", n, (n*sizeof(PawnHashEntry)) / 1024 );
    }

    size = n;
    mask = n-1;
    table = new PawnHashEntry[ size ];
}

PawnHashTable::~PawnHashTable()
{
    delete [] table;
}

void PawnHashTable::reset()
{
    memset( table, 0, size * sizeof(PawnHashEntry) );
}

PawnHashEntry * PawnHashTable::probe( const Position & pos ) const
{
    Counters::pawnHashProbes++;

    PawnHashEntry * entry = table + (xhash(pos) & mask);

    if( entry->isValid() && entry->code == pos.pawnHashCode ) {
        return entry;
    }

    Counters::pawnHashProbesFailed++;

    return 0;
}

PawnHashEntry * PawnHashTable::store( const Position & pos, int bs1, int bs2, int ws1, int ws2, unsigned f1, unsigned f2 )
{
    Counters::pawnHashStores++;

    PawnHashEntry * entry = table + (xhash(pos) & mask);

    entry->setBlackScore( bs1, bs2 );
    entry->setWhiteScore( ws1, ws2 );
    entry->setFlags1( f1 );
    entry->setFlags2( f2 | PawnHashValidEntry );
    entry->code = pos.pawnHashCode;

    return entry;
}
