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
#include <string.h>

#include "board.h"
#include "log.h"
#include "move.h"

char * Move::toString( char * buf ) const 
{
    static char defaultBuffer[8];

    if( buf == 0 ) {
        buf = defaultBuffer;
    }

    if( operator == (Null) ) {
        // This is a null move
        strcpy( buf, "null" );
    }
    else {
        char * dest = buf;

        Board::getSquareName( dest, getFrom() );
        dest += 2;

        if( getCaptured() != 0 ) {
            *dest++ = 'x';
        }

        Board::getSquareName( dest, getTo() );
        dest += 2;

        if( getPromoted() != 0 ) {
            *dest++ = '=';
            *dest++ = "?PNBRQK??PNBRQK"[ getPromoted() ];
        }

        *dest = '\0';
    }

    return buf;
}

void Move::dump() const
{
    FILE * f = Log::file();

    char buf[16];

    toString( buf );

    fprintf( f, buf );
}
