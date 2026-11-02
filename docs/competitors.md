# Other C code for Bikram Sambat

Checked 2026-11-02 against GitHub repository search for "bikram sambat language:C", "nepali calendar
language:C" and "nepdate language:C", plus `vcpkg search` and `conan search -r conancenter`. The
selection follows [`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md) section 7. Usage is
GitHub stars plus dependents, read on 2026-11-02, and whether the code is in vcpkg or Conan. The bar
is 25 stars or a vcpkg or Conan port, first release at least 12 months before 2026-11-02, a release
or commit in the last 36 months, a license, and a build on the current toolchain.

## Candidate table

| Code | Stars | Last change | License | Kind | Verdict |
|---|---:|---|---|---|---|
| `aatishnn/nepdate` | 2 | 2015-03-18 | GPL-2.0 | small C library, no package | fails 25 stars and the 36-month bar |
| `khumnath/nepdate` | 16 | 2026-07-13 | GPL-3.0 | Qt desktop application | fails 25 stars and is an application, not a library |
| `khumnath/nepdate-cli` | 7 | 2026-02-05 | GPL-3.0 | C++ CLI | fails 25 stars and is not C |

vcpkg and Conan Center return nothing for `nepdate`, `nepali` or `bikram`. No C Bikram Sambat
library is published in either registry.

## Result

No candidate passes the bar. C has no mature library to compare, so the benchmark reports NepDate
only and the report says so. `aatishnn/nepdate`, the one competitor in earlier reports, is removed
from the harness and its old numbers are not kept.

## Sources

https://github.com/aatishnn/nepdate, https://github.com/khumnath/nepdate,
https://github.com/khumnath/nepdate-cli,
https://github.com/microsoft/vcpkg/tree/master/ports,
https://github.com/conan-io/conan-center-index/tree/master/recipes.
