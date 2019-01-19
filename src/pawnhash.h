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
#ifndef PAWNHASH_H_
#define PAWNHASH_H_

#include "bitboard.h"
#include "counters.h"
#include "position.h"

enum {
    // Flags #1
    BlackPassedBase     = 0x00000001,
    BlackPassedMask     = 0x000000FF,
    WhitePassedBase     = 0x00000100,
    WhitePassedMask     = 0x0000FF00,
    //
    BlackPassedQueenSide= 0x000000C0,
    BlackPassedKingSide = 0x00000003,
    WhitePassedQueenSide= 0x0000C000,
    WhitePassedKingSide = 0x00000300,

    // Warning: these fields use a negated boolean value, so a clear bit means 
    // an (half)open file: this makes it easier to check for fully open files
    // as we can use an "and" operation and then simply test for zero
    BlackOpenFileBase   = 0x00010000,
    BlackOpenFileMask   = 0x00FF0000,
    WhiteOpenFileBase   = 0x01000000,
    WhiteOpenFileMask   = 0xFF000000,

    // Flags #2
    PositionLockedMask  = 0x000000FF,
    PawnHashValidEntry  = 0x00000100,
    WhiteBadLiteBishop  = 0x00000200,
    WhiteBadDarkBishop  = 0x00000400,
    BlackBadLiteBishop  = 0x00000800,
    BlackBadDarkBishop  = 0x00001000,
};

class PawnHashEntry
{
public:
    friend class PawnHashTable;

    // Constructor (empty)
    PawnHashEntry() {
    }

    // Destructor (empty)
    ~PawnHashEntry() {
    }

    // Data manipulation methods
    void setFlags1( unsigned f ) {
        flags1 = f;
    }

    void setFlags2( unsigned f ) {
        flags2 = f;
    }

    void setBlackScore( int v1, int v2 ) {
        blackScore = (((unsigned)(v1+0x8000)) << 16) | ((unsigned)(v2+0x8000));
    }

    void setWhiteScore( int v1, int v2 ) {
        whiteScore = (((unsigned)(v1+0x8000)) << 16) | ((unsigned)(v2+0x8000));
    }

    void reset() {
        flags2 = 0;
    }

    // Access methods
    unsigned isValid() const {
        return flags2 & PawnHashValidEntry;
    }

    Uint32      flags1;
    Uint32      flags2;
    Uint32      whiteScore;
    Uint32      blackScore;
    BitBoard    code;
};

class PawnHashTable
{
public:
    PawnHashTable( unsigned n );

    ~PawnHashTable();

    void    reset();

    PawnHashEntry * probe( const Position & pos ) const;

    PawnHashEntry * store( const Position & pos, int bs1, int bs2, int ws1, int ws2, unsigned f1, unsigned f2 );

    unsigned getSize() const {
        return size;
    }

private:
    // Unimplemented methods
    PawnHashTable( const PawnHashTable & );
    PawnHashTable & operator = ( const PawnHashTable & );

    unsigned    xhash( const Position & pos ) const {
        return pos.pawnHashCode.toUnsigned();
    }

    PawnHashEntry * table;
    unsigned        size;   // Size of table (number of entries)
    unsigned        mask;
};

#endif // PAWNHASH_H_
