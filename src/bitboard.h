/*
    Kiwi
    Bitboard management

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
#ifndef BITBOARD_H_
#define BITBOARD_H_

// Note: assembly code removed in version 0.6d

#include <stdio.h>
#include "platform.h"

extern CACHE_ALIGN const unsigned int lsz64_tbl[64];

/*
    Bit count function by Gerd Isenberg.
*/
inline int bitCount( Uint64 bb )
{
   unsigned w = (unsigned) (bb >> 32);
   unsigned v = (unsigned) bb;

   v = v - ((v >> 1) & 0x55555555);
   w = w - ((w >> 1) & 0x55555555);
   v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
   w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
   v = (v + (v >> 4)) & 0x0F0F0F0F;
   w = (w + (w >> 4)) & 0x0F0F0F0F;
   v = ((v+w) * 0x01010101) >> 24;

   return v;
}

/**
    Bitboard, i.e. a 64-bit unsigned integer with some extra chess-related functions.
*/
class BitBoard
{
public:
    static const BitBoard   Clr[64];
    static const BitBoard   Set[64];

    //
    BitBoard() {
    }

    BitBoard( Uint32 lo, Uint32 hi ) {
        data = ((Uint64)hi << 32) | ((Uint64)lo);
    }

    BitBoard( Uint64 n ) {
        data = n;
    }

    BitBoard( const BitBoard & bb ) {
        data = bb.data;
    }

    //
    ~BitBoard() {
    }

    //
    void operator = ( const BitBoard & bb ) {
        data = bb.data;
    }

    void operator = ( Uint64 bb ) {
        data = bb;
    }

    //
    void operator &= ( const BitBoard & bb ) {
        data &= bb.data;
    }

    void operator |= ( const BitBoard & bb ) {
        data |= bb.data;
    }

    void operator ^= ( const BitBoard & bb ) {
        data ^= bb.data;
    }

    //
    void operator &= ( Uint64 bb ) {
        data &= bb;
    }

    void operator |= ( Uint64 bb ) {
        data |= bb;
    }

    void operator ^= ( Uint64 bb ) {
        data ^= bb;
    }

    BitBoard operator << ( int len ) const {
        return BitBoard( data << len );
    }

    BitBoard operator >> ( int len ) const {
        return BitBoard( data >> len );
    }

    //
    BitBoard operator ~ () const {
        return BitBoard( ~data );
    }

    BitBoard operator & ( const BitBoard & bb ) const {
        return BitBoard( data & bb.data );
    }

    BitBoard operator | ( const BitBoard & bb ) const {
        return BitBoard( data | bb.data );
    }

    BitBoard operator ^ ( const BitBoard & bb ) const {
        return BitBoard( data ^ bb.data );
    }

    int operator < ( const BitBoard & bb ) const {
        return data < bb.data;
    }

    int operator > ( const BitBoard & bb ) const {
        return data > bb.data;
    }

    //
    int operator == ( const BitBoard & bb ) const {
        return data == bb.data;
    }

    int operator != ( const BitBoard & bb ) const {
        return data != bb.data;
    }

    //
    operator int () const {
        return data != 0;
    }

    //
    void setBit( unsigned idx ) {
        data |= Set[idx].data;
    }

    void clrBit( unsigned idx ) {
        data &= Clr[idx].data;
    }

    //
    int getBit( unsigned idx ) const {
        return (data & Set[idx].data) != 0 ? 1 : 0;
    }

    //
    void clear( void ) {
        data = 0;
    }

    void negate( void ) {
        data = ~data;
    }

    void andNot( const BitBoard & bb ) {
        data &= ~bb.data;
    }

    //
    unsigned getRank( int rank ) const {
        return 0xFF & (unsigned)(data >> (rank*8));
    }

    //
    int isEmpty() const {
        return (data == 0);
    }

    //
    unsigned toByte( void ) const {
        return (unsigned)data & 0xFF;
    }

    //
    unsigned toUnsigned( void ) const {
        return (unsigned)data;
    }

    // Dumps this bitboard to the logfile
    void dump( const char * header = 0 ) const;

    bool isZero() const {
        return data == 0;
    }

    bool isNotZero() const {
        return data != 0;
    }

    int bitScanForward() const;

public:
    Uint64  data;
};

/*
    Bit search functions by Gerd Isenberg.
*/
inline unsigned int bitSearch( BitBoard bb )
{
   Uint64 b = bb.data ^ (bb.data - 1);
   unsigned int fold = ((unsigned)b) ^ ((unsigned)(b>>32));
   return lsz64_tbl[(fold * 0x78291ACF) >> (32-6)];
}

inline unsigned int bitSearchAndReset( BitBoard & bb )
{
    Uint64 b = bb.data ^ (bb.data - 1);
    bb.data = bb.data & (bb.data - 1);
    unsigned int fold = ((unsigned)b) ^ ((unsigned)(b>>32));
    return lsz64_tbl[(fold * 0x78291ACF) >> (32-6)];
}

inline int bitScanForward( BitBoard bb )
{
    return bb.data != 0 ? bitSearch(bb) : -1;
}

inline int bitScanAndResetForward( BitBoard & bb )
{
    return bb.data != 0 ? bitSearchAndReset(bb) : -1;
}

inline int bitCount( const BitBoard & b )
{
    return bitCount( b.data );
}

inline int BitBoard::bitScanForward() const {
    return ::bitScanForward( data );
}

#endif // BITBOARD_H_
