/*
    Kiwi
    Copyright (c) 2004 Alessandro Scotti
*/
#ifndef DISTANCE_H_
#define DISTANCE_H_

#include "board.h"

inline int iabs( int n ) 
{
    return (n >= 0) ? n : -n;
}

inline int imin( int a, int b )
{
    return (a <= b) ? a : b;
}

inline int imax( int a, int b )
{
    return (a >= b) ? a : b;
}

class Distance
{
public:
    static int king( int sq1, int sq2 ) {
        int filedist    = iabs( FileOfSquare(sq1) - FileOfSquare(sq2) );
        int rankdist    = iabs( RankOfSquare(sq1) - RankOfSquare(sq2) );

        return imax( filedist, rankdist );
    }

    static int taxicab( int sq1, int sq2 ) {
        int filedist    = iabs( FileOfSquare(sq1) - FileOfSquare(sq2) );
        int rankdist    = iabs( RankOfSquare(sq1) - RankOfSquare(sq2) );

        return filedist+rankdist;
    }

    static int fromNearestEdge( int sq ) {
        int f = FileOfSquare( sq );
        int r = RankOfSquare( sq );

        if( f > 3 ) f = 7-f;
        if( r > 3 ) r = 7-r;

        return imin( f, r );
    }

    static int fromSweetCenter( int sq ) {
        int f = FileOfSquare( sq );
        int r = RankOfSquare( sq );

        if( f > 3 ) f = 7-f;
        if( r > 3 ) r = 7-r;

        return 3-imax( f, r );
    }
};

#endif // DISTANCE_H_
