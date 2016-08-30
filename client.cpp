#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) {
  std::cout << msg << "\n" << strerror(errno) << std::endl;
  exit(0);
}

int main(int argc, char* argv[]) {
  struct sockaddr_in serv_addr;
  struct hostent *server;

  std::string buffer;
  char rcvbuf[256];
  if (argc < 3) {
    std::cerr << "Usage " << argv[0] << " hostname port" << std::endl;
    exit(0);
  }
  int portno = std::stoi(argv[2]);
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
  }
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    std::cerr << "ERROR, no such host: " << argv[1] << std::endl;
    return 0;
  }
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr,
         (char *)server->h_addr, 
         server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    error("ERROR connecting");
  }
  while (true) {
    std::cout << "Please enter the message: " << std::endl;
    std::getline(std::cin, buffer);
    if (buffer == "quit") {
      break;
    }
    char sizebuf[2];
    sizebuf[0] = (char) buffer.size();
    sizebuf[1] = 0;
    int n = write(sockfd, sizebuf, 2);
    if (n < 2) {
      error("ERROR writing to socket");
    }
    n = write(sockfd, buffer.c_str(), buffer.size());
    if (n < 0) {
      error("ERROR writing to socket");
    }
    memset(rcvbuf, 0, 256);
    n = read(sockfd, rcvbuf, 255);
    if (n < 0) {
      error("ERROR reading from socket");
    }
    rcvbuf[n] = 0;
    std::cout << rcvbuf + 2 << std::endl;
  }
  close(sockfd);
  return 0;
}

