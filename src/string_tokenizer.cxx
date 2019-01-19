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

#include "string_tokenizer.h"

StringTokenizer::StringTokenizer( const String & string )
    : string_(string)
{
    index_ = 0;
    skipBlanks();
}

StringTokenizer::StringTokenizer( const char * string )
    : string_(string)
{
    index_ = 0;
    skipBlanks();
}

bool StringTokenizer::hasMoreTokens() const
{
    return index_ < string_.length();
}

bool StringTokenizer::getNextToken( String & token )
{
    bool result = false;

    if( index_ < string_.length() ) {
        token.clear();

        if( string_[index_] == '"' ) {
            // Quoted string
            index_++;

            while( true ) {
                if( index_ >= string_.length() ) {
                    break;
                }

                char c = string_[ index_++ ];

                if( c == '"' ) {
                    // End quotes (which may be itself quoted)
                    if( string_[index_] != '"' ) {
                        break;
                    }
                    else {
                        index_++;
                    }
                }

                token += c;
            }
        }
        else {
            // Standard parameter
            while( (index_ < string_.length()) && (! isspace(string_[index_])) ) {
                token += string_[ index_ ];
                index_++;
            }
        }

        // Skip blanks to next token
        skipBlanks();

        result = true;
    }

    return result;
}

bool StringTokenizer::getRemainingTokens( String & tokens )
{
    bool result = hasMoreTokens();

    if( result ) {
        tokens = string_.cstr() + index_;
    }

    return result;
}

void StringTokenizer::skipBlanks()
{
    while( (index_ < string_.length()) && isspace(string_[index_]) ) {
        index_++;
    }
}
