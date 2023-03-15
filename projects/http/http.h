#include<iostream>
#include<string>
#include<vector>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
 
//封装TCPsocket类
#define MAX_LISTEN 5
#define CHECK_RETURN(X) if((X) == false) {return -1;}
 
class TCPsocket {
  private:
    int _sockfd;
  public:
    TCPsocket () : _sockfd(-1) {}
 
    //1.创建套接字
    bool Socket() {
      _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (_sockfd < 0) {
        perror("create socket error!");
        return false;
      }
      return true;
    } 
 
    //2.为套接字绑定地址信息
    bool Bind(const std::string &ip, uint16_t port) {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(ip.c_str());
      socklen_t len = sizeof(struct sockaddr_in);
 
      int ret = bind(_sockfd, (struct sockaddr*)&addr, len);
      if (ret < 0) {
        perror("bind error!");
        return false;
      }
      return true;
    }
 
    //客户端：3.向服务器发起连接请求
    bool Connect(const std::string &ip, uint16_t port) {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(ip.c_str());
      socklen_t len = sizeof(struct sockaddr_in);
      
      int ret = connect(_sockfd, (struct sockaddr*)&addr, len);
      if (ret < 0) {
        perror("connect error!");
        return false;
      }
      return true;
    }
 
    //服务端：3.开始监听
    bool Listen(int backlog = MAX_LISTEN) {
      int ret = listen(_sockfd, backlog);
      if (ret < 0) {
        perror("connect error!");
        return false;
      }
      return true;
    }
 
    //服务端：4. 获取新建连接
    bool Accept(TCPsocket *sock, std::string *ip = NULL, uint16_t *port = NULL) {
      struct sockaddr_in addr;
      socklen_t len = sizeof(struct sockaddr_in);
 
      int newfd = accept(_sockfd, (struct sockaddr*)&addr, &len);
      if (newfd < 0) {
        perror("accept error!");
        return false;
      }
      sock -> _sockfd = newfd;
 
      if (ip != NULL) *ip = inet_ntoa(addr.sin_addr);
      if (port != NULL) *port = ntohs(addr.sin_port);
 
      return true;
    }
 
    //4. 接收数据
    bool Recve(std::string *body) {
      char temp[8193] = {0};
 
      int ret = recv(_sockfd, temp, 8192, 0);
      if (ret < 0) {
        perror("recve error!");
        return false;
      }
      else if (ret == 0) {
        //std::cout<<"peer shutdown!"<< std::endl;
        std::cout<<"recv 0!"<< std::endl;
        //return false;
      }
 
      body -> assign(temp, ret);
      return true;
    }
 
    //5.发送数据
    bool Send(const std::string &body) {
      int ret = send(_sockfd, body.c_str(), body.size(), 0);
      if (ret < 0) {
        perror("send error!");
        return false;
      }
      return true;
    }
 
    //6.关闭套接字
    bool Close() {
      if (_sockfd != -1) close(_sockfd);
      return true;
    }
};
