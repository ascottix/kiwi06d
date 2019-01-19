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
#include <string.h>

#include "log.h"
#include "pgn.h"
#include "pgn_lex.h"
#include "san.h"
#include "string.hxx"
#include "string_tokenizer.h"

const unsigned maxExportLineLength = 79;

const char * PGN::TagBlack      = "Black";
const char * PGN::TagBlackElo   = "BlackElo";
const char * PGN::TagDate       = "Date";
const char * PGN::TagEvent      = "Event";
const char * PGN::TagFEN        = "FEN";
const char * PGN::TagResult     = "Result";
const char * PGN::TagRound      = "Round";
const char * PGN::TagSite       = "Site";
const char * PGN::TagWhite      = "White";
const char * PGN::TagWhiteElo   = "WhiteElo";

static const char * SevenTagRoster[] = { 
    PGN::TagDate,
    PGN::TagEvent,
    PGN::TagSite,
    PGN::TagRound,
    PGN::TagWhite,
    PGN::TagBlack,
    PGN::TagResult,
    0 
};

static bool isTagInList( const char * tag, const char ** taglist )
{
    while( *taglist != 0 ) {
        if( strcmp( tag, *taglist ) == 0 ) {
            return true;
        }

        taglist++;
    }

    return false;
}

PGNMoveList::PGNMoveList()
{
    length_ = 0;
    max_length_ = 40;
    list_ = new PGNMove [ max_length_ ];
}

PGNMoveList::~PGNMoveList()
{
    for( int i=0; i<length_; i++ ) {
        delete list_[i].comment_;
    }

    delete [] list_;
}

PGNMove & PGNMoveList::move( int index ) const
{
    return list_[index];
}

int PGNMoveList::getMove( int index, PGNMove & move ) const
{
    if( (index < 0) || (index >= length_) ) {
        return -1;
    }

    move = list_[ index ];

    return 0;
}

void PGNMoveList::add( char from, char to, char promotion, char nag )
{
    if( length_ >= max_length_ ) {
        // Expand the list
        max_length_ += 20;

        PGNMove * new_list = new PGNMove [ max_length_ ];

        memcpy( new_list, list_, sizeof(PGNMove)*length_ );

        delete [] list_;

        list_ = new_list;
    }

    list_[ length_ ].from_ = from;
    list_[ length_ ].to_ = to;
    list_[ length_ ].promotion_ = promotion;
    list_[ length_ ].nag_ = nag;
    list_[ length_ ].comment_ = 0;
    
    length_++;
}

PGNGame::PGNGame()
{
    result_ = PGN::Unknown;
    tags_ = 0;
}

PGNGame::~PGNGame()
{
}

void PGNGame::clear()
{
    clearTagList();

    setStartPosition( Position::startPosition );
}

void PGNGame::setStartPosition( const char * fen )
{
    movelist_.clear();

    startPosition_.setBoard( fen );
}

// This macro verifies that the current token is the one we expect from the PGN
// grammar, and exits from the function otherwise.
// I know the method is dirty. Using exceptions would be cleaner, but I don't
// want them in my chess program...
#define Match( expectedTokenId )                    \
    if( lex->tokenId() != expectedTokenId ) {       \
        Log::write( "pgn: expected token %d, but found: %d\n", expectedTokenId, lex->tokenId() ); \
        return -1;                                  \
    }                                               \
                                                    \
    lex->getNextToken();

int PGNGame::parseHeader( PGNLex * lex )
{
    while( lex->tokenId() == pgnTokenOBracket ) {
        Match( pgnTokenOBracket );

        String tagName = lex->tokenAsString();

        Match( pgnTokenSymbol );

        String tagValue = lex->tokenAsString();

        Match( pgnTokenString );
        Match( pgnTokenCBracket );

        setTag( tagName.cstr(), tagValue.cstr() );
    }

    return 0;
}

int PGNGame::parseMoveList( PGNLex * lex, const Position & pos, PGNMoveList * list )
{
    Position p( pos );

    while( true ) {
        if( lex->tokenId() == pgnTokenOParen ) {
            // Variation
            Match( pgnTokenOParen );

            int result = parseMoveList( lex, p, 0 ); // Simply discard the variation

            if( result != 0 ) {
                return result;
            }

            Match( pgnTokenCParen );
        }
        else if( lex->tokenId() == pgnTokenNumber ) {
            // Move number
            Match( pgnTokenNumber );
            while( lex->tokenId() == pgnTokenPeriod ) {
                Match( pgnTokenPeriod );
            }
        }
        else if( lex->tokenId() == pgnTokenSymbol ) {
            // SAN move or else
            Move m;

            if( SAN::textToMove( &m, p, lex->tokenAsString().cstr() ) == 0 ) {
                // It's a move, check for a NAG token
                Match( pgnTokenSymbol );

                int nag = 0;

                if( lex->tokenId() == pgnTokenNAG ) {
                    nag = lex->tokenAsNumber();
                    Match( pgnTokenNAG );
                }

                // Add the move to the list
                if( list != 0 ) {
                    list->add( m.getFrom(), m.getTo(), m.getPromoted(), nag );
                }

                // Play the move so we get ready for the next one
                p.doMove( m );
            }
            else {
                return -1;
            }
        }
        else if( lex->tokenId() == pgnTokenComment ) {
            // Add the comment to the current move if any, otherwise just ignore it
            if( list->length() > 0 ) {
                PGNMove & m = list->move( list->length() - 1 );

                if( m.comment_ == 0 ) {
                    m.comment_ = new String( lex->tokenAsString() );
                }
                else {
                    *m.comment_ += " ";
                    *m.comment_ += lex->tokenAsString().cstr();
                }
            }

            Match( pgnTokenComment );
        }
        else {
            // Should be a result or end-of-variation token...
            break;
        }
    }

    return 0;
}

