#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
  std::cout << msg << "\n" << strerror(errno) << std::endl;
}

void work(int newsockfd) {
  char buffer[256];
  memset(buffer, 0, 256);
  int n = read(newsockfd,buffer,255);
  if (n < 0) {
    error("ERROR reading from socket");
    close(newsockfd);
    return;
  }
  buffer[n] = 0;
  std::cout << "Here is the message: " << buffer << std::endl;
  n = write(newsockfd, "I got your message", 18);
  if (n < 0) {
    error("ERROR writing to socket");
  }
  close(newsockfd);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "ERROR, no port provided" << std::endl;
    return 1;
  }
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
    return 0;
  }
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<void*>(&opt), sizeof(opt));
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  int portno = std::stoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
    return 0;
  }
  listen(sockfd,100);
  clilen = sizeof(cli_addr);
  while (true) {
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
      error("ERROR on accept");
      return 0;
    }
    work(newsockfd);
  }
  close(sockfd);
  return 0; 
}

