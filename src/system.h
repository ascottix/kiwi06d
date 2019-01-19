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
#ifndef SYSTEM_H_
#define SYSTEM_H_

/**
    System dependent functions.

    This class (again, used like a namespace) contains methods that
    hide platform and system dependent functions.
*/
class System
{
public:
    /** 
        Returns the value of a system timer, in milliseconds.

        Note: since it's impossible to know when the timer started, the return value
        is only useful to compute differences, i.e. elapsed intervals.
    */
    static unsigned getTickCount();

    /** Releases rest of time slice for the current thread. */
    static void yield();

    /** Puts current thread to sleep for (approximately) the specified time in milliseconds. */
    static void sleep( unsigned ms );

    /** 
        Returns true if there is input pending, false otherwise.

        When this function returns true, call readInputLine() to get the input.

        @return true if there is input available, false otherwise
    */
    static bool isInputAvailable();

    /** 
        Returns true if the input stream has been closed, false otherwise.

        When the input stream is closed and the EOF_AS_INPUT compile option is enabled, 
        isInputAvailable() will be forced to true (to get the program "attention")
        and this method must be called before attempting to read input.

        @return true if the input stream has been closed, false otherwise
    */
    static bool isInputStreamClosed() {
        return inputStreamClosed_;
    }

    /** 
        Reads one input line.

        Note: after a successful (i.e. not NULL) return it is necessary
        to call acknowledgeInput() to process further input, otherwise this
        function will always return the same value.

        @return an input line or NULL if there is no input
    */
    static const char * readInputLine();

    /**
        Acknowledges the input returned by readInputLine() and allows processing
        of further input.

        Note that readInputLine() will always return the same value until this
        function is called.

        Has no effect if there is no input to acknowledge.
    */
    static void acknowledgeInput();

    /**
        Returns the status of the startup initialization.

        The system library automatically performs the necessary initialization
        at program startup and sets a status variable accordingly.

        In normal conditions, initialization will never fail. However, 
        if this function does not return success then it is recommended that the
        program logs a proper error message and then exits immediately, as it 
        won't be able to receive any kind of input.

        @return 0 if the initialization was successful, an error code otherwise
    */
    static int getInitializationStatus() {
        return initializer_.status;
    }

private:
    struct SystemInit {
        SystemInit();

        ~SystemInit();

        int status; // Initialization status (0 if ok, otherwise an error code)
    };

    friend struct SystemInit;

    // This static member is used to perform automatic initialization (in the constructor)
    // and termination (in the destructor) of the system library
    static SystemInit initializer_;

    static volatile bool inputStreamClosed_;

    static volatile bool inputAvailable_;
};

#endif // SYSTEM_H_
