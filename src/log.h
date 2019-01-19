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
#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#include "platform.h"

enum {
    log_All = 0,
    log_Info,
    log_Warning,
    log_Error,
    log_Nothing
};

const int LogLevel  = log_Warning;

#define LOG( s )            if( LogLevel <= log_Info ) Log::write s
#define LOG_WARNING( s )    if( LogLevel <= log_Warning ) Log::write s
#define LOG_ERROR( s )      if( LogLevel <= log_Error ) Log::write s

class Log
{
public:
    static void CDECL write( const char * format, ... );
    
    static void assign( const char * name );
    
    static void flush();
    
    static void close();
    
    static FILE * file() {
        return logfile_ != 0 ? logfile_ : stdout;
    }
    
private:
    static FILE *   logfile_;
};

#endif // LOG_H_
