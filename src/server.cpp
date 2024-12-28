#include <iostream>
#include <regex>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

void extract_url_path(const std::string& received_message, int client){
  std::istringstream stream(received_message);

  std::string request_line;
  std::getline(stream, request_line);
  std::cout << "Request Line: " << request_line << std::endl;
  std::istringstream line_stream(request_line);
  
  std::string method, target, version;

  // Using the string stream to parse each part
  line_stream >> method >> target >> version;

  if (method == "GET") {
    if (target == "/") {
      std::string message = "HTTP/1.1 200 OK\r\n\r\n";
      send(client, message.c_str(), message.length() + 1, 0);
    } else { 
      std::string message = "HTTP/1.1 404 Not Found\r\n\r\n";
      send(client, message.c_str(), message.length() + 1, 0);
    }
  } else {
    std::string message = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    send(client, message.c_str(), message.length() + 1, 0);
  }
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  /* Creating a socket descripter to do socket stuff with
  domain - describes the socket you want (IPv4)
  type - Controls if you want a stream socket or a datagram socket
  protocol - Tells which protocol you want to use, (set to 0 as can infer from SOCK_STREAM)
  */
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;

  // Address family, allows you to connect to only addresses in that family (IPv4)
  server_addr.sin_family = AF_INET;

  // Listens to all ip addresses asigned to the host machine
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // host to network order conversion. Different machines use different byte orders
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n to " << client << "\n";

  // Receiving message
  std::string received_message(256, '\0');
  int bytes_received = recv(client, received_message.data(), received_message.size(), 0);

  // 1. Find first line, if between GET and HTTP there is a /, send 200, else send 404
  extract_url_path(received_message, client);

  std::cout << "Message sent!\n";

  close(server_fd);

  return 0;
}
