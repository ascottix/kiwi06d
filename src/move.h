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
#ifndef MOVE_H_
#define MOVE_H_

/**
    Contains information about a move.

    Information is position-independent and not checked for validity.
    It includes:
    - the square the piece is moved from;
    - the square the piece is moved to;
    - the piece that is moved;
    - the piece currently on the destination square;
    - for pawns moved to the 8th rank: the promotion piece.

    Information is bit-packed into a single integer according
    to the following table. Note that a move is only required to hold
    the source square, the destination square and the promoted piece
    if any (other fields may be set while the move is being
    processed in order to help other operations).

    Offset  Size    Mask        Field
    ------  ----    ----        -----
    0       6       0x0000003F  from
    6       6       0x00000FC0  to
    12      4       0x0000F000  piece promoted (if move is a promotion)
    16      4       0x000F0000  piece captured (if move is a capture)
    20      4       0x00F00000  piece moved
    24      1       0x01000000  en-passant flag
    25-31   7       0xFE000000  reserved: must NOT be used!
*/
class Move
{
public:
    enum { 
        Null = 0
    };

    /** 
        Default constructor.

        For performance reasons, the default constructor does nothing
        and leaves the object in an undefined state.
    */
    Move() {
    }

    /**
        Copy constructor.
    */
    Move( const Move & m ) {
        data = m.data;
    }

    /**
        Creates a move from information packed in an unsigned integer.
    */
    Move( unsigned m ) {
        data = m;
    }

    /**
        Creates a move that simply moves a piece from a square to another,
        no check is performed.
    */
    Move( int from, int to ) {
        assign( from, to );
    }

    /**
        Creates a move that moves a pawn from a square to another and
        promotes it. No check is performed.
    */
    Move( int from, int to, int promoted ) {
        assign( from, to, promoted );
    }

    /**
        Assignment operator.
    */
    void operator = ( const Move & m ) {
        data = m.data;
    }

    /**
        Assignment operator.
    */
    void operator = ( unsigned d ) {
        data = d;
    }

    /**
        Returns true if this move is same as another.
    */
    bool operator == ( unsigned m ) const {
        return (data & 0xFFFF) == m;
    }

    bool operator != ( unsigned m ) const {
        return (data & 0xFFFF) != m;
    }

    bool operator == ( const Move & m ) const {
        return (data & 0xFFFF) == (m.data & 0xFFFF);
    }

    bool operator != ( const Move & m ) const {
        return (data & 0xFFFF) != (m.data & 0xFFFF);
    }

    //
    void assign( int from, int to ) {
        data = from | (to << 6);
    }

    void assign( int from, int to, int promoted ) {
        data = from | (to << 6) | (promoted << 12);
    }

    /** 
        Returns the starting square of this move.
    */
    int getFrom() const {
        return data & 0x3F;
    }

    /** 
        Returns the destination square of this move.
    */
    int getTo() const {
        return (data >>  6) & 0x3F;
    }

    /** 
        If the move is a capture, returns the captured piece.
    */
    int getCaptured() const {
        return (data >> 16) & 0x0F;
    }

    /** 
        Returns the piece moved.
    */
    int getMoved() const {
        return (data >> 20) & 0x0F;
    }

    /** 
        If the move is a promotion, returns the piece the pawn was promoted to.
    */
    int getPromoted() const {
        return (data >> 12) & 0x0F;
    }

    /**
        Returns non-zero if the move is an en-passant capture.
    */
    int getEnPassant() const {
        return data & 0x01000000;
    }

    /**
        Returns non-zero if the move is a capture.
    */
    int isCapture() const {
        return data & 0x000F0000;
    }

    /**
        Returns non-zero if the move is a promotion.
    */
    int isPromotion() const {
        return data & 0x0000F000;
    }

    /**
        Returns non-zero if the move is a capture or a promotion.
    */
    int isCaptureOrPromotion() const {
        return data & 0x000FF000;
    }

    /**
        If the move is a capture, sets the captured piece.
    */
    void setCaptured( int piece ) {
        data = (data & ~0x000F0000 ) | (piece << 16);
    }

    /**
        Sets the moved piece.
    */
    void setMoved( int piece ) {
        data = (data & ~0x00F00000 ) | (piece << 20);
    }

    /**
        If the move is a promotion, sets the piece the pawn was promoted to.
    */
    void setPromoted( int piece ) {
        data = (data & ~0x0000F000 ) | (piece << 12);
    }

    /**
        Tags the move as an en-passant capture.
    */
    void setEnPassant() {
        data |= 0x01000000;
    }

    /**
        Returns the move as an unsigned integer that contains only
        information about the start and destination squares.
    */
    unsigned toUint12() const {
        return data & 0xFFF;
    }

    /**
        Returns the move as an unsigned integer that contains
        information about the start and destination squares, and
        the promotion piece if any.
    */
    unsigned toUint16() const {
        return data & 0xFFFF;
    }

    /**
        Returns the move as an unsigned integer that contains
        information about the start and destination squares, the
        promotion piece (if any) and the captured piece (if any).
    */
    unsigned toUint24() const {
        return data & 0xFFFFFF;
    }

    /**
        Returns the move as an unsigned integer.

        All information is preserved.
    */
    unsigned toUnsigned() const {
        return data;
    }

    /**
        Converts the move into a string of the form h7xh8=Q,
        where h7 is the starting square and h8 is the destination square.
        
        If the move is a capture then a 'x' character is added between the
        start and destination squares. 

        If the move is a promotion, then the suffix '=' followed by the promotion
        piece is added.

        Note: if the specified buffer is a null pointer, an internal buffer is 
        used (this would not be good for multithreaded access).

        @return the specified output buffer
    */
    char * toString( char * buf = 0 ) const;

    /**
        Debugging only: dumps the move to the log file.
    */
    void dump() const;

private:
    unsigned    data;
};

#endif // MOVE_H_
