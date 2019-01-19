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
#ifndef COMMAND_H_
#define COMMAND_H_

#include "string.hxx"

enum {
    cmd_Null = 0,
    cmd_DisplaySearchStatus,
    cmd_EnterAnalyzeMode,
    cmd_Force,
    cmd_Go,
    cmd_GoPlayOther,
    cmd_HideThinking,
    cmd_LeaveAnalyzeMode,
    cmd_MoveNow,
    cmd_New,
    cmd_OpponentMoves,
    cmd_OpponentOffersDraw,
    cmd_Result,
    cmd_SetBoard,
    cmd_SetClock,
    cmd_SetFixedDepth,
    cmd_SetFixedTime,
    cmd_SetLevel,
    cmd_SetOpponentClock,
    cmd_SetOpponentIsComputer,
    cmd_SetPonderingOff,
    cmd_SetPonderingOn,
    cmd_ShowBook,
    cmd_ShowHint,
    cmd_ShowThinking,
    cmd_Quit,
    cmd_Ping,
    cmd_UndoLastFullMove,
    cmd_UndoLastHalfMove,

    // Kiwi extensions
    cmd_KiwiAddToBookTree,
    cmd_KiwiBestMove,
    cmd_KiwiEvaluateSuite,
    cmd_KiwiExportBookTree,
    cmd_KiwiGenBB,
    cmd_KiwiLoadBook,
    cmd_KiwiHelp,
    cmd_KiwiPerft,
    cmd_KiwiRunSuite,
    cmd_KiwiSetOption,
    cmd_KiwiTest,
    cmd_KiwiAnalyze,

    res_WhiteWins,
    res_BlackWins,
    res_Draw,
    res_Unknown,
};

class Command
{
public:
    Command();

    int code() const {
        return code_;
    }

    void setCode( int code ) {
        code_ = code;
    }

    int intParam( int index, int def = -1 ) const;

    int intParamCount() const {
        return int_index_;
    }

    const char * strParam( int index ) const;

    int strParamCount() const {
        return str_index_;
    }

    void addIntParam( int param ) {
        int_param_[ int_index_++ ] = param;
    }

    void addStrParam( const String & str ) {
        str_param_[ str_index_++ ] = str;
    }

    void clear();

private:
    int code_;
    int int_index_;
    int str_index_;
    int     int_param_[9];
    String  str_param_[4];
};

#endif // COMMAND_H_
