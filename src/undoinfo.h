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
#ifndef UNDOINFO_H_
#define UNDOINFO_H_

#include "bitboard.h"
#include "move.h"
#include "position.h"

/*
    Stores information useful to take back a played move.
*/
struct UndoInfo 
{
    UndoInfo( const Position & p ) {
        allPieces       = p.allPieces;
        allPiecesTA1H8  = p.allPiecesTA1H8;
        allPiecesDA1H8  = p.allPiecesDA1H8;
        allPiecesDA8H1  = p.allPiecesDA8H1;
        hashCode        = p.hashCode;
        pawnHashCode    = p.pawnHashCode;
        boardFlags      = p.boardFlags;
        matSignature    = p.materialSignature;
        pstScoreOpening = p.pstScoreOpening;
        pstScoreEndgame = p.pstScoreEndgame;
    }

    BitBoard    allPieces;
    BitBoard    allPiecesTA1H8;
    BitBoard    allPiecesDA1H8;
    BitBoard    allPiecesDA8H1;
    BitBoard    hashCode;
    BitBoard    pawnHashCode;
    unsigned    boardFlags;
    unsigned    matSignature;
    int         pstScoreOpening;
    int         pstScoreEndgame;
};

#endif // UNDOINFO_H_
