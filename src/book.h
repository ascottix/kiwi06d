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
#ifndef BOOK_H_
#define BOOK_H_

#include "bitboard.h"
#include "pgn.h"
#include "position.h"

class Book
{
public:
    // This structure represents a book entry (in the final "compiled" book)
    struct Entry
    {
        BitBoard        hashCode;
        unsigned short  count;
        unsigned char   pad[6];
    };

    Book();

    ~Book();

    int loadFromFile( const char * fileName );

    Entry * lookup( Position & pos );

    unsigned entryCount() const {
        return entryCount_;
    }

private:
    unsigned    entryCount_;
    Entry *     entries_;
};

// This structure represents a board position while building the opening book
struct BookNode
{
    BookNode( const Position & pos );

    BitBoard        hashCode;   // Position hash code
    int             count;      // Number of times this position occurred
    unsigned char   flags;
    BookNode *      prev;
    BookNode *      next;
};

class BookTree
{
public:
    BookTree();

    ~BookTree();

    BookNode * insertNode( const Position & pos );

    void freeNode( BookNode * node );

    BookNode * findPosition( const Position & pos );

    int exportToBookFile( const char * fileName, int minCount );

    unsigned countPositions( int minCount ) {
        return countPositions( root, minCount );
    }

    void addGame( const PGNGame & game, int numOfPlies );

    int addGameCollection( const char * fileName, int minMovesPerGame, int numOfPlies );

private:
    unsigned countPositions( BookNode * node, int minCount );

    void exportTree( BookNode * node, int minCount, unsigned shiftCount, FILE * f );

    BookNode *  root;
};

#endif // BOOK_H_
