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
#include <ctype.h>
#include <string.h>

#include "string.hxx"

const int BufChunkSize  = 32;

String::String()
{
    buf_ = 0;
    len_ = 0;
    bufsize_ = 0;
}

String::String( const char * s )
{
    buf_ = 0;
    len_ = 0;
    bufsize_ = 0;

    operator = ( s );
}

String::String( const String & s )
{
    bufsize_ = s.bufsize_;
    buf_ = new char [ bufsize_ ];
    len_ = s.len_;

    memcpy( buf_, s.buf_, len_+1 );
}

String::~String()
{
    delete [] buf_;
}

void String::clear()
{
    if( buf_ != 0 ) {
        len_ = 0;
        *buf_ = '\0';
    }
}

void String::deleteChar( int index )
{
    if( (index >= 0) && (index < len_) ) {
        len_--;

        char * b = buf_ + index;

        while( index < len_ ) {
            b[0] = b[1];
            b++;
            index++;
        }

        *b = '\0';
    }
}

void String::trim()
{
    if( len_ > 0 ) {
        char * b = buf_;

        while( (*b != '\0') && isspace(*b) ) {
            b++;
        }

        char * e = buf_ + len_ - 1;

        while( (e > b) && isspace(*e) ) {
            e--;
        }

        if( len_ != e - b + 1 ) {
            len_ = 0;
            while( b <= e ) {
                buf_[ len_++ ] = *b++;
            }
            buf_[ len_ ] = 0;
        }
    }
}

String & String::operator = ( const char * s )
{
    if( s != 0 ) {
        int slen = (int) strlen( s );

        reserve( slen );

        strcpy( buf_, s );
        len_ = slen;
    }

    return *this;
}

void String::operator += ( char c )
{
    reserve( len_ + 1 );

    buf_[ len_ ] = c;

    len_++;

    buf_[ len_ ] = '\0';
}

void String::operator += ( const char * s )
{
    if( s != 0 ) {
        int slen = (int) strlen( s );

        reserve( len_ + slen );

        strcpy( buf_+len_, s );
        len_ += slen;
    }
}

char String::operator [] ( int index ) const
{
    return (index >= 0) && (index < len_) ? buf_[index] : '\0';
}

bool String::operator == ( const char * s )
{
    return (buf_ != 0) && (s != 0) && (strcmp(buf_,s) == 0);
}

void String::reserve( int size )
{
    size++; // Take into account the terminator character

    if( size > bufsize_ ) {
        // Buffer must be expanded: compute new size (multiple of the base chunk size)
        int newbufsize = BufChunkSize * (1 + (size / BufChunkSize));

        // Allocate new buffer
        char * newbuf = new char [ newbufsize ];

        // Copy old buffer content if needed
        if( buf_ != 0 ) {
            memcpy( newbuf, buf_, len_+1 );

            delete [] buf_;
        }

        // Replace old buffer
        buf_ = newbuf;
        bufsize_ = newbufsize;
    }
}
