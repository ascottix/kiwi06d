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

#include "engine.h"
#include "platform.h"
#include "winboard.h"

int CDECL main( int argc, char * argv[] )
{
    for( int i=1; i<argc; i++ ) {
        if( strcmp( argv[i], "/xboard" ) == 0 ) {
            printf( "feature done=0\n" ); fflush( stdout );
        }
    }

    WinBoardAdapter * adapter = new WinBoardAdapter();

    Engine::main( adapter );

    delete adapter;

    return 0;
}
