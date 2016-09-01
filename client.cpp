#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <climits>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <algorithm>

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
  std::vector<uint64_t> latencies;
  latencies.reserve(nrReq);
  std::chrono::high_resolution_clock clock;

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
  auto completeStartTime = clock.now();
  for (size_t i = 0; i < nrReq; i++) {
    auto startTime = clock.now();
    int res = performRequest(sockfd, buffer, rcvbuf);
    auto endTime = clock.now();
    latencies.push_back(timeDiff(startTime, endTime));
    if (res < 0) {
      error("ERROR in request");
    }
    //std::cout << "Got result of length " << res << std::endl;
  }
  auto completeEndTime = clock.now();
  close(sockfd);
  std::sort(latencies.begin(), latencies.end());
  uint64_t completeTime = timeDiff(completeStartTime, completeEndTime);
  {
    std::lock_guard<std::mutex> guard(outMutex);
    std::cout << "Done " << nrReq << " in "
        << completeTime / 1000 << " us,\n"
        << "    avg: " << (double) completeTime / (double) nrReq << " ns"
        << " 50%: " << latencies[nrReq * 50 / 100]
        << " 95%: " << latencies[nrReq * 95 / 100]
        << " 99%: " << latencies[nrReq * 99 / 100]
        << " 99.9%: " << latencies[nrReq * 999 / 1000]
        << std::endl;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 6) {
    std::cerr << "Usage " << argv[0]
              << " hostname port nrReq sizeReq nrThreads" 
              << std::endl;
    exit(0);
  }
  int portno = std::stoi(argv[2]);
  size_t nrReq = std::stoul(argv[3]);
  size_t sizeReq = std::stoul(argv[4]);
  int nrThreads = std::stoi(argv[5]);
  std::vector<std::thread> threads;
  std::chrono::high_resolution_clock clock;
  auto start = clock.now();
  for (int i = 0; i < nrThreads; i++) {
    threads.push_back(std::thread(work, argv[1], portno, nrReq, sizeReq));
  }
  for (int i = 0; i < nrThreads; i++) {
    threads[i].join();
  }
  auto finish = clock.now();
  uint64_t t = timeDiff(start, finish);
  std::cout << "Performed " << nrThreads * nrReq
            << " of size " << sizeReq << " for a total of "
            << nrThreads * nrReq * sizeReq << " bytes in "
            << t / 1000000.0 << " ms" << std::endl;
  std::cout << "This amounts to "
    << (double) (nrThreads * nrReq) / (t / 1000000000.0)
    << " request per second or "
    << (double) (nrThreads * nrReq * sizeReq) / (t / 1000000000.0)
    << " bytes per second." << std::endl;
  return 0;
}

