#ifndef __INIT_WRAPPER_H_INCL__
    #define __INIT_WRAPPER_H_INCL__

class CefInitWrapper
{
public:
    CefInitWrapper( int argc, char *argv[] );
    ~CefInitWrapper();
    bool wasCefInstance() { return initResultCode < 0; }
    int resultCode() { return initResultCode; }
private:
    int initResultCode;
};

#endif // __INIT_WRAPPER_H_INCL__
