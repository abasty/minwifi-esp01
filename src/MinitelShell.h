#ifndef MinitelShell_h
#define MinitelShell_h

#include "Shell.h"

#define INPUT_BUFFER_SIZE 64

class MinitelShell : public Shell
{
public:
    MinitelShell(Print *term = 0) : Shell(term) {}
    void connectServer();

protected:
    char inputBuffer[INPUT_BUFFER_SIZE];

    virtual void runCommand();
};

#endif // MinitelShell_h