int PGNGame::loadFromFile( PGNLex * lex )
{
    int result = -1;

    if( lex->tokenId() == pgnTokenOBracket ) {
        // Get the header
        clearTagList();

        result = parseHeader( lex );

        // Get the move list
        movelist_.clear();

        if( result == 0 ) {
            // Setup initial position
            const char * fen = getTag( PGN::TagFEN );

            startPosition_.setBoard( fen == 0 ? Position::startPosition : fen );

            // Parse the game move list
            result = parseMoveList( lex, startPosition_, &movelist_ );
        }

        // Get result tag
        if( result == 0 ) {
            result_ = (PGN::Result) lex->tokenAsNumber();

            Match( pgnTokenResult );
        }
    }

    return result;
}

void PGNGame::clearTagList()
{
    while( tags_ != 0 ) {
        PGNTag * t = tags_->next_;

        delete tags_;

        tags_ = t;
    }
}

const char * PGNGame::getTag( const char * tag )
{
    PGNTag * t = tags_;

    while( t != 0 ) {
        if( t->name_ == tag ) {
            return t->value_.cstr();
        }

        t = t->next_;
    }

    return 0;
}

void PGNGame::setTag( const char * tag, const char * value )
{
    PGNTag * t = tags_;

    while( t != 0 ) {
        if( t->name_ == tag ) {
            t->value_ = value;
            return;
        }

        t = t->next_;
    }

    t = new PGNTag;

    t->name_ = tag;
    t->value_ = value;
    t->next_ = tags_;

    tags_ = t;
}

static void addToLine( FILE * f, String & line, const char * text )
{
    size_t space = line.length() == 0 ? 0 : 1;

    if( (line.length() + space + strlen(text)) > maxExportLineLength ) {
        fprintf( f, "%s\n", line.cstr() );
        line = text;
    }
    else {
        if( space ) {
            line += " ";
        }
        line += text;
    }
}

int PGNGame::saveToFile( const char * fileName, bool append )
{
    int result = -1;

    FILE * f = fopen( fileName, append ? "w" : "a" );

    if( f != 0 ) {
        result = saveToFile( f );

        fclose( f );
    }

    return result;
}

int PGNGame::saveToFile( FILE * f )
{
    // Save the "seven tag roster" list first
    int i = 0;

    while( SevenTagRoster[i] != 0 ) {
        const char * tag = getTag( SevenTagRoster[i] );

        if( tag == 0 ) {
            return -2;
        }

        fprintf( f, "[%s \"%s\"]\n", SevenTagRoster[i], tag );

        i++;
    }

    // Save the other tags
    PGNTag * tag = tags_;

    while( tag != 0 ) {
        if( ! isTagInList( tag->name_.cstr(), SevenTagRoster ) ) {
            fprintf( f, "[%s \"%s\"]\n", tag->name_.cstr(), tag->value_.cstr() );
        }

        tag = tag->next_;
    }

    fprintf( f, "\n" );

    // Save the move list
    Position pos( startPosition_ );
    String line;

    for( i=0; i<movelist_.length(); i++ ) {
        char buffer[30];

        buffer[0] = '\0';

        // Move number
        if( i == 0 ) {
            strcpy( buffer, startPosition_.sideToPlay == Black ? "1... " : "1. " );
        }
        else if( (startPosition_.sideToPlay == White) && ((i & 1) == 0) ) {
            sprintf( buffer, "%d. ", ((i + 1) / 2) + 1 );
        }
        else if( (startPosition_.sideToPlay == Black) && ((i & 1) != 0) ) {
            sprintf( buffer, "%d. ", ((i + 1) / 2) + 1 );
        }

        // Move text
        PGNMove & pgnMove = movelist_.move( i );

        Move move( pgnMove.from_, pgnMove.to_, pgnMove.promotion_ );

        SAN::moveToText( buffer+strlen(buffer), pos, move );

        pos.doMove( move );

        // Add to current line
        addToLine( f, line, buffer );

        // Add comment if any
        if( pgnMove.comment_ != 0 ) {
            pgnMove.comment_->trim();

            if( ! pgnMove.comment_->isEmpty() ) {
                String  comment( "{" );
                comment += pgnMove.comment_->cstr();
                comment += "}";

                // TODO: warning, tokenizer will remove quotes from quoted strings!
                StringTokenizer tk( comment );

                while( tk.hasMoreTokens() ) {
                    String token;

                    tk.getNextToken( token );

                    addToLine( f, line, token.cstr() );
                }
            }
        }
    }

    // Save the result
    switch( result_ ) {
    case PGN::WhiteWins:
        addToLine( f, line, "1-0" );
        break;
    case PGN::BlackWins:
        addToLine( f, line, "0-1" );
        break;
    case PGN::Drawn:
        addToLine( f, line, "1/2-1/2" );
        break;
    case PGN::Unknown:
        addToLine( f, line, "*" );
        break;
    }

    fprintf( f, "%s\n\n", line.cstr() );

    return 0;
}
