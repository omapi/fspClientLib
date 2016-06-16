

#include <assert.h>

#ifdef __cplusplus
extern "C"
#endif
char semop();

int main() {
#if defined (__stub_semop) || defined (__stub___semop)
  fail fail fail
#else
  semop();
#endif

  return 0;
}
