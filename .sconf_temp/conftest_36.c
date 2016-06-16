

#include <assert.h>

#ifdef __cplusplus
extern "C"
#endif
char lockf();

int main() {
#if defined (__stub_lockf) || defined (__stub___lockf)
  fail fail fail
#else
  lockf();
#endif

  return 0;
}
