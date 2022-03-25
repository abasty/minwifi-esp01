#ifndef MinitelShell_h
#define MinitelShell_h

#include "Shell.h"

class MinitelShell : public Shell
{
public:
    MinitelShell(Print *term = 0, Print *bin = 0) : Shell(term, bin) {}
#define INPUT_SIZE 64
    char input0[INPUT_SIZE];

    void connectServer();

protected:
    virtual void runCommand();
};

#endif // MinitelShell_h
