#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <climits>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "util.h"

int performRequest(int fd, std::string const& buf, char rcvbuf[16384]) {
  int res = sendMsg(fd, buf.c_str(), buf.size());
  if (res < 0) {
    return res;
  }
  res = getMsg(fd, rcvbuf);
  if (res < 0) {
    return res;
  }
  rcvbuf[res] = 0;
  return res;
}

void work(char const* name, int portno) {
  struct sockaddr_in serv_addr;
  struct hostent *server;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
    return;
  }
  server = gethostbyname(name);
  if (server == NULL) {
    std::cerr << "ERROR, no such host: " << name << std::endl;
    return;
  }
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr,
         (char *)server->h_addr, 
         server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    error("ERROR connecting");
    return;
  }
  std::string buffer;
  char rcvbuf[16384];
  while (true) {
    std::cout << "Please enter the message: " << std::endl;
    std::getline(std::cin, buffer);
    if (buffer == "quit") {
      break;
    }
    int res = performRequest(sockfd, buffer, rcvbuf);
    if (res < 0) {
      break;
    }
    std::cout << rcvbuf + 2 << std::endl;
  }
  close(sockfd);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage " << argv[0] << " hostname port" << std::endl;
    exit(0);
  }
  int portno = std::stoi(argv[2]);
  work(argv[1], portno);
  return 0;
}

