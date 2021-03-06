#include <sstream>
#include <odbc/Util.h>
//------------------------------------------------------------------------------
using namespace std;
//------------------------------------------------------------------------------
namespace {
//------------------------------------------------------------------------------
void escape(const char* s, ostream& out)
{
    while (*s)
    {
        switch (*s)
        {
        case '"':
            out << "\"\"";
            break;
        default:
            out << *s;
            break;
        }
        ++s;
    }
}
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
NS_ODBC_START
//------------------------------------------------------------------------------
string Util::quote(const std::string& s)
{
    return quote(s.c_str());
}
//------------------------------------------------------------------------------
string Util::quote(const char* s)
{
    ostringstream os;
    os << "\"";
    escape(s, os);
    os << "\"";
    return os.str();
}
//------------------------------------------------------------------------------
string Util::quote(const std::string& schema, const std::string& table)
{
    return quote(schema.c_str(), table.c_str());
}
//------------------------------------------------------------------------------
string Util::quote(const char* schema, const char* table)
{
    ostringstream os;
    os << "\"";
    escape(schema, os);
    os << "\".\"";
    escape(table, os);
    os << "\"";
    return os.str();
}
//------------------------------------------------------------------------------
NS_ODBC_END
//------------------------------------------------------------------------------
