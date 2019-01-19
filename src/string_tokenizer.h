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
#ifndef STRING_TOKENIZER_H_
#define STRING_TOKENIZER_H_

#include "string.hxx"

class StringTokenizer
{
public:
    StringTokenizer( const String & string );

    StringTokenizer( const char * string );

    bool hasMoreTokens() const;

    bool getNextToken( String & token );

    bool getRemainingTokens( String & tokens );

private:
    void skipBlanks();

    int index_;
    String string_;
};

#endif // STRING_TOKENIZER_H_
