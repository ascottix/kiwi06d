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
#ifndef BITBASE_H_
#define BITBASE_H_

#include <assert.h>

#include "board.h"
#include "packed_array.h"
#include "position_enum.h"

// All known bitbases
enum {
    // 3-pieces
    bb_KPK = 0,
    // 4-pieces
    bb_KBKP,
    bb_KBPK,
    bb_KBNK,
    bb_KNKP,
    bb_KNPK,
    bb_KPKP,
    bb_KPPK,
    bb_KQKB,
    bb_KQKN,
    bb_KQKP,
    bb_KQKQ,
    bb_KQKR,
    bb_KRKB,
    bb_KRKN,
    bb_KRKP,
    bb_KRKR,
};

enum {
    // Side to move
    bb_BtM = 0,
    bb_WtM = 1,
    // Bits per position: note that white here means "the strongest side"
    bb_BitsWin  = 1,    // White wins/draws (1 bit/position)
    bb_BitsWDL  = 2,    // White wins/draws/loses (2 bits/position)
    // Bits meaning
    bb_Draw         = 0,
    bb_WhiteWins    = 1,
    bb_BlackWins    = 2,
    // Flags
    bb_PackMask = 0x0F,
    bb_PackRLE  = 1,
    bb_DefaultIs1   = 0x100,    // Bitbase is initialized to all 1's rather than 0's
};

// Bitbase-specific function for encoding/decoding
typedef int (* AdjustBitBaseFunc) ( PackedArray * pa, int id, int wtm, int op );

/** Bitbase information. */
struct BitBaseInfo
{
    int id;                         // Identifier
    unsigned bits;                  // Bits per position
    int num_pieces;                 // Number of pieces (kings excluded)
    int pieces[3];                  // Piece list
    unsigned flags;                 // Flags (e.g. compression)
    const char * filename;          // Filename (prefix)
    AdjustBitBaseFunc adjust_func;  // Encode/decode function
};

/** Initializes the bitbase library. */
void initBitBases();

/** Loads all bitbases from disk. */
void loadBitBases();

/** Saves the specified bitbase data. */
int saveBitBase( int id, int wtm, const PackedArray * pa );

/** Returns information on the specified bitbase, or a null pointer. */
const BitBaseInfo * getBitBaseInfo( int id );

/** Returns the data for the specified bitbase. */
PackedArray * getBitBase( int id, int wtm );

/** Returns the index for the specified 3-pieces position. */
int getBitBaseIndex( int id, int wk, int bk, int p1 );

/** Returns the index for the specified 4-pieces position. */
int getBitBaseIndex( int id, int wk, int bk, int p1, int p2 );

int getBitBaseIndex( int id, const PositionEnumerator & pe );

/** Returns the index range (i.e. max value + 1) for the specified bitbase. */
int getBitBaseIndexRange( int id );

void generateBitbases();

#endif // BITBASE_H_
