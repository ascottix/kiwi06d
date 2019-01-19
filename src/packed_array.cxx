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
#include <stdio.h>
#include <stdlib.h>

#include "packed_array.h"

PackedArray::PackedArray( unsigned numberOfElements, unsigned bitsPerElement )
{
    assert( numberOfElements > 0 );

    data_ = 0;
    max_index_ = numberOfElements;
    bits_per_elem_ = bitsPerElement;

    switch( bitsPerElement ) {
    case 1:
        bit_shift_ = 3;
        bit_and_ = 0x07;
        bit_and_shift_ = 0;
        bit_mask_ = 0x01;
        break;
    case 2:
        bit_shift_ = 2;
        bit_and_ = 0x03;
        bit_and_shift_ = 1;
        bit_mask_ = 0x03;
        break;
    case 4:
        bit_shift_ = 3;
        bit_and_ = 0x01;
        bit_and_shift_ = 2;
        bit_mask_ = 0x0F;
        break;
    default:
        assert( 0 );
        break;
    }

    data_size_ = (numberOfElements * bitsPerElement + 7) / 8;
    data_ = new unsigned char [ data_size_ ];
}

PackedArray::PackedArray( const PackedArray & a )
{
    data_ = new unsigned char [ a.data_size_ ];
    data_size_ = a.data_size_;
    max_index_ = a.max_index_;
    bits_per_elem_ = a.bits_per_elem_;
    bit_shift_ = a.bit_shift_;
    bit_and_ = a.bit_and_;
    bit_and_shift_ = a.bit_and_shift_;
    bit_mask_ = a.bit_mask_;

    memcpy( data_, a.data_, data_size_ );
}

PackedArray::~PackedArray()
{
    delete [] data_;
}
