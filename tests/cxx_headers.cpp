// The public header compiled as C++ (ADR-0001): it must parse, link through extern "C" and agree
// with the library it was built with.
#include <nepdate.h>
#include <nepdate_patro.h>

#include <cstring>

int main() {
    const nepdate_range coverage = nepdate_patro_coverage();
    return std::strcmp(nepdate_version(), NEPDATE_VERSION_STRING) == 0 &&
                   coverage.start.value <= coverage.end.value
               ? 0
               : 1;
}
