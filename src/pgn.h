/*
    Kiwi
    PGN (Portable Game Notation) handling

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
#ifndef PGN_H_
#define PGN_H_

#include "position.h"
#include "string.hxx"

class PGNLex;

struct PGN
{
    enum Result
    {
        WhiteWins   = +1,
        BlackWins   = -1,
        Drawn       =  0,
        Unknown     =  9
    };

    static const char * TagBlack;
    static const char * TagBlackElo;
    static const char * TagDate;
    static const char * TagEvent;
    static const char * TagFEN;
    static const char * TagResult;
    static const char * TagRound;
    static const char * TagSite;
    static const char * TagWhite;
    static const char * TagWhiteElo;
};

struct PGNMove
{
    char from_;
    char to_;
    char promotion_;
    unsigned char nag_;
    String * comment_;
};

struct PGNTag
{
    String name_;
    String value_;
    PGNTag * next_;
};

class PGNMoveList
{
public:
    PGNMoveList();

    ~PGNMoveList();

    void clear() {
        length_ = 0;
    }

    void add( char from, char to, char promotion, char nag );

    PGNMove & move( int index ) const;

    int getMove( int index, PGNMove & move ) const;

    int length() const {
        return length_;
    }

private:
    PGNMove * list_;
    int length_;
    int max_length_;
};

class PGNGame
{
public:
    PGNGame();

    ~PGNGame();

    void clear();

    int loadFromFile( PGNLex * lex );

    int saveToFile( FILE * file );

    int saveToFile( const char * fileName, bool append = true );

    const char * getTag( const char * tag );

    void setTag( const char * tag, const char * value );

    void clearTagList();

    PGN::Result getResult() const {
        return result_;
    }

    void setResult( PGN::Result result ) {
        result_ = result;
    }

    const Position & getStartPosition() const {
        return startPosition_;
    }

    void setStartPosition( const char * fen );    

    const PGNMoveList & getMoveList() const {
        return movelist_;
    }

private:
    int parseHeader( PGNLex * lex );
    int parseMoveList( PGNLex * lex, const Position & pos, PGNMoveList * list );

    Position    startPosition_;
    PGNTag *    tags_;
    PGN::Result result_;
    PGNMoveList movelist_;
};

#endif // PGN_H_
