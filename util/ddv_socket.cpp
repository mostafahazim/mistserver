#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

bool socketError = false;
bool socketBlocking = false;

int DDV_OpenUnix(std::string adres, bool nonblock = false){
  int s = socket(PF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, adres.c_str(), adres.size()+1);
  int r = connect(s, (sockaddr*)&addr, sizeof(addr));
  if (r == 0){
    if (nonblock){
      int flags = fcntl(s, F_GETFL, 0);
      flags |= O_NONBLOCK;
      fcntl(s, F_SETFL, flags);
    }
    return s;
  }else{
    close(s);
    return 0;
  }
}

int DDV_Listen(int port){
  int s = socket(AF_INET, SOCK_STREAM, 0);
  int on = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);//port 8888
  inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);//listen on all interfaces
  int ret = bind(s, (sockaddr*)&addr, sizeof(addr));//bind to all interfaces, chosen port
  if (ret == 0){
    ret = listen(s, 100);//start listening, backlog of 100 allowed
    if (ret == 0){
      return s;
    }else{
      fprintf(stderr, "Listen failed! Error: %s\n", strerror(errno));
      close(s);
      return 0;
    }
  }else{
    fprintf(stderr, "Binding failed! Error: %s\n", strerror(errno));
    close(s);
    return 0;
  }
}

int DDV_UnixListen(std::string adres, bool nonblock = false){
  unlink(adres.c_str());
  int s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (nonblock){
    int flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(s, F_SETFL, flags);
  }
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, adres.c_str(), adres.size()+1);
  int ret = bind(s, (sockaddr*)&addr, sizeof(addr));
  if (ret == 0){
    ret = listen(s, 100);//start listening, backlog of 100 allowed
    if (ret == 0){
      return s;
    }else{
      fprintf(stderr, "Listen failed! Error: %s\n", strerror(errno));
      close(s);
      return 0;
    }
  }else{
    fprintf(stderr, "Binding failed! Error: %s\n", strerror(errno));
    close(s);
    return 0;
  }
}

int DDV_Accept(int sock, bool nonblock = false){
  int r = accept(sock, 0, 0);
  if ((r >= 0) && nonblock){
    int flags = fcntl(r, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(r, F_SETFL, flags);
  }
  return r;
}

bool DDV_write(void * buffer, int todo, int sock){
  int sofar = 0;
  socketBlocking = false;
  while (sofar != todo){
    int r = send(sock, (char*)buffer + sofar, todo-sofar, 0);
    if (r <= 0){
      switch (errno){
        case EWOULDBLOCK: socketBlocking = true; break;
        default:
          socketError = true;
          fprintf(stderr, "Could not write! %s\n", strerror(errno));
          return false;
          break;
      }
    }
    sofar += r;
  }
  return true;
}

signed int DDV_ready(int sock){
  char tmp;
  int preflags = fcntl(sock, F_GETFL, 0);
  int postflags = preflags | O_NONBLOCK;
  fcntl(sock, F_SETFL, postflags);
  int r = recv(sock, &tmp, 1, MSG_PEEK);
  fcntl(sock, F_SETFL, preflags);
  return r;
}

bool DDV_read(void * buffer, int todo, int sock){
  int sofar = 0;
  socketBlocking = false;
  while (sofar != todo){
    int r = recv(sock, (char*)buffer + sofar, todo-sofar, 0);
    if (r <= 0){
      switch (errno){
        case EWOULDBLOCK: socketBlocking = true; break;
        default:
          socketError = true;
          fprintf(stderr, "Could not read! %s\n", strerror(errno));
          return false;
          break;
      }
    }
    sofar += r;
  }
  return true;
}


bool DDV_read(void * buffer, int width, int count, int sock){return DDV_read(buffer, width*count, sock);}
bool DDV_write(void * buffer, int width, int count, int sock){return DDV_write(buffer, width*count, sock);}


int DDV_iwrite(void * buffer, int todo, int sock){
  int r = send(sock, buffer, todo, 0);
  if (r < 0){
    switch (errno){
      case EWOULDBLOCK: return 0; break;
      default:
        socketError = true;
        fprintf(stderr, "Could not write! %s\n", strerror(errno));
        return 0;
        break;
    }
  }
  return r;
}

int DDV_iread(void * buffer, int todo, int sock){
  int r = recv(sock, buffer, todo, 0);
  if (r < 0){
    switch (errno){
      case EWOULDBLOCK: break;
      default:
        socketError = true;
        fprintf(stderr, "Could not read! %s\n", strerror(errno));
        return 0;
        break;
    }
  }
  return r;
}


