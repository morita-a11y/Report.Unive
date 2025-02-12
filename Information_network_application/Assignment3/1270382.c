#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>

int main (void) {
  int listenfd, connfd, nbytes;
  char buf[BUFSIZ];
  pid_t pid;
  struct sockaddr_in servaddr;
  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("socket"); exit (1);
  }
  memset (&servaddr, 0, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons (10000); /* echo port 7 is reserved */
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))
      < 0) {
    perror ("bind"); exit (1);
  }
  if (listen (listenfd, 5) < 0) {
    perror ("listen"); exit (1);
  }
  for (;;) {
    if ((connfd = accept (listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
      perror ("accept"); exit (1);
    }
    fprintf (stderr, "accept\n");
    pid = fork ();
    fprintf (stderr, "fork: %d\n", pid);
    if (pid == 0) {	/* child */
      fprintf (stderr, "child\n");
      close (listenfd);
      while ((nbytes = read (connfd, buf, sizeof (buf))) > 0) {
	fprintf (stderr, "child: ");
	fwrite (buf, 1, nbytes, stderr);
	write (connfd, buf, nbytes);
      }
      close (connfd);
      fprintf (stderr, "child: exit\n");
      exit (0);
    } else {		/* parent */
      int stat; pid_t wpid;
      fprintf (stderr, "parent\n");
      close (connfd);
      while ((wpid = waitpid (-1, &stat, WNOHANG)) > 0) {
	// Do nothing
      }
    }
  }
}