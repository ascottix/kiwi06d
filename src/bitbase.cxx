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
#include "bitbase.h"
#include "bitbase_adjust.h"
#include "log.h"
#include "string.hxx"

const char * BbWtmSuffix    = "_wtm";
const char * BbBtmSuffix    = "_btm";

struct BitBaseData
{
    BitBaseInfo *   info;
    PackedArray *   data[2];    // WTM and BTM
};

/*
    Info used to load bitbases: be careful when changing the order,
    as some bitbases may depend on others (for decoding)
*/
BitBaseInfo bbInfo[] = {
    // 3-pieces
    { bb_KPK,  bb_BitsWin, 1, { WhitePawn },    0,  "kpk",  0 },
    // 4-pieces
    { bb_KPPK, bb_BitsWin, 2, { WhitePawn, WhitePawn },     bb_PackRLE, "kppk", bbAdjustKPPK },
    { bb_KBPK, bb_BitsWin, 2, { WhiteBishop, WhitePawn }, bb_DefaultIs1 | bb_PackRLE, "kbpk", bbAdjustKBPK },
    { bb_KBNK,  bb_BitsWin, 2,  { WhiteBishop, WhiteKnight }, bb_DefaultIs1 | bb_PackRLE, "kbnk", bbAdjustKBNK },
    // Sentinel
    { 0, 0 }
};

static int              maxBbId = 0;
static BitBaseData *    bitBases;

unsigned getMaxSizeOfPackedDataRle( unsigned size );
unsigned packDataRle( unsigned char * dst, unsigned dstlen, const unsigned char * src, unsigned srclen );
unsigned getPackedDataLengthRle( const unsigned char * buf, unsigned buflen );
unsigned unpackDataRle( unsigned char * dst, unsigned dstlen, const unsigned char * src, unsigned srclen );

const BitBaseInfo * getBitBaseInfo( int id )
{
    assert( id >= 0 );
    assert( id < maxBbId );
    
    BitBaseInfo * result = bitBases[ id ].info;

    if( result != 0 ) {
        assert( result->filename != 0 );
        assert( result->id == id );
    }

    return result;
}

static unsigned char Magic[4] = { 'K', 'b', 'b', '0' };

static void getBitBaseFilename( int id, int wtm, String & filename )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );

    filename = info->filename;

    filename += wtm ? BbWtmSuffix : BbBtmSuffix;

    filename += ".bb";
}

static int saveBitBaseDataToFile( const char * filename, PackedArray * data, int id )
{
    int result = 0;

    // Open file
    FILE * f = fopen( filename, "wb" );

    if( f == 0 ) {
        result = -1;
    }

    // Get data
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );

    unsigned char * buf = data->getRawDataWriteable();
    unsigned len = data->getRawDataSize();

    if( result == 0 ) {
        if( (info->flags & bb_PackMask) == bb_PackRLE ) {
            // Pack with RLE compression
            len = getMaxSizeOfPackedDataRle( len );
            buf = new unsigned char [ len ];

            assert( buf != 0 );

            len = packDataRle( buf, len, data->getRawData(), data->getRawDataSize() );

            assert( len != 0 );
        }
    }

    // Write header
    unsigned char header[16];

    header[ 0] = Magic[0];
    header[ 1] = Magic[1];
    header[ 2] = Magic[2];
    header[ 3] = Magic[3];
    header[ 4] = 0; // CRC, not used
    header[ 5] = 0;
    header[ 6] = 0;
    header[ 7] = 0;
    header[ 8] = (unsigned char) ((len >> 24) & 0xFF);
    header[ 9] = (unsigned char) ((len >> 16) & 0xFF);
    header[10] = (unsigned char) ((len >>  8) & 0xFF);
    header[11] = (unsigned char) ((len      ) & 0xFF);
    header[12] = 0; // Not used
    header[13] = 0;
    header[14] = 0;
    header[15] = 0;

    if( result == 0 ) {
        if( fwrite( header, sizeof(header), 1, f ) != 1 ) {
            result = -2;
        }
    }

    // Write data
    if( result == 0 ) {
        if( fwrite( buf, len, 1, f ) != 1 ) {
            result = -3;
        }
    }

    // Free resources and exit
    if( buf != data->getRawData() ) {
        delete [] buf;
    }

    fclose( f );

    return result;
}

