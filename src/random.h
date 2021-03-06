/*
    Kiwi
    Random number generation (for Zobrist hashing)

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
#ifndef RANDOM_H_
#define RANDOM_H_

#include "bitboard.h"

class Random
{
public:
    Random();
    
    ~Random() {
    }

    unsigned    get();

    BitBoard    getBitBoard();

private:
    unsigned    x[55];
    unsigned    j;
    unsigned    k;

    // Not implemented
    Random( const Random & );
    Random & operator = ( const Random & );
};

#endif // RANDOM_H_
