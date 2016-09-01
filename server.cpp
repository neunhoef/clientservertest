#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <climits>
#include <atomic>
#include <vector>
#include <mutex>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"

std::atomic<int> connCount(0);
std::atomic<uint64_t> reqCount(0);

void work(int newsockfd, uint64_t latsize, size_t outSize) {
  std::vector<uint64_t> latencies;
  latencies.reserve(latsize);
  std::chrono::high_resolution_clock clock;

  char buffer[16384];
  memset(buffer, 0, 16384);
  char outBuffer[16384];
  outBuffer[0] = (char) (outSize & 0xff);
  outBuffer[1] = (char) ((outSize >> 8) & 0xff);
  for (size_t i = 0; i < outSize; i++) {
    outBuffer[i+2] = (char) (i & 0xff);
  }
  std::cout << "New connection, currently " << ++connCount << " open."
            << std::endl;
  auto completeStartTime = clock.now();
  while (true) {
    int pos = getMsg(newsockfd, buffer);
    if (pos < 0) {
      break;
    }
    buffer[pos] = 0;
    auto startTime = clock.now();
    int n = sendMsg(newsockfd, outBuffer, outSize + 2);
    if (n < 0) {
      error("ERROR writing to socket");
      break;
    }
    auto endTime = clock.now();
    latencies.push_back(timeDiff(startTime, endTime));
  }
  auto completeEndTime = clock.now();
  close(newsockfd);
  std::sort(latencies.begin(), latencies.end());
  uint64_t completeTime = timeDiff(completeStartTime, completeEndTime);
  uint64_t nrReq = latencies.size();
  uint64_t timeSum = 0;
  for (size_t i = 0; i < latencies.size(); i++) {
    timeSum += latencies[i];
  }
  {
    std::lock_guard<std::mutex> guard(outMutex);
    std::cout << "Done " << nrReq << " in "
        << completeTime / 1000 << " us,";
    if (nrReq > 0) {
      std::cout
        << "\n    avg: " << (double) timeSum / (double) nrReq << " ns"
        << " 50%: " << latencies[nrReq * 50 / 100]
        << " 95%: " << latencies[nrReq * 95 / 100]
        << " 99%: " << latencies[nrReq * 99 / 100]
        << " 99.9%: " << latencies[nrReq * 999 / 1000];
    }
    std::cout << "\n    Still open: " << --connCount << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " PORT LATENCYBUFSIZE MSGSIZE"
              << std::endl;
    return 1;
  }
  uint64_t latBufSize = std::stoul(argv[2]);
  size_t msgSize = std::stoul(argv[3]);
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
    std::thread(work, newsockfd, latBufSize, msgSize).detach();
  }
  close(sockfd);
  return 0; 
}

