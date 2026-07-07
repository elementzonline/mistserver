#include <mist/urireader.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

namespace{
  size_t countOpenFds(){
    long maxFd = sysconf(_SC_OPEN_MAX);
    if (maxFd < 0 || maxFd > 4096){maxFd = 4096;}

    size_t count = 0;
    for (long fd = 0; fd < maxFd; ++fd){
      errno = 0;
      if (fcntl((int)fd, F_GETFD) != -1 || errno != EBADF){++count;}
    }
    return count;
  }

  bool readFileThroughURIReader(const std::string &path){
    HTTP::URIReader reader(path);
    if (!reader){
      std::cerr << "URIReader failed to open " << path << std::endl;
      return false;
    }

    char *data = 0;
    size_t len = 0;
    reader.readAll(data, len);
    const std::string expected = "uri-reader-cleanup\n";
    if (len != expected.size() || std::string(data, len) != expected){
      std::cerr << "Unexpected read result length=" << len << std::endl;
      return false;
    }
    return true;
  }
}// namespace

int main(){
  char templatePath[] = "/tmp/mist-uri-reader-cleanup-XXXXXX";
  int fd = mkstemp(templatePath);
  if (fd == -1){
    std::cerr << "mkstemp failed: " << strerror(errno) << std::endl;
    return 1;
  }

  const char payload[] = "uri-reader-cleanup\n";
  ssize_t written = write(fd, payload, sizeof(payload) - 1);
  if (written != (ssize_t)(sizeof(payload) - 1)){
    std::cerr << "write failed: " << strerror(errno) << std::endl;
    close(fd);
    unlink(templatePath);
    return 1;
  }
  close(fd);

  const size_t before = countOpenFds();
  for (size_t i = 0; i < 32; ++i){
    if (!readFileThroughURIReader(templatePath)){
      unlink(templatePath);
      return 1;
    }
  }
  const size_t after = countOpenFds();

  unlink(templatePath);

  if (after != before){
    std::cerr << "URIReader leaked file descriptors: before=" << before
              << " after=" << after << std::endl;
    return 1;
  }

  return 0;
}
