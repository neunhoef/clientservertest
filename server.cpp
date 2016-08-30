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
  exit(0);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "ERROR, no port provided" << std::endl;
    return 1;
  }
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
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
  }
  listen(sockfd,100);
  clilen = sizeof(cli_addr);
  int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) {
    error("ERROR on accept");
  }
  memset(buffer, 0, 256);
  int n = read(newsockfd,buffer,255);
  if (n < 0) {
    error("ERROR reading from socket");
  }
  buffer[n] = 0;
  std::cout << "Here is the message: " << buffer << std::endl;
  n = write(newsockfd, "I got your message", 18);
  if (n < 0) {
    error("ERROR writing to socket");
  }
  close(newsockfd);
  close(sockfd);
  return 0; 
}

