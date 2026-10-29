/* The inlined form of the benchmark: the amalgamation and the harness share one translation unit,
 * so the compiler sees the library bodies exactly as it does when a project drops dist/ into its
 * own build. The out-of-line benchmark links the library instead. */
#include "bench_main.c"
#include "nepdate.c"
#include "nepdate_patro.c"
