/*
    Kiwi
    Lexical analyzer for PGN files

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
#ifndef PGN_LEX_H_
#define PGN_LEX_H_

#include <stdio.h>

#include "string.hxx"

enum PGNTokenId {
    pgnTokenEOF,            // End of file
    pgnTokenOParen,
    pgnTokenCParen,
    pgnTokenOBracket,
    pgnTokenCBracket,
    pgnTokenOAngleBracket,
    pgnTokenCAngleBracket,
    pgnTokenNAG,
    pgnTokenPeriod,
    pgnTokenString,
    pgnTokenComment,        // Commentary text
    pgnTokenPragma,
    pgnTokenResult,
    pgnTokenSymbol,
    pgnTokenNumber,         // A symbol made entirely by digits
    pgnTokenUnknown,        // Unknown character
    pgnTokenNull,           // No token read from input
    pgnTokenError           // Parsing error
};

/**
    PGN lexical analyzer.

    Parses a PGN file into a series of tokens.
*/
class PGNLex
{
public:
    /**
        Creates a new PGN lexical analyzer associated to the specified stream.
    */
    PGNLex( FILE * f ) {
        inputStream_ = f;

        reset();
    }

    ~PGNLex() {
    }

    void reset();

    /**
        Reads another token from the input stream.

        @return token identifier
    */
    PGNTokenId PGNLex::getNextToken();

    /** Returns the current token identifier. */
    PGNTokenId tokenId() const {
        return tokenId_;
    }

    /** Returns the current token data as an integer. */
    int tokenAsNumber() const {
        return tokenAsNumber_;
    }

    /** Returns the current token data as a string. */
    const String & tokenAsString() const {
        return tokenAsString_;
    }

    /** Returns the input stream that is being parsed. */
    FILE * getInputStream() const {
        return inputStream_;
    }

private:
    char getNextChar();

    FILE *      inputStream_;
    char        lastChar_;
    int         curColumn_;
    int         curLine_;
    PGNTokenId  tokenId_;
    int         tokenAsNumber_;
    String      tokenAsString_;
};

#endif // PGN_LEX_H_
