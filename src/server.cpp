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

#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>

class HttpRequest {
    public:
        // Constructor
        HttpRequest(const std::string& rawRequest) {
            parse(rawRequest);
        }

        // Getters
        std::string getMethod() const { return method; }
        std::string getPath() const { return path; }
        std::string getVersion() const { return version; }
        std::string getHeader(const std::string& key) const {
          auto it = headers.find(key);
          return it != headers.end() ? it->second : "";
        }
    private:
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;

        // Parse the request
        void parse(const std::string& raw_request) {
            std::istringstream stream(raw_request);
            std::string line;

            // Parse the request line
            if (std::getline(stream, line)) {
                std::istringstream request_line_stream(line);
                request_line_stream >> method >> path >> version;

            }

            // Parse headers
            // Checking that we have succesffuly read a line and its not empty
            while (std::getline(stream, line) && !line.empty()) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {

                    // Capturing value
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);

                    value.erase(0, value.find_first_not_of(" \r"));
                    value.erase(value.find_last_not_of(" \r") + 1);
                    headers[key] = value;


                }
            }

            // Parse body (if any)
            std::ostringstream bodyStream;
            while (std::getline(stream, line)) {
                bodyStream << line << "\n";
            } 
            body = bodyStream.str();
            if (!body.empty()) {
                body.pop_back(); 
            }
        }

};

void send_response(const std::string& received_message, int client){

  HttpRequest request(received_message);
  std::string method = request.getMethod();
  std::string path = request.getPath();

  std::smatch match;
  const std::regex echo_pattern(R"(\/echo\/([^/]+))");

  std::string header_user_agent = request.getHeader("User-Agent");
  std::cout << header_user_agent[header_user_agent.length()-2] << "\n";


  if (method == "GET") {
    // Parse get method and echo method
    if (path == "/") {
      std::string message = "HTTP/1.1 200 OK\r\n\r\n";
      send(client, message.c_str(), message.length(), 0);
      
    } else if (std::regex_match(path, match, echo_pattern)) {
      std::string message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
      std::string format_string = std::to_string(match[1].length()) + "\r\n\r\n" + match[1].str(); 
      message = message + format_string;
      send(client, message.c_str(), message.length(), 0);

    } else if (path == "/user-agent") {
      std::string message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
      std::string format_string = std::to_string(header_user_agent.length()) + "\r\n\r\n" + header_user_agent;
      message = message + format_string;
      std::cout << message << "\n";
      send(client, message.c_str(), message.length(), 0);

    } else { 
      std::string message = "HTTP/1.1 404 Not Found\r\n\r\n";
      send(client, message.c_str(), message.length(), 0);
    }
  } else {
    std::string message = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    send(client, message.c_str(), message.length(), 0);
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
  send_response(received_message, client);

  std::cout << "Message sent!\n";

  close(server_fd);

  return 0;
}
