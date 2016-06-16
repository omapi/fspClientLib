

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main() {
  if ((union semun *) 0)
    return 0;
  if (sizeof (union semun))
    return 0;
}
