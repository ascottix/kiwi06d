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
#ifndef BITBASE_ADJUST_H_
#define BITBASE_ADJUST_H_

#include "packed_array.h"

enum {
    AdjustBb_Encode,
    AdjustBb_Decode
};

/*
    These functions are used to encode/decode bitbase before/after streaming
    to disk. 
    The goal is to optimize compression by having zeroes in as many bits as
    possible, so what happens is that we try to predict the value of each entry
    and then replace the entry with a bit that states whether the prediction
    was correct or not. So if the predictions are accurate we get large portions
    of the bitbase set to the same value, which helps compression.
*/

int bbAdjustKPPK( PackedArray * pa, int id, int wtm, int op );

int bbAdjustKBPK( PackedArray * pa, int id, int wtm, int op );

int bbAdjustKBNK( PackedArray * pa, int id, int wtm, int op );

#endif // BITBASE_ADJUST_H_
