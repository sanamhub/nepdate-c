#include <nepdate/nepdate.hpp>

#include <cstdio>

int main() {
    const auto bs = nepdate::from_gregorian(2024, 7, 30);
    if (!bs) {
        return 1;
    }
    std::printf("C++ BS %s\n", nepdate::to_string(bs.value()).c_str());
    return 0;
}
