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

void work(char const* name, int portno, size_t nrReq, size_t sizeReq) {
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
  buffer.reserve(sizeReq);
  for (size_t i = 0; i < sizeReq; i++) {
    buffer.push_back((char) (i % 0xff));
  }
  for (size_t i = 0; i < nrReq; i++) {
    int res = performRequest(sockfd, buffer, rcvbuf);
    if (res < 0) {
      error("ERROR in request");
    }
    std::cout << "Got result of length " << res << std::endl;
  }
  close(sockfd);
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    std::cerr << "Usage " << argv[0] << " hostname port nrReq sizeReq" 
              << std::endl;
    exit(0);
  }
  int portno = std::stoi(argv[2]);
  size_t nrReq = std::stoul(argv[3]);
  size_t sizeReq = std::stoul(argv[4]);
  work(argv[1], portno, nrReq, sizeReq);
  return 0;
}