static PackedArray * loadBitBaseDataFromFile( const char * filename, int id )
{
    int result = 0;

    // Open file
    FILE * f = fopen( filename, "rb" );

    if( f == 0 ) {
        result = -1;
    }

    // Load header
    unsigned char header[16];

    if( result == 0 ) {
        if( fread( header, sizeof(header), 1, f ) != 1 ) {
            result = -2;
        }
        else if( (header[0] != Magic[0]) || (header[1] != Magic[1]) || (header[2] != Magic[2]) || (header[3] != Magic[3]) ) {
            result = -3;
        }
    }

    unsigned len = (unsigned) header[11] | (((unsigned) header[10]) << 8) |
        (((unsigned) header[9]) << 16) | (((unsigned) header[8]) << 24);

    // Create destination array
    const BitBaseInfo * info = getBitBaseInfo( id );

    PackedArray * pa = new PackedArray( getBitBaseIndexRange( id ), info->bits );

    if( pa == 0 ) {
        result = -4;
    }

    // Load data
    if( result == 0 ) {
        if( (info->flags & bb_PackMask) == bb_PackRLE ) {
            // Data is RLE packed
            unsigned char * buf = new unsigned char [ len ];

            assert( buf != 0 );

            if( fread( buf, len, 1, f ) != 1 ) {
                result = -5;
            }
            else {
                unsigned exp_len = pa->getRawDataSize();

                len = unpackDataRle( pa->getRawDataWriteable(), exp_len, buf, len );

                if( len != exp_len ) {
                    result = -6;
                }
            }

            delete [] buf;
        }
        else {
            // Data is raw
            if( len != pa->getRawDataSize() ) {
                result = -7;
            }

            if( fread( pa->getRawDataWriteable(), len, 1, f ) != 1 ) {
                result = -8;
            }
        }
    }

    if( f != 0 ) {
        fclose( f );
    }

    // Return null on error
    if( result != 0 ) {
        Log::write( "Cannot load bitbase '%s', error: %d\n", filename, result );
        delete pa;
        pa = 0;
    }

    return pa;
}

static int loadBitBase( int id, int wtm )
{
    PackedArray * pa = 0;

    const BitBaseInfo * info = getBitBaseInfo( id );

    if( info != 0 ) {
        String filename;

        getBitBaseFilename( id, wtm, filename );

        pa = loadBitBaseDataFromFile( filename.cstr(), id );

        if( pa != 0 ) {
            AdjustBitBaseFunc func = info->adjust_func;

            if( func != 0 ) {
                int res = (* func)( pa, id, wtm, AdjustBb_Decode );

                if( res != 0 ) {
                    Log::write( "Cannot decode bitbase '%s', error: %d\n", filename.cstr(), res );
                    delete pa;
                    pa = 0;
                }
            }
        }

        bitBases[ id ].data[wtm ? 0 : 1] = pa;

        if( pa != 0 ) {
            Log::write( "Bitbase '%s' loaded successfully\n", filename.cstr() );
        }
    }

    return pa == 0 ? -1 : 0;
}

int saveBitBase( int id, int wtm, const PackedArray * pa )
{
    assert( pa != 0 );

    int res = -1;

    const BitBaseInfo * info = getBitBaseInfo( id );

    if( info != 0 ) {
        assert( pa->getNumberOfElements() == (unsigned) getBitBaseIndexRange( id ) );
        assert( pa->getBitsPerElement() == info->bits );

        String filename;

        getBitBaseFilename( id, wtm, filename );

        res = 0;

        PackedArray * bb = new PackedArray( *pa );

        assert( bb != 0 );

        AdjustBitBaseFunc func = info->adjust_func;

        if( func != 0 ) {
            res = (* func) ( bb, id, wtm, AdjustBb_Encode );

            if( res != 0 ) {
                Log::write( "Cannot encode bitbase '%s', error: %d\n", filename.cstr(), res );
            }
        }

        if( res == 0 ) {
            res = saveBitBaseDataToFile( filename.cstr(), bb, id );

            if( res != 0 ) {
                Log::write( "Cannot save bitbase '%s'\n", filename.cstr() );
            }
        }

        if( bitBases[ id ].data[wtm ? 0 : 1] == 0 ) {
            bitBases[ id ].data[wtm ? 0 : 1] = bb;
        }
    }
    else {
        Log::write( "Invalid bitbase index %d, cannot save\n", id );
    }

    return res;
}

void initBitBases()
{
    int i;

    // Allocate enough space to index all bitbases
    maxBbId = 0;

    i = 0;

    while( bbInfo[i].filename != 0 ) {
        if( bbInfo[i].id > maxBbId ) {
            maxBbId = bbInfo[i].id;
        }

        i++;
    }

    maxBbId++;

    // Allocate just the space needed
    bitBases = new BitBaseData [ maxBbId ];

    for( i=0; i<maxBbId; i++ ) {
        bitBases[i].info = 0;
        bitBases[i].data[0] = 0;
        bitBases[i].data[1] = 0;
    }

    // Allow bitbase info to be accessed by index
    i = 0;

    while( bbInfo[i].filename != 0 ) {
        bitBases[ bbInfo[i].id ].info = &bbInfo[i];
        i++;
    }
}

void loadBitBases()
{
    int i = 0;

    // Load data for known bitbases
    while( bbInfo[i].filename != 0 ) {
        loadBitBase( bbInfo[i].id, 0 );
        loadBitBase( bbInfo[i].id, 1 );
        i++;
    }
}

