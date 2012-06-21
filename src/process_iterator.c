#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <time.h>
#include "process_iterator.h"

//See this link to port to other systems: http://www.steve.org.uk/Reference/Unix/faq_8.html#SEC85

#ifdef __linux__

#include "process_iterator_linux.c"

#elif defined __FreeBSD__

#include "process_iterator_freebsd.c"

#elif defined __APPLE__

#include "process_iterator_apple.c"

#else

#error Platform not supported

#endif
