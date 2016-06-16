

#include <assert.h>

#ifdef __cplusplus
extern "C"
#endif
char flock();

int main() {
#if defined (__stub_flock) || defined (__stub___flock)
  fail fail fail
#else
  flock();
#endif

  return 0;
}
