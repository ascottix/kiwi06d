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
#ifndef HASH_H_
#define HASH_H_

// #define TEST_HASH

#include "bitboard.h"
#include "counters.h"
#include "move.h"
#include "position.h"

/*
    Bits    Bytes   Description
    ----    -----   -----------
    24      3       Move
     3      1       Age
     2      -       -
     1      -       Null move reported a mate threat
     1      -       There is only one valid move in this position
     1      -       Value type (0=lo bound, 1=hi bound)
    16      2       Value
    16      2       Depth
    64      8       Hash
*/

class HashTable
{
public:
    struct Entry
    {
        friend class HashTable;

        enum {
            LowerBound = 0,
            UpperBound          = 0x80000000,
            SingleReply         = 0x40000000,
            MateThreat          = 0x20000000,
            ExactBound          = 0x10000000,
            SearchIdIncrement   = 0x01000000,
            SearchIdMask        = 0x0F000000,
        };

        // Constructor (empty)
        Entry() {
        }

        // Destructor (empty)
        ~Entry() {
        }

        // Data manipulation methods
        void packData1( const Move & move, unsigned flags, unsigned search_id ) {
            data1 = flags | search_id | move.toUint24();
        }

        void packData2( int value, int depth ) {
            data2 = ((unsigned)(value+0x8000)) | (depth << 16);
        }

        void reset() {
            code = 0;
        }

        // Access methods
        unsigned isUpperBound() const {
            return data1 & UpperBound;
        }

        unsigned hasSingleReply() const {
            return data1 & SingleReply;
        }

        unsigned hasMateThreat() const {
            return data1 & MateThreat;
        }

        int getValue() const {
            return ((int)data2 & 0xFFFF)-0x8000;
        }

        int getDepth() const {
            return (int)(data2 >> 16);
        }

        // For PVS
        int getBound() const {
            return data1 & (UpperBound | ExactBound);
        }

        unsigned getSearchId() const {
            return data1 & SearchIdMask;
        }

        Move getMove() const {
            return Move( data1 & 0x00FFFFFF );
        }

    private:
        BitBoard    code;
        Uint32      data1;
        Uint32      data2;

#ifdef TEST_HASH
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
        unsigned        boardFlags;
        int             sideToPlay;
#endif
    };

    HashTable( unsigned n );

    ~HashTable();

    void    reset();

    Entry * probe( const Position & pos ) const;

    void    store( const Position & pos, const Move & move, int value, unsigned flags, int depth );

    void    clean( const Position & pos );

    void    bumpSearchId() {
        search_id = (search_id + Entry::SearchIdIncrement ) & Entry::SearchIdMask;
    }

    unsigned getSize() const {
        return size;
    }

    void    dumpStats();

    int backup( char * buf, unsigned len );

    int restore( char * buf, unsigned len );

private:
    // Unimplemented methods
    HashTable( const HashTable & );
    HashTable & operator = ( const HashTable & );

    Entry *     table;
    unsigned    mask;
    unsigned    search_id;
    unsigned    size;   // Size of table (number of entries)
};

struct EvalItem
{
    Uint32 code;
    int eval;
};

const int ItemsInEvalCache = 256*1024; // Must be power of 2

extern EvalItem evalCache[ ItemsInEvalCache ];

#endif // HASH_H_
