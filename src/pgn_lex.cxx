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
#include <ctype.h>

#include "pgn.h"
#include "pgn_lex.h"

const char chQuote      = '"';
const char chPragma     = '%';
const char chEscape     = '\\';
const char chNewLine    = '\n';
const char chEOF        = '\x1A';

void PGNLex::reset()
{
    lastChar_ = '\n';
    curColumn_ = 0;
    curLine_ = 0;
    tokenId_ = pgnTokenNull;
    tokenAsNumber_ = 0;
    tokenAsString_.clear();
}

char PGNLex::getNextChar()
{
    int c = fgetc( inputStream_ );

    if( c < 0 ) {
        c = chEOF;
    }
    else if( c == chNewLine ) {
        curLine_++;
        curColumn_ = 0;
    }

    curColumn_++;

    lastChar_ = (char) c;

    return lastChar_;
}

PGNTokenId PGNLex::getNextToken()
{
    // Initialize token
    tokenId_ = pgnTokenNull;
    tokenAsNumber_ = 0;
    tokenAsString_.clear();

    // Restart from last character
    char c = lastChar_;

    // Strip blanks
    while( isspace(c) ) {
        c = getNextChar();
    }

    // Process character
    if( isalnum(c) ) {
        // Symbol (or number)
        tokenId_ = pgnTokenNumber;
        
        while( isalnum(c) || (c == '_') || (c == '+') || (c == '#') || (c == '=') || (c == ':') || (c == '-') || (c == '/') ) {
            tokenAsString_ += c;

            if( isdigit(c) && (tokenId_ == pgnTokenNumber) ) {
                tokenAsNumber_ = 10*tokenAsNumber_ + (c - '0');
            }
            else {
                tokenId_ = pgnTokenSymbol;
            }

            c = getNextChar();
        }

        // Convert symbol into result if possible
        if( (tokenAsString_ == "1-0") || (tokenAsString_ == "0-1") || (tokenAsString_ == "1/2-1/2") ) {
            tokenId_ = pgnTokenResult;
            tokenAsNumber_ = PGN::Drawn;
            if( tokenAsString_.length() == 3 ) {
                tokenAsNumber_ = tokenAsString_ == "1-0" ? PGN::WhiteWins : PGN::BlackWins;
            }
        }
    }
    else if( c == chQuote ) {
        // String
        tokenId_ = pgnTokenString;

        c = getNextChar();

        while( c != chQuote ) {
            if( c == chEscape ) {
                c = getNextChar();
            }
            else if( c == chEOF ) {
                tokenId_ = pgnTokenError;
                break;
            }
            else if( c == chNewLine ) {
                break;
            }

            tokenAsString_ += c;

            c = getNextChar();
        }

        getNextChar();
    }
    else if( (c == chPragma) && (curColumn_ == 1) ) {
        // Private extension
        tokenId_ = pgnTokenPragma;

        while( c != chNewLine ) {
            if( c == chEOF ) {
                tokenId_ = pgnTokenError;
                break;
            }

            tokenAsString_ += c;

            c = getNextChar();
        }

        getNextChar();
    }
    else if( (c == '{') || ((c == ';') && (curColumn_ == 1)) ) {
        // Comment
        tokenId_ = pgnTokenComment;

        char e = (c == '{') ? '}' : chNewLine;

        c = getNextChar();

        while( c != e ) {
            if( (c == ' ') || (! isspace(c)) ) {
                tokenAsString_ += c;
            }

            c = getNextChar();
        }

        getNextChar();
    }
    else if( c == '$' ) {
        // Numeric annotation glyph
        tokenId_ = pgnTokenNAG;

        c = getNextChar();

        while( isdigit(c) ) {
            tokenAsNumber_ = 10*tokenAsNumber_ + (c - '0');
            tokenAsString_ += c;

            c = getNextChar();
        }

        if( tokenAsString_.isEmpty() ) {
            // Missing NAG value
            tokenId_ = pgnTokenError;
        }
    }
    else if( (c == '!') || (c == '?') ) {
        // Annotation suffix (convert to NAG)
        tokenId_ = pgnTokenNAG;

        tokenAsNumber_ = (c == '!') ? 1 : 2;    // "!" and "?"
        tokenAsString_ += c;

        c = getNextChar();

        if( (c == '!') || (c == '?') ) {
            // Double character code
            tokenAsString_ += c;

            if( tokenAsNumber_ == 1 ) {
                tokenAsNumber_ = (c == '!') ? 3 : 5;    // "!!" and "!?"
            }
            else {
                tokenAsNumber_ = (c == '!') ? 6 : 4;    // "?!" and "??"
            }

            c = getNextChar();
        }
    }
    else {
        // One-character
        tokenAsString_ += c;

        switch( c ) {
        case chEOF:
            tokenId_ = pgnTokenEOF;
            break;
        case '(':
            tokenId_ = pgnTokenOParen;
            break;
        case ')':
            tokenId_ = pgnTokenCParen;
            break;
        case '[':
            tokenId_ = pgnTokenOBracket;
            break;
        case ']':
            tokenId_ = pgnTokenCBracket;
            break;
        case '<':
            tokenId_ = pgnTokenOAngleBracket;
            break;
        case '>':
            tokenId_ = pgnTokenCAngleBracket;
            break;
        case '.':
            tokenId_ = pgnTokenPeriod;
            break;
        case '*':
            tokenId_ = pgnTokenResult;
            break;
        default:
            tokenId_ = pgnTokenUnknown;
        }

        getNextChar();
    }

    return tokenId_;
}
