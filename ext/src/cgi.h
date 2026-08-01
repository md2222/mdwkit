#ifndef CGI_H
#define CGI_H

#include <string>

using namespace std;

struct Error
{
    int code = 0;
    string text = "";
};

//extern void cgi(const char* script, const char* data, int dataSize, string& out, GError& err);
void cgi(const string& script, const string& params, const string& data, string& out, Error& err);


#endif // CGI_H
