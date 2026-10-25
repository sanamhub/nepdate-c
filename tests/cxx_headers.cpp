// The public header compiled as C++ (ADR-0001): it must parse, link through extern "C" and agree
// with the library it was built with.
#include <nepdate.h>

#include <cstring>

int main() { return std::strcmp(nepdate_version(), NEPDATE_VERSION_STRING) == 0 ? 0 : 1; }
