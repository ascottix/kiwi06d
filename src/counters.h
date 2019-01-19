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
#ifndef COUNTERS_H_
#define COUNTERS_H_

#include <stdio.h>

struct Counters
{
    static unsigned callsToGenMoves;
    static unsigned callsToSideInCheck;
    static unsigned callsToEvaluation;

    static unsigned nullMoveAttempts;
    static unsigned nullMoveCutOffs;

    static unsigned posGenerated;
    static unsigned posInvalid;
    static unsigned posSearched;

    static unsigned firstFailedHigh;
    static unsigned secondFailedHigh;
    static unsigned anyFailedHigh;

    static unsigned pawnHashProbes;
    static unsigned pawnHashProbesFailed;
    static unsigned pawnHashStores;

    static unsigned hashStores;
    static unsigned hashProbes;
    static unsigned hashProbesFailed;

    static unsigned miscCounter1;
    static unsigned miscCounter2;

    static void reset();

    static void dump();
};

#endif // COUNTERS_H_
