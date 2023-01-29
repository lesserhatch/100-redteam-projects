#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MY_PORT "3489"
#define MY_BACKLOG 10

int open_socket(char *port);
int receive_message(int sockfd);
void *get_in_addr(struct sockaddr *sa);

/// Buffer to receive data from socket listener
char receive_buffer[1024];

int main(int argc, char *argv[]) {
  int sockfd;
  struct sockaddr_storage their_addr;
  socklen_t sin_size = sizeof their_addr;

  sockfd = open_socket(MY_PORT);

  while (1) {
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (fork() == 0) {
      // child process handles the new connection
      // no need to accept more connections in this child process
      close(sockfd);
      receive_message(new_fd);
      close(new_fd);
      exit(0);
    }

    // parent process does not need the new connection
    close(new_fd);
  }

  close(sockfd);
  return 0;
}

/// @brief Open a socket listener
/// @param port string representation of the network port
/// @return socket file descriptor
int open_socket(char *port) {
  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct addrinfo *p;
  int sockfd;
  int yes = 1;
  char ipstr[INET6_ADDRSTRLEN];

  // Get a TCP socket with our address
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, port, &hints, &servinfo);

  if (status != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  // Loop through available addresses and connect to one
  for (p = servinfo; p != NULL; p = servinfo->ai_next) {

    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (sockfd == -1) {
      continue;
    }

    // allow the socket to reuse ports
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (status == -1) {
      perror("setsockopt error");
      exit(1);
    }

    // bind the socket to the address/port
    status = bind(sockfd, p->ai_addr, p->ai_addrlen);

    if (status != 0) {
      close(sockfd);
      perror("bind error");
      exit(1);
    }

    break;
  }

  status = listen(sockfd, MY_BACKLOG);

  if (status != 0) {
    perror("listen error");
    exit(1);
  }

  // Get the address and convert to presentation
  inet_ntop(p->ai_family, get_in_addr(p->ai_addr), ipstr, sizeof(ipstr));
  printf("Listening on %s on port %s...\n", ipstr, port);

  freeaddrinfo(servinfo);

  return sockfd;
}

/// @brief Receive message from socket connection
/// @param sockfd socket file descriptor
/// @returns bytes read or receive status
/// @retval -1 for error
/// @retval 0 for closed connection
int receive_message(int sockfd) {
  int status = recv(sockfd, &receive_buffer, sizeof(receive_buffer), 0);

  if (status == -1) {
    fprintf(stderr, "[%d] Error: %s\n", getpid(), strerror(errno));
  } else if (status == 0) {
    fprintf(stderr, "[%d] Error: remote closed connection\n", getpid());
  } else {
    printf("[%d] Message Received: %s\n", getpid(), receive_buffer);
  }

  return status;
}

/// @brief Get internet address
/// @param sa socket address struct
/// @return internet address pointer
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
