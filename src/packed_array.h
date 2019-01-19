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
#ifndef PACKED_ARRAY_H_
#define PACKED_ARRAY_H_

#include <assert.h>
#include <string.h>

class PackedArray
{
public:
    PackedArray( unsigned numberOfElements, unsigned bitsPerElement = 1 );

    PackedArray( const PackedArray & a );

    ~PackedArray();

    // Resets all elements to zero
    void clear() {
        memset( data_, 0, data_size_ );
    }

    // Sets all bits to one
    void setAll() {
        memset( data_, 0xFF, data_size_ );
    }

    // Clears the specified element
    void clr( unsigned index ) {
        assert( index < max_index_ );

        data_[ index >> bit_shift_ ] &= ~(bit_mask_ << ((index & bit_and_) << bit_and_shift_));
    }

    // Sets the specified element to a non-zero value (precise value is unknown)
    void set( unsigned index ) {
        assert( index < max_index_ );

        data_[ index >> bit_shift_ ] |= 1 << ((index & bit_and_) << bit_and_shift_);
    }

    // Sets the specified element value
    void set( unsigned index, unsigned value ) {
        assert( index < max_index_ );
        assert( (value & ~bit_mask_) == 0 );

        data_[ index >> bit_shift_ ] &= ~(bit_mask_ << ((index & bit_and_) << bit_and_shift_));
        data_[ index >> bit_shift_ ] |= value << ((index & bit_and_) << bit_and_shift_);
    }

    // Returns the specified element
    unsigned get( unsigned index ) const {
        assert( index < max_index_ );

        return (data_[ index >> bit_shift_ ] >> ((index & bit_and_) << bit_and_shift_)) & bit_mask_;
    }

    // Returns the number of element in the array
    unsigned getNumberOfElements() const {
        return max_index_;
    }

    // Returns the number of bits per element
    unsigned getBitsPerElement() const {
        return bits_per_elem_;
    }

    const unsigned char * getRawData() const {
        return data_;
    }

    unsigned getRawDataSize() const {
        return data_size_;
    }

    unsigned char * getRawDataWriteable() {
        return data_;
    }

private:
    unsigned char * data_;
    unsigned data_size_;
    unsigned max_index_;
    unsigned bits_per_elem_;
    unsigned bit_shift_;
    unsigned bit_and_;
    unsigned bit_and_shift_;
    unsigned bit_mask_;
};

#endif // PACKED_ARRAY_H_
