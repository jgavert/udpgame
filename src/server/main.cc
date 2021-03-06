#include "server/server.h"
#include "common/util/die.h"

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

int quitPipe[2];

void handle_sigint(int) {
  char c = 'q';
  write(quitPipe[1], &c, 1);
}

int main(void) {
  if (-1 == pipe2(quitPipe, O_NONBLOCK)) die("pipe");
  if (SIG_ERR == signal(SIGINT, handle_sigint)) die("signal");

  Server server(quitPipe[0]);
  server.init();
  server.serve();

  return 0;
}
