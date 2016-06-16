

#include <assert.h>

#ifdef __cplusplus
extern "C"
#endif
char shmget();

int main() {
#if defined (__stub_shmget) || defined (__stub___shmget)
  fail fail fail
#else
  shmget();
#endif

  return 0;
}
