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
#ifndef RECOGNIZER_H_
#define RECOGNIZER_H_

#include "position.h"
#include "score.h"

#define HAVE_RECOGNIZER_STATS

enum {
    rtUnknown,
    rtLowerBound,
    rtUpperBound,
    rtExact,
};

/*
    Recognizers are based on the paper "Efficient Interior-Node Recognition"
    by Ernst A. Heinz, published in the ICCA Journal 21, Sep. 1998.

    A preprint of this paper is freely available from Heinz's site.
*/

class RecognizerInfo {
public:
    RecognizerInfo() {
        type_ = rtUnknown;
    }

    void set( int value, int type ) {
        value_ = value;
        type_ = type;
    }

    /**
        Negates the current score.

        Note: if score is a lower or upper bound, the bound is inverted too.
    */
    void negate() {
        value_ = -value_;

        if( type_ == rtLowerBound ) {
            type_ = rtUpperBound;
        }
        else if( type_ == rtUpperBound ) {
            type_ = rtLowerBound;
        }
    }

    /**
        Adjusts the current score for the specified ply.

        If the score is an exact mate or mated in n (relative to the recognized position),
        it is adjusted for the specified ply.

        @return the updated score
    */
    int adjust( int ply ) {
        if( type_ == rtExact ) {
            if( value_ < Score::MateLo ) {
                value_ += ply;
            }
            else if( value_ > Score::MateHi ) {
                value_ -= ply;
            }
        }

        return value_;
    }

    int value() const {
        return value_;
    }

    int type() const {
        return type_;
    }

private:
    int value_;
    int type_;
};

typedef bool (* RecognizerHandler) ( const Position & pos, RecognizerInfo & result );

class Recognizer {
public:
    static void initialize();

    /*
        Looks if the current position can be handled by some recognizer.

        Note: returned scores are always *relative* to the side on move.

        @param  pos position to look for
        @param  result (out) position evaluation (if recognition was successful)

        @return true if the position was recognized and handled, false
                otherwise.
    */
    static inline bool probe( const Position & pos, RecognizerInfo & result ) {
        if( knownHandlersSignature_ & (1 << pos.getShortMaterialSignature()) ) {
            unsigned sig = pos.getMaterialSignature();
            if( knownHandlers_[ sig >> 3 ] & (1 << (sig & 7)) ) {
#ifdef HAVE_RECOGNIZER_STATS
                stats_[ sig ].probes_++;
                bool handled = handler_[ sig ] ( pos, result );
                if( handled ) {
                    stats_[ sig ].hits_++;
                }
                return handled;
#else
                return handler_[ sig ] ( pos, result );
#endif
            }
        }

        return false;
    }

    static bool clearStats();

    static bool dumpStats();

private:
    enum {
        SignatureBits = 10  // Can only change according to how position signature is computed
    };

#ifdef HAVE_RECOGNIZER_STATS
    struct Stats {
        unsigned    probes_;    // How many times a recognizer has been invoked
        unsigned    hits_;      // How many times a recognizer successfully handled a position
    };

    static  Stats               stats_[ 1 << SignatureBits ];
#endif

    Recognizer();

    static void registerHandler( unsigned signature, RecognizerHandler handler );

    static unsigned             knownHandlersSignature_;
    static unsigned char        knownHandlers_[ (1 << SignatureBits) / 8 ]; // Signature takes 10 bits
    static RecognizerHandler    handler_[ 1 << SignatureBits ];
};

#endif // RECOGNIZER_H_
