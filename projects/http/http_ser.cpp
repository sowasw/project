#include "http.h"
#include <sstream>
#include <fstream>
#include <cstring>
 
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout<<"./http_server  port"<< std::endl;
    return -1;
  }
 
  int srv_port = std::stoi(argv[1]);
  std::string srv_ip = "0.0.0.0";
 
  TCPsocket sock;
  //1.创建套接字
  CHECK_RETURN(sock.Socket());
  //2.绑定地址信息
  CHECK_RETURN(sock.Bind(srv_ip, srv_port));
  //3.开始监听
  CHECK_RETURN(sock.Listen());
  while(1) {
    TCPsocket new_sock;
    bool ret = sock.Accept(&new_sock);
    //4.获取新建连接
    if (ret == false) continue;
 
    //5.收发数据
    std::string rec;
    new_sock.Recve(&rec);
    size_t pos = rec.find("\r\n\r\n");//查找头部结尾标志
    if (pos == std::string::npos) {
      //没有找到，则认为头部过大
      //将响应状态码设置为414
      //这里简单实现，就直接错误返回了
      return -1;
    }
    std::string header = rec.substr(0,pos);//截取头部
    std::string body = rec.substr(pos + 4);//截取正文
    //正常的截图正文，应该将头部按照\r\n进行分割，逐个取出头部；
    //然后根据头部中的Content-Length确定正文长度，正文长度= rec.size() - header.size() - 4;
    //然后在截取正文
    std::cout<<"header:[" << header << "]" << std::endl;
    std::cout<< "body:[" << body << "]" << std::endl;
 
    //响应
    std::string rep_body = "<html><body><h1>Hello fcs 1!</h1></body></html>";
    
    //size_t port_pos = header.find("/");//查找头部结尾标志
    //pos = header.find("\r\n");
    std::string file = header.substr(header.find("/") + 1, header.find("HTTP") - 2 - strlen("HTTP"));
    std::string path = file;
    std::ifstream in;
    in.open(path.c_str());
    std::cout<< "path:[" << path << "]" << std::endl;
    
    std::string str;
    if (!in.fail()) {
        std::cout<< "path:[" << path << "]2" << std::endl;
        rep_body = "";
        while (std::getline(in, str)) {
            rep_body += str;
        }
        in.close();
    }
    
    std::stringstream ss_reply;
    ss_reply << "HTTP/1.1 200 OK\r\n";
    ss_reply << "Connection: close\r\n";
    ss_reply << "Content-Type: text/html\r\n";
    ss_reply << "Content-Length: " << rep_body.size() << "\r\n";
    ss_reply << "\r\n";
    ss_reply << rep_body;
    std::cout << "response：[" << ss_reply.str() << "]" << std::endl;
 
    new_sock.Send(ss_reply.str());
    //短连接，完成响应，关闭连接
    new_sock.Close();
  }
  //6.关闭套接字
  sock.Close();
  return 0;
}
