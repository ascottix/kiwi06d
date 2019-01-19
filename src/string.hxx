/*
    Kiwi

    Note: extension .hxx is used for this file to avoid conflicts with
    string.h from the standard C library.

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
#ifndef STRING_H_
#define STRING_H_

class String
{
public:
    String();

    String( const char * s );

    String( const String & s );

    ~String();

    String & operator = ( const char * s );

    String & operator = ( const String & s ) {
        return operator = ( s.cstr() );
    }

    void clear();

    void deleteChar( int index );

    void trim();

    void operator += ( char c );

    void operator += ( const char * s );

    char operator [] ( int index ) const;

    bool operator == ( const char * s );

    bool isNull() const {
        return buf_ == 0;
    }

    bool isEmpty() const {
        return isNull() || (len_ == 0);
    }

    int length() const {
        return len_;
    }

    const char * cstr() const {
        return buf_;
    }

private:
    void reserve( int size );

    char *  buf_;
    int     bufsize_;
    int     len_;
};

#endif // STRING_H_
