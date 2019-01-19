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
#ifndef MOVELIST_H_
#define MOVELIST_H_

#include <stdio.h>

#include "bitboard.h"
#include "move.h"
#include "position.h"

class MoveList
{
public:
    enum {
        MaxMoveCount = 218  // This is the largest value know to theory...
    };

    MoveList() {
        len = 0;
    }

    void add( int from, int to ) {
        move[len++].assign( from, to );
    }

    void add( int from, int to, int promotion ) {
        move[len++].assign( from, to, promotion );
    }

    void add( Move m ) {
        move[len++] = m;
    }

    Move get() {
        return move[--len];
    }

    Move get( int index ) {
        return move[index];
    }

    int count() const {
        return len;
    }

    bool isEmpty() const {
        return (len == 0);
    }

    void reset() {
        len = 0;
    }

    void swap( int index1, int index2 );

    void dump( const char * header = 0 );

    Move    move[ MaxMoveCount ];
    int     len;
};

#endif // MOVELIST_H_
