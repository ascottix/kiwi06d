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
#include <stdarg.h>

#include "log.h"

FILE *  Log::logfile_ = 0;

void Log::assign( const char * name )
{
    FILE * f = (name == 0) ? stdout : fopen( name, "w" );
    
    if( f != 0 && f != logfile_ ) {
        close();
        logfile_ = f;
    }
}

void Log::flush()
{
    if( logfile_ != 0 ) {
        fflush( logfile_ );
    }
}

void Log::close()
{
    if( logfile_ != 0 ) {
        fclose( logfile_ );
        logfile_ = 0;
    }
}

void CDECL Log::write( const char * format, ... )
{
    va_list vl;
    
    va_start( vl, format );
    
    vfprintf( file(), format, vl );
    
    va_end( vl );

    flush();
}
