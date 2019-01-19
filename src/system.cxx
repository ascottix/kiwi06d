/*
    Kiwi
    System dependent functions

    Copyright (c) 2004,2005 Alessandro Scotti

    You can freely use this software in your CHESS ENGINE (and NOT other kind
    of programs, libraries or applications). Giving credits would be nice,
    but it is not required.

    This software is distributed "as-is" in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    In no event will the author be held liable for any damage arising from
    the use of this software.
*/
#include <stdio.h>
#include <string.h>

#include "system.h"

/*
Although it is always possible to call isInputStreamClosed() to check the status
of the input stream, it is often convenient to do the check only in the input
processing part of the program. If the following is defined (either in the program
or on the project settings or command line):

#define EOF_AS_INPUT

then isInputAvailable() will start returning true as soon as the input stream
has been closed, which allows you to handle this condition almost as if it is
part of the input.
*/

static bool isEndOfLineChar( char c ) {
    return (c == '\r') || (c == '\n');
}

volatile bool System::inputStreamClosed_ = false;
volatile bool System::inputAvailable_ = false;

System::SystemInit System::initializer_; // To perform initialization and cleanup

static char inputBuffer[2048];
static int  inputBufIdx;
static int  inputBufLen = 0;

struct ThreadParam
{
    volatile bool *  pInputAvailable;
    volatile bool *  pInputStreamClosed;
};

#ifdef WIN32

// Define WIN32_POLLING to use polling when input is a pipe: this is necessary
// because there is a nasty bug that may cause the input thread to freeze for
// seconds or even minutes inside ReadFile() when there is input available
#define WIN32_POLLING

#include <windows.h>
#include <io.h>         // For _isatty()

static HANDLE   hInput              = INVALID_HANDLE_VALUE;
static HANDLE   hInputThread        = INVALID_HANDLE_VALUE;
static HANDLE   hInputBufferEvent   = INVALID_HANDLE_VALUE;
static BOOL     isInputPipe;

// Note: this thread is constantly blocked on some object,
// so in practice it takes only a negligible amount of CPU time
static DWORD WINAPI ReadInputThread( LPVOID param )
{
    ThreadParam *   tp = (ThreadParam *) param;

    while( 1 ) {
        DWORD   dwRead;

        if( ! ReadFile( hInput, inputBuffer+inputBufLen, sizeof(inputBuffer)-inputBufLen, &dwRead, 0 ) )
            break;

        if( dwRead == 0 )
            break;

        // Converts all end-of-line characters to null and detect if at least one line has been read
        bool hasFullLine = false;

        for( DWORD dw=0; dw<dwRead; dw++ ) {
            if( isEndOfLineChar( inputBuffer[inputBufLen + dw] ) ) {
                hasFullLine = true;
                inputBuffer[inputBufLen + dw] = '\0';
            }
        }

        inputBufLen += (int) dwRead;

        // If at least one line has been read, make input available and wait until it's been processed
        if( hasFullLine ) {
            inputBufIdx = 0;

            *tp->pInputAvailable = true;

            WaitForSingleObject( hInputBufferEvent, INFINITE );
        }

        if( inputBufLen == sizeof(inputBuffer) ) {
            // Uh-oh, buffer is full and we haven't a full line to output: sorry but some data has to go...
            inputBufLen = 0;
        }
    }

    *tp->pInputStreamClosed = true;

#ifdef EOF_AS_INPUT
    *tp->pInputAvailable = true;
#endif

    return 0;
}

#else // POSIX

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

static pthread_t        inputThread;
static pthread_mutex_t  inputAckLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   inputAckCond = PTHREAD_COND_INITIALIZER;

