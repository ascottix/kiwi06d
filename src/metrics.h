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
#ifndef METRICS_H_
#define METRICS_H_

#include "board.h"

/** Returns the max value between a and b. */
inline int imax( int a, int b )
{
    return a > b ? a : b;
}

/** Returns the min value between a and b. */
inline int imin( int a, int b )
{
    return a < b ? a : b;
}

/** Returns the absolute value of a. */
inline int iabs( int a )
{
    return a < 0 ? -a : +a;
}

/** Swaps a and b. */
inline void iswap( int & a, int & b )
{
    a ^= b;
    b ^= a;
    a ^= b;
}

/** Computes the linear distance between a and b. */
inline int linear_distance( int a, int b )
{
    return a > b ? a-b : b-a;
}

/** Computes the distance, in king steps, between squares sq1 and sq2. */
inline int distance( int sq1, int sq2 )
{
    return imax( linear_distance(FileOfSquare(sq1),FileOfSquare(sq2)), linear_distance(RankOfSquare(sq1),RankOfSquare(sq2)) );
}

/** Flips the specified square with respect to the board center rank. */
inline int flip_rank( int sq )
{
    return FileRankToSquare( FileOfSquare(sq), 7 - RankOfSquare(sq) );
}

/** Flips the specified square with respect to the board center file. */
inline int flip_file( int sq )
{
    return FileRankToSquare( 7 - FileOfSquare(sq), RankOfSquare(sq) );
}

/** Flips the specified square with respect to the A1-H8 diagonal. */
inline int flip_a1h8( int sq )
{
    return FileRankToSquare( RankOfSquare(sq), FileOfSquare(sq) );
}

/** Flips the specified square with respect to the A8-H1 diagonal. */
inline int flip_a8h1( int sq )
{
    return FileRankToSquare( 7 - RankOfSquare(sq), 7 - FileOfSquare(sq) );
}

/** Flips the specified square with respect to the specified reference file. */
inline int flip_file_relative( int sq, int reference )
{
    return FileRankToSquare( 2*reference - FileOfSquare(sq), RankOfSquare(sq) );
}

/** Returns 1 if the square is light, 0 if dark. */
inline int color_of( int sq ) {
    return (sq + (sq >> 3)) & 1;
}

inline int is_light_square( int sq ) {
    return color_of( sq );
}

/** Returns the distance from the nearest edge. */
inline int edge_distance( int sq ) {
    return imin( FileOfSquare(sq), imin( 7-FileOfSquare(sq), imin( RankOfSquare(sq), 7-RankOfSquare(sq) ) ) );
}

/** Returns the distance from the nearest corner. */
inline int corner_distance( int sq )
{
    return imin( distance(A1,sq), imin( distance(A8,sq), imin( distance(H1,sq), distance(H8,sq) ) ) );
}

/** Returns the distance from the nearest corner of specified color. */
inline int b_corner_distance( int sq, int color )
{
    return color ? imin( distance(H1,sq), distance(A8,sq) ) : imin( distance(A1,sq), distance(H8,sq) );
}

/** Returns true if the square is on the edge of the board, false otherwise. */
inline bool is_edge_square( int sq ) {
    return (FileOfSquare(sq) == 0) || (FileOfSquare(sq) == 7) || (RankOfSquare(sq) == 0) || (RankOfSquare(sq) == 7);
}

/** If needed, brings the white king in the left half of the board, flipping all pieces. */
inline void mirror_wk( int & wk, int & bk, int & p1 )
{
    if( FileOfSquare(wk) >= 4 ) {
        wk = flip_file(wk);
        bk = flip_file(bk);
        p1 = flip_file(p1);
    }
}

/** If needed, brings the white king in the left half of the board, flipping all pieces. */
inline void mirror_wk( int & wk, int & bk, int & p1, int & p2 )
{
    if( FileOfSquare(wk) >= 4 ) {
        wk = flip_file(wk);
        bk = flip_file(bk);
        p1 = flip_file(p1);
        p2 = flip_file(p2);
    }
}

#endif // METRICS_H_
