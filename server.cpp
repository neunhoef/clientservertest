#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <climits>
#include <atomic>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"

std::atomic<int> connCount(0);
std::atomic<uint64_t> reqCount(0);

void work(int newsockfd) {
  char buffer[16384];
  memset(buffer, 0, 16384);
  std::cout << "New connection, currently " << ++connCount << " open."
            << std::endl;
  uint64_t count = 0;
  while (true) {
    int pos = getMsg(newsockfd, buffer);
    if (pos < 0) {
      close(newsockfd);
      std::cout << "Connection closed. Currently open: " << --connCount
                << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "Exiting." << std::endl;
      return;
    }
    buffer[pos] = 0;
    std::cout << "Here is the message of size " << pos << ": " 
              << buffer + 2 << std::endl;
    if (++count >= 100000) {
      reqCount += count;
      count = 0;
      std::cout << "Total requests: " << reqCount << std::endl;
    }
    int n = write(newsockfd, "\x14\x00I got your message", 20);
    if (n < 20) {
      error("ERROR writing to socket");
      std::cout << "Still open: " << --connCount << std::endl;
      close(newsockfd);
      return;
    }
  }
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
    std::thread(work, newsockfd).detach();
  }
  close(sockfd);
  return 0; 
}