static void * ReadInputThread( void * param )
{
    ThreadParam *   tp = (ThreadParam *) param;

    while( 1 ) {
        int n = read( 0, inputBuffer+inputBufLen, sizeof(inputBuffer)-inputBufLen );

        if( n <= 0 )
            break;

        // Converts all end-of-line characters to null and detect if at least one line has been read
        bool hasFullLine = false;

        for( int i=0; i<n; i++ ) {
            if( isEndOfLineChar( inputBuffer[inputBufLen + i] ) ) {
                hasFullLine = true;
                inputBuffer[inputBufLen + i] = '\0';
            }
        }

        inputBufLen += n;

        // If at least one line has been read, make input available and wait until it's been processed
        if( hasFullLine ) {
            inputBufIdx = 0;

            *tp->pInputAvailable = true;
           
            // Wait for acknowledgement
            while( *tp->pInputAvailable ) {
                pthread_mutex_lock( &inputAckLock );
    
                if( *tp->pInputAvailable ) {
                    pthread_cond_wait( &inputAckCond, &inputAckLock );
                }

                pthread_mutex_unlock( &inputAckLock );
            }
        }

        if( inputBufLen == sizeof(inputBuffer) ) {
            // Uh-oh, buffer is full and we haven't a full line to output: sorry but some data has to go...
            inputBufLen = 0;
        }
    }

    *tp->pInputStreamClosed = true;

#ifdef EOF_AS_INPUT
    *tp->pInputAvailable = true;
#endif
    
    return 0;
}

#endif

/*
    System library initialization
*/
System::SystemInit::SystemInit()
{
    status = 0;

#ifdef WIN32
    hInput = GetStdHandle( STD_INPUT_HANDLE );

    DWORD   dwConsoleMode;

    isInputPipe = false;

    if( ! GetConsoleMode( hInput, &dwConsoleMode ) ) {
        DWORD dwAvailable = 0;

        isInputPipe = PeekNamedPipe( hInput, 0, 0, 0, &dwAvailable, 0 ) != 0;
    }

#ifdef WIN32_POLLING
    if( isInputPipe ) {
        // Use polling on pipes, do not start the input thread
        OutputDebugString( "System: using polling to read input from pipe\n" );
        return;
    }
#endif

    // Prepare thread parameters
    ThreadParam * tp = new ThreadParam;

    tp->pInputAvailable = &System::inputAvailable_;
    tp->pInputStreamClosed = &System::inputStreamClosed_;

    hInputBufferEvent = CreateEvent( 0, FALSE, FALSE, 0 );
    
    if( hInputBufferEvent == INVALID_HANDLE_VALUE ) {
        status = -1;
    }
    else {
        DWORD dwThreadId; // We don't need this, but Windows 95/98 complains if we don't specify a valid pointer

        hInputThread = CreateThread( 0, 0, ReadInputThread, tp, 0, &dwThreadId );

        if( hInputThread == INVALID_HANDLE_VALUE ) {
            status = -2;
        }
    }
#else // POSIX
    // Prepare thread parameters
    ThreadParam * tp = new ThreadParam;

    tp->pInputAvailable = &System::inputAvailable_;
    tp->pInputStreamClosed = &System::inputStreamClosed_;

    pthread_attr_t attr;
    pthread_attr_init( &attr );

    status = pthread_create( &inputThread, &attr, ReadInputThread, tp );

    pthread_attr_destroy(&attr);
#endif
}

/*
    System library finalization
*/
System::SystemInit::~SystemInit()
{
#ifdef WIN32
    // Try to make the input thread exit but do not wait too much for it,
    // as we assume that the program is exiting anyway
    CloseHandle( hInput );

    if( ! System::isInputStreamClosed() ) {
        while( System::isInputAvailable() && ! System::isInputStreamClosed() ) {
            System::acknowledgeInput();
        }

        if( hInputThread != INVALID_HANDLE_VALUE ) {
            WaitForSingleObject( hInputThread, 500 );
        }
    }

    if( hInputBufferEvent != INVALID_HANDLE_VALUE ) {
        CloseHandle( hInputBufferEvent );
    }

#else // POSIX
    pthread_cancel( inputThread );
#endif
}