PackedArray * getBitBase( int id, int wtm )
{
    assert( id >= 0 );
    assert( id < maxBbId );

    return bitBases[ id ].data[ wtm ? 0 : 1 ];
}

inline int getBbIndexForKk( int wk, int bk )
{
    return (8*FileOfSquare(wk)+RankOfSquare(wk)) + 32*bk;
}

int getBitBaseIndex( int id, int wk, int bk, int p1 )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );
    assert( info->num_pieces == 1 );

    if( id == bb_KPK ) {
        p1 -= 8;
    }

    return getBbIndexForKk( wk, bk ) + 32*64*p1;
}

int getBitBaseIndex( int id, int wk, int bk, int p1, int p2 )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );
    assert( info->num_pieces == 2 );

    int r1 = 32*64*64;

    if( PieceType(info->pieces[0]) == Pawn ) {
        p1 -= 8;
        r1 = 32*64*48;
    }

    if( PieceType(info->pieces[1]) == Pawn ) {
        p2 -= 8;
    }

    return getBbIndexForKk( wk, bk ) + 32*64*p1 + r1*p2;
}

int getBitBaseIndex( int id, const PositionEnumerator & pe )
{
    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );
    assert( info->num_pieces == (pe.getPieceCount() - 2) );

    int result = getBbIndexForKk( pe.getWhiteKingPos(), pe.getBlackKingPos() );
    int range = 32*64;

    for( int i=2; i<pe.getPieceCount(); i++ ) {
        if( PieceType( pe.getPiece( i ) ) == Pawn ) {
            result += range * (pe.getPiecePos(i) - 8);
            range *= 48;
        }
        else {
            result += range * pe.getPiecePos(i);
            range *= 64;
        }
    }

    return result;
}

int getBitBaseIndexRange( int id )
{
    int result = 32*64;

    const BitBaseInfo * info = getBitBaseInfo( id );

    assert( info != 0 );

    for( int i=0; i<info->num_pieces; i++ ) {
        int p = info->pieces[i];

        result *= PieceType(p) == Pawn ? 48 : 64;
    }

    return result;
}

unsigned getMaxSizeOfPackedDataRle( unsigned size )
{
    return size + (size / 128) + 1;
}

unsigned packDataRle( unsigned char * dst, unsigned dstlen, const unsigned char * src, unsigned srclen )
{
    const unsigned char * input = src;
    const unsigned char * input_end = src + srclen;
    unsigned char * output = dst;

    if( dstlen < getMaxSizeOfPackedDataRle( srclen ) ) {
        assert( 0 );
        return 0;
    }

    while( input < input_end ) {
        const unsigned char * input_start = input;

        // Search for start of a run length
        while( ((input + 2) < input_end) && ((input[0] != input[1]) || (input[1] != input[2])) ) {
            input++;
        }

        size_t count = input - input_start;

        // These bytes were found before the beginning of a run length, and must be copied
        while( count > 0 ) {
            size_t block_length = count > 128 ? 128 : count;

            *output++ = 0x80 | (unsigned char) (block_length - 1);

            memcpy( output, input_start, block_length );

            output += block_length;
            input_start += block_length;
            count -= block_length;
        }

        // Compute the length of the repeated sequence (if any)
        input_start = input;

        unsigned char rle_byte = *input++;

        while( (input < input_end) && (*input == rle_byte) ) {
            input++;
        }

        count = input - input_start;

        while( count > 0 ) {
            size_t run_length = count > 128 ? 128 : count;

            *output++ = (unsigned char) (run_length - 1);
            *output++ = rle_byte;

            count -= run_length;
        }
    }

    return (unsigned) (output - dst);
}

unsigned getPackedDataLengthRle( const unsigned char * buf, unsigned buflen )
{
    unsigned result = 0;

    while( buflen > 0 ) {
        unsigned char header = *buf++;
        unsigned count = 1 + (unsigned) (header & 0x7F);

        buflen--;

        result += count;

        if( header & 0x80 ) {
            // Data sequence
            assert( buflen >= count);

            buf += count;
            buflen -= count;
        }
        else {
            // Run length encoded sequence
            assert( buflen >= 1 );

            buf++;
            buflen--;
        }
    }

    return result;
}

unsigned unpackDataRle( unsigned char * dst, unsigned dstlen, const unsigned char * src, unsigned srclen )
{
    unsigned result = 0;

    while( srclen > 0 ) {
        unsigned char header = *src++;
        unsigned count = 1 + (unsigned) (header & 0x7F);

        srclen--;

        result += count;

        assert( result <= dstlen );

        if( result > dstlen ) {
            return 0;
        }

        if( header & 0x80 ) {
            // Data sequence
            assert( srclen >= count);

            memcpy( dst, src, count );
            dst += count;
            src += count;
            srclen -= count;
        }
        else {
            // Run length encoded sequence
            assert( srclen >= 1 );
            
            memset( dst, *src, count );
            dst += count;
            src++;
            srclen--;
        }
    }

    return result;
}
