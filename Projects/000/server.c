#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MY_PORT "3489"
#define MY_BACKLOG 10

int main(int argc, char* argv[]) {

  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct addrinfo *p;
  int sockfd;
  int yes = 1;
  char ipstr[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, MY_PORT, &hints, &servinfo);

  if (status != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = servinfo->ai_next) {

    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (sockfd == -1) {
      continue;
    }

    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (status == -1) {
      perror("setsockopt error");
      exit(1);
    }

    void* addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;

    inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));

    printf("binding %s\n", ipstr);

    status = bind(sockfd, p->ai_addr, p->ai_addrlen);

    if (status != 0) {
      close(sockfd);
      perror("bind error");
      exit(1);
    }

    break;
  }

  freeaddrinfo(servinfo);

  status = listen(sockfd, MY_BACKLOG);

  if (status != 0) {
    perror("listen error");
    exit(1);
  }

  struct sockaddr_storage their_addr;
  socklen_t sin_size = sizeof their_addr;

  int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
  send(new_fd, "Hello, world!", 13, 0);
  close(new_fd);

  close(sockfd);

  return 0;
}