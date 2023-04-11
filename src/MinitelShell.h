#ifndef MinitelShell_h
#define MinitelShell_h

#include "Terminal.h"
#include "Shell.h"


#define INPUT_BUFFER_SIZE 64

class MinitelShell : public Shell
{
public:
    MinitelShell(Terminal *term) : Shell(term) {}

protected:
    char inputBuffer[INPUT_BUFFER_SIZE];

    virtual void runCommand();
};

#endif // MinitelShell_h
