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
#include "book.h"

#include "log.h"
#include "movelist.h"
#include "pgn_lex.h"
#include "system.h"

BookNode::BookNode( const Position & pos )
{
    hashCode = pos.relativeHashCode();
    count = 1;
    flags = 0;
    prev = 0;
    next = 0;
}

BookTree::BookTree()
{
    root = 0;
}

BookTree::~BookTree()
{
    freeNode( root );
}

void BookTree::freeNode( BookNode * node )
{
    if( node != 0 ) {
        freeNode( node->prev );
        freeNode( node->next );
        delete node;
    }
}

BookNode * BookTree::insertNode( const Position & pos )
{
    BookNode * result = 0;
    BitBoard posHashCode = pos.relativeHashCode();

    if( root == 0 ) {
        result = new BookNode( pos );

        root = result;
    }
    else {
        result = root;

        while( true ) {
            BookNode * curr = result;

            if( result->hashCode == posHashCode ) {
                // Found
                curr->count++;
                break;
            }
            else if( curr->hashCode > posHashCode ) {
                result = curr->prev;

                if( result == 0 ) {
                    result = new BookNode( pos );
                    curr->prev = result;
                    break;
                }
            }
            else {
                result = curr->next;

                if( result == 0 ) {
                    result = new BookNode( pos );
                    curr->next = result;
                    break;
                }
            }
        }
    }

    return result;
}

unsigned BookTree::countPositions( BookNode * node, int minCount )
{
    unsigned result = 0;

    if( node != 0 ) {
        result += countPositions( node->prev, minCount );

        if( node->count >= minCount ) {
            result++;
        }

        result += countPositions( node->next, minCount );
    }

    return result;
}

void BookTree::exportTree( BookNode * node, int minCount, unsigned shiftCount, FILE * f )
{
    if( node != 0 ) {
        exportTree( node->prev, minCount, shiftCount, f );

        if( node->count >= minCount ) {
            Book::Entry entry;

            entry.hashCode = node->hashCode;
            entry.count = (unsigned short) (node->count >> shiftCount);
            if( entry.count == 0 ) {
                entry.count++;
            }

            fwrite( &entry, sizeof(entry), 1, f );
        }

        exportTree( node->next, minCount, shiftCount, f );
    }
}

BookNode * BookTree::findPosition( const Position & pos )
{
    BookNode * result = root;
    BitBoard posHashCode = pos.relativeHashCode();

    while( result != 0 ) {
        if( result->hashCode == posHashCode ) {
            break;
        }

        if( result->hashCode > posHashCode ) {
            result = result->prev;
        }
        else {
            result = result->next;
        }
    }

    return result;
}

int BookTree::exportToBookFile( const char * fileName, int minCount )
{
    printf( "Book export to file: %s, min position count = %d\n",
        fileName, minCount );

    FILE * f = fopen( fileName, "wb" );

    if( f == 0 ) {
        printf( "Cannot create output file.\n" );
        return -1;
    }

    printf( "Saving..." );

    // Get number of positions and save it
    unsigned posCount = countPositions( root, minCount );

    fwrite( &posCount, sizeof(unsigned), 1, f );

    // Normalize position count so that eventually everything fits into 16 bits
    unsigned shiftCount = 0;
    unsigned normalizesPosCount = posCount;

    while( normalizesPosCount > 0x7FFF ) {
        normalizesPosCount >>= 1;
        shiftCount++;
    }

    // Save the tree
    exportTree( root, minCount, shiftCount, f );

    // Close and exit
    fclose( f );

    printf( " done, %u positions saved.\n", posCount );

    return 0;
}

Book::Book()
{
    entryCount_ = 0;
    entries_ = 0;
}

Book::~Book()
{
    delete [] entries_;
}

int Book::loadFromFile( const char * fileName )
{
    FILE * f = fopen( fileName, "rb" );

    if( f == 0 ) {
        return -1;
    }

    unsigned count;

    if( fread( &count, sizeof(int), 1, f ) != 1 ) {
        fclose( f );
        return -2;
    }

    Entry * list = new Entry [ count ];

    if( fread( list, sizeof(Entry), count, f ) != count ) {
        delete [] list;
        fclose( f );
        return -3;
    }

    delete [] entries_;

    entries_ = list;
    entryCount_ = count;

    Log::write( "Loaded book: %u entries\n", count );

    fclose( f );

    return 0;
}

Book::Entry * Book::lookup( Position & pos )
{
    Entry * result = 0;
    BitBoard posHashCode = pos.relativeHashCode();

    int lo = 0;
    int hi = entryCount_ - 1;

    while( (lo <= hi) && (result == 0) ) {
        int index = (lo + hi) / 2;

        if( entries_[ index ].hashCode == posHashCode ) {
            result = entries_ + index;
        }
        else if( entries_[ index ].hashCode < posHashCode ) {
            lo = index + 1;
        }
        else {
            hi = index - 1;
        }
    }

    return result;
}

void BookTree::addGame( const PGNGame & game, int numOfPlies )
{
    int moveCount = game.getMoveList().length();

    if( moveCount > numOfPlies ) {
        moveCount = numOfPlies;
    }

    Position pos( game.getStartPosition() );

    insertNode( pos );

    for( int i=0; i<moveCount; i++ ) {
        PGNMove pgnMove;

        game.getMoveList().getMove( i, pgnMove );

        Move move( pgnMove.from_, pgnMove.to_, pgnMove.promotion_ );

        pos.doMove( move );

        insertNode( pos );
    }
}

int BookTree::addGameCollection( const char * fileName, int minMovesPerGame, int numOfPlies )
{
    FILE * f = fopen( fileName, "rb" );

    if( f == 0 ) {
        printf( "Cannot open input file!\n" );
        return -1;
    }

    // Create and initialize the lexical analyzer for this file
    PGNLex lex( f );

    lex.getNextToken();
    
    // Start reading games and add them to the book tree
    printf( "Book import from: %s, min moves per game = %d, num of plies = %d\n",
        fileName, minMovesPerGame, numOfPlies );

    unsigned startTime = System::getTickCount();

    printf( "Loading..." );

    unsigned moveCount = 0;
    unsigned gameCount = 0;
    unsigned goodGameCount = 0;

    PGNGame g;

    while( true ) {
        if( g.loadFromFile( &lex ) != 0 ) {
            break;
        }

        int l = g.getMoveList().length();

        gameCount++;
        moveCount += (unsigned) l;

        if( (gameCount & 0x3FF) == 0 ) {
            printf( "." );
        }

        if( l >= minMovesPerGame ) {
            goodGameCount++;

            addGame( g, numOfPlies );
        }
    }

    unsigned totalTime = System::getTickCount() - startTime;

    printf( " done (%.1f seconds)\n", (double) totalTime / 1000.0 );

    fclose( f );

    printf( "Games loaded: %u (%u good for book), positions: %u.\n",
        gameCount,
        goodGameCount,
        moveCount );

    return 0;
}
