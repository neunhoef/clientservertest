typedef std::chrono::high_resolution_clock::time_point timePointType;
typedef std::chrono::duration<uint64_t, std::nano>     nanoSecondsType;

static inline uint64_t timeDiff (timePointType& a, timePointType& b) {
      return std::chrono::duration_cast<nanoSecondsType>(b-a).count();
}

void error(const char *msg) {
  std::cout << msg << "\n" << strerror(errno) << std::endl;
}

char sendBuf[16384];

int sendMsg(int fd, char const* buf, size_t len) {
  if (len > 16382) {
    return -1;
  }
  len += 2;
  sendBuf[0] = (char) (len & 0xff);
  sendBuf[1] = (char) ((len >> 8) & 0xff);
  memcpy(sendBuf + 2, buf, len);
  size_t pos = 0;
  while (pos < len) {
    int n = write(fd, sendBuf + pos, len - pos);
    if (n <= 0) {
      error("ERROR writing to socket");
      return -2;
    }
    pos += n;
  }
  return 0;
}

int getMsg(int fd, char buffer[16384]) {
  int pos = 0;
  int expected = INT_MAX;
  while (true) {
    int n = read(fd, buffer + pos, 16384 - pos);
    if (n <= 0) {
      return -1;
    }
    pos += n;
    if (expected == INT_MAX && pos >= 2) {
      expected = (int) buffer[1] * 128 + buffer[0];
    }
    if (pos >= expected) {
      break;
    }
  }
  return pos;
}

