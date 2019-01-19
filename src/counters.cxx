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
#include "counters.h"
#include "log.h"

unsigned Counters::callsToGenMoves      = 0;
unsigned Counters::callsToSideInCheck   = 0;
unsigned Counters::callsToEvaluation    = 0;

unsigned Counters::posGenerated         = 0;
unsigned Counters::posInvalid           = 0;
unsigned Counters::posSearched          = 0;

unsigned Counters::pawnHashProbes       = 0;
unsigned Counters::pawnHashProbesFailed = 0;
unsigned Counters::pawnHashStores       = 0;

unsigned Counters::hashStores           = 0;
unsigned Counters::hashProbes           = 0;
unsigned Counters::hashProbesFailed     = 0;

unsigned Counters::firstFailedHigh      = 0;
unsigned Counters::secondFailedHigh     = 0;
unsigned Counters::anyFailedHigh        = 0;

unsigned Counters::nullMoveAttempts     = 0;
unsigned Counters::nullMoveCutOffs      = 0;

unsigned Counters::miscCounter1 = 0;
unsigned Counters::miscCounter2 = 0;

void Counters::reset()
{
    callsToGenMoves      = 0;
    callsToSideInCheck   = 0;
    callsToEvaluation    = 0;

    posGenerated         = 0;
    posInvalid           = 0;
    posSearched          = 0;
    
    pawnHashProbes       = 0;
    pawnHashProbesFailed = 0;
    pawnHashStores       = 0;

    hashStores           = 0;
    hashProbes           = 0;
    hashProbesFailed     = 0;

    firstFailedHigh      = 0;
    secondFailedHigh     = 0;
    anyFailedHigh        = 0;

    nullMoveAttempts     = 0;
    nullMoveCutOffs      = 0;
}

void Counters::dump()
{
    FILE * f = Log::file();

    fprintf( f, "Counters\n" );
    fprintf( f, "--------\n" );
    fprintf( f, "Calls to GenMoves      : %u\n", callsToGenMoves );
    fprintf( f, "Calls to SideInCheck   : %u\n", callsToSideInCheck );
    fprintf( f, "Calls to Evaluation    : %u\n", callsToEvaluation );

    if( nullMoveAttempts > 0 ) {
        double f1 = (nullMoveCutOffs * 100.0) / nullMoveAttempts;

        fprintf( f, "Null move cutoffs      : %u / %u (%05.2f%%)\n", nullMoveCutOffs, nullMoveAttempts, f1 );
    }

    fprintf( f, "Positions generated    : %u\n", posGenerated );
    fprintf( f, "Invalid moves generated: %u\n", posInvalid );
    fprintf( f, "Positions searched     : %u\n", posSearched );
    fprintf( f, "Hash probes failed     : %u / %u\n", hashProbesFailed, hashProbes );
    fprintf( f, "Pawn hash probes failed: %u / %u\n", pawnHashProbesFailed, pawnHashProbes );
    fprintf( f, "Pawn hash stores       : %u\n", pawnHashStores );

    if( anyFailedHigh > 0 ) {
        double f1 = (firstFailedHigh * 100.0) / anyFailedHigh;
        double f2 = (secondFailedHigh * 100.0) / anyFailedHigh;

        fprintf( f, "Failed high at 1st try : %05.2f%%\n", f1 );
        fprintf( f, "Failed high at 2nd try : %05.2f%%\n", f2 );
    }

    fprintf( f, "Misc. counter #1       : %u\n", miscCounter1 );
    fprintf( f, "Misc. counter #2       : %u\n", miscCounter2 );
}
