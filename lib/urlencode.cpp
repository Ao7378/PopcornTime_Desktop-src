#include "urlencode.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

std::string urlEncode( const std::string& url )
{
    ostringstream escaped;
    escaped.fill( '0' );
    escaped << hex;

    for ( string::const_iterator i = url.begin(), n = url.end(); i != n; ++i )
    {
        unsigned char c = (*i);
        if ( isalnum( (unsigned char) c ) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' ) escaped << c;
        else if (  c == '\\' ) escaped << '/';
//      else if ( c == ' ' ) escaped << '+';
        else escaped << '%' << setw( 2 ) << ((int) c) << setw( 0 );
    }

    return escaped.str();
}
