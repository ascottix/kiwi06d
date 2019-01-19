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
#ifndef MOVE_HANDLER_H_
#define MOVE_HANDLER_H_

#include "bitboard.h"
#include "engine.h"
#include "move.h"
#include "movelist.h"
#include "position.h"

enum GenerationMode {
    GenerateNothing,
    GenerateForSearch,
    GenerateForQuiesce,
};

class MoveHandler
{
public:
    MoveHandler( const Position & pos, int ply, int mode, Move hashMove = Move::Null );

    int getNextMove( Move & m );

    MoveList & getDiscardedMoves() {
        return discardedMoves_;
    }

    void restart( Move hashMove );

    static void resetHistoryTable();
    static void updateHistoryTable( int side, const Move & m, int delta );

    static void resetKillerTable();
    static void addToKillerTable( const Move & m, int ply );

    enum {
        MaxKiller   = 400 + Engine::MaxSearchPly    // Max search depth plus some buffer for quiesce nodes
    };

    static bool isKillerMove( const Move & m, int ply ) {
        return m == tableKiller1[ply] || m == tableKiller2[ply];
    }

    static int  tableHistoryBlack[64*64];
    static int  tableHistoryWhite[64*64];
    static Move tableKiller1[MaxKiller];
    static Move tableKiller2[MaxKiller];

private:
    enum State {
        StateDone,
        StateTryHashMove,
        StateGenerateMoves,
        StateReadNextMove,
    };

    int                 ply_;
    const Position &    pos_;
    int                 mode_;
    Move                hashMove_;
    Move                move_[ MoveList::MaxMoveCount ];
    int                 moveWeight_[ MoveList::MaxMoveCount ];
    int                 moveIndex_;
    int                 moveCount_;
    State               state_;
    int *               historyTable_;
    MoveList            discardedMoves_;
};

#endif // MOVE_HANDLER_H_