bool System::isInputAvailable()
{
#ifdef WIN32_POLLING
    // Peek the named pipe
    if( isInputPipe && ! inputAvailable_ && ! inputStreamClosed_ ) {
        DWORD dwAvailable = 0;

        if( PeekNamedPipe( hInput, 0, 0, 0, &dwAvailable, 0 ) ) {
            if( dwAvailable > 0 ) {
                DWORD dwRead;

                if( ! ReadFile( hInput, inputBuffer+inputBufLen, sizeof(inputBuffer)-inputBufLen, &dwRead, 0 ) ) {
                    OutputDebugString( "System: error reading input from pipe\n" );
                    return false;
                }

                if( dwRead == 0 ) {
                    OutputDebugString( "System: reading input from pipe returned no data\n" );
                    return false;
                }

                // Converts all end-of-line characters to null and detect if at least one line has been read
                bool hasFullLine = false;

                for( DWORD dw=0; dw<dwRead; dw++ ) {
                    if( isEndOfLineChar( inputBuffer[inputBufLen + dw] ) ) {
                        hasFullLine = true;
                        inputBuffer[inputBufLen + dw] = '\0';
                    }
                }

                inputBufLen += (int) dwRead;

                // If at least one line has been read, make input available and wait until it's been processed
                if( hasFullLine ) {
                    inputBufIdx = 0;

                    inputAvailable_ = true;
                }
                else {
                    if( inputBufLen == sizeof(inputBuffer) ) {
                        // Uh-oh, buffer is full and we haven't a full line to output: sorry but some data has to go...
                        OutputDebugString( "System: input buffer full, flushing...\n" );
                        inputBufLen = 0;
                    }
                }
            }
            // ...else there is no input available
        }
        else {
            OutputDebugString( "System: input pipe closed!\n" );

            // Cannot peek the pipe
#ifdef EOF_AS_INPUT
            inputAvailable_ = true;
#endif

            inputStreamClosed_ = true;
            
        }
    }
#endif

    return inputAvailable_;
}

const char * System::readInputLine()
{
    return inputAvailable_ && ! inputStreamClosed_ ? inputBuffer+inputBufIdx : 0;
}

void System::acknowledgeInput()
{
    if( inputAvailable_ ) {
        inputBufIdx += strlen(inputBuffer + inputBufIdx);

        // Skip all terminating characters
        while( (inputBufIdx < inputBufLen) && (inputBuffer[inputBufIdx] == '\0') ) {
            inputBufIdx++;
        }

        // Check to see if there is another full line to read
        if( memchr( inputBuffer+inputBufIdx, 0, inputBufLen-inputBufIdx ) == 0 ) {
            // No more lines, unlock input thread
            inputBufLen -= inputBufIdx;

            if( inputBufLen > 0 ) {
                memmove( inputBuffer, inputBuffer+inputBufIdx, inputBufLen );
            }

#ifdef WIN32
            inputAvailable_ = false;

            if( hInputBufferEvent != INVALID_HANDLE_VALUE ) {
                SetEvent( hInputBufferEvent );
            }
#else // POSIX
            pthread_mutex_lock( &inputAckLock );

            inputAvailable_ = false;

            pthread_cond_signal( &inputAckCond );

            pthread_mutex_unlock( &inputAckLock );
#endif
        }
        // ...else there is more input and now we point to the right place to read it
    }
}


unsigned System::getTickCount()
{
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval  tv;

    gettimeofday( &tv, NULL );

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

void System::yield()
{
#ifdef WIN32
    Sleep( 1 );
#else
    usleep( 1000 );
#endif
}

void System::sleep( unsigned ms )
{
#ifdef WIN32
    Sleep( ms );
#else // POSIX
    struct timeval  tv;
    struct timespec ts;
    pthread_cond_t  cond;
    pthread_mutex_t mutex;

    // Condition wait requires an absolute time
    gettimeofday( &tv, NULL );

    ts.tv_sec = tv.tv_sec + (ms / 1000);
    ts.tv_nsec = tv.tv_usec + (ms % 1000);
    if( ts.tv_nsec > 1000 ) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000;
    }
    ts.tv_nsec *= 1000; // Convert microseconds to nanoseconds

    // Build a condition variable and wait for the specified time
    pthread_mutex_init( &mutex, NULL );
    pthread_cond_init( &cond, NULL );

    pthread_mutex_lock( &mutex );

    pthread_cond_timedwait( &cond, &mutex, &ts );

    // Destroy the condition
    pthread_cond_destroy( &cond );
    pthread_mutex_unlock( &mutex );
    pthread_mutex_destroy( &mutex );
#endif
}
