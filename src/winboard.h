/*
    Kiwi
    XBoard/WinBoard adapter

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
#ifndef WINBOARD_H_
#define WINBOARD_H_

#include "adapter.h"

class WinBoardAdapter : public Adapter
{
public:
    WinBoardAdapter();

    virtual ~WinBoardAdapter();

    virtual bool getNextCommand( Command & command );

    virtual void sendError( const char * what, const char * why );

    virtual void sendHint( const Position & pos, Move move );

    virtual void sendResult( int result, const char * why );

    virtual void resignGame( const Position & pos, const char * why );

    virtual void showThinking( const Position & pos, const MoveInfo & move );

    virtual void showCurrentSearchMove( const Position & pos, const RootMoveStat & move );

    virtual void rejectMove( const char * move, const char * why );

    virtual void playMove( const Position & pos, Move move );

    virtual void answerToPing( const Command & ping );

private:
    int parseCommand( const char * line, Command & command );
};

#endif // WINBOARD_H_
