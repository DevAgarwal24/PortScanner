#include <iostream>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>

#include <thread>
#include <vector>

bool isIpAddress(const std::string& str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str.c_str(), &(sa.sin_addr)) != 0;
}

void connectToServer(struct sockaddr_in serverAddr, int port) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Initialize server address structure
    serverAddr.sin_port = htons(port);

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 0.5;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Sockopt error\n";
        return;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0) {
        std::cout << "Port: " << port << " is open\n";
    }

    close(sockfd);
}

void connectToServerMultiplePorts(struct sockaddr_in serverAddr, int portStart, int portEnd) {
    for (int port = portStart; port < portEnd; port++) {
        // Initialize server address structure
        serverAddr.sin_port = htons(port);

        connectToServer(serverAddr, port);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Missing arguments. Use -h or --help for help.\n";
        return 1;
    }

    std::string hostname{};
    int port{};

    std::string hostOpt = argv[1];
    
    if (hostOpt.find("-host=") != std::string::npos) {
        hostname = hostOpt.substr(hostOpt.find("-host=")+6);
    } else {
        std::cerr << "Incorrect arguments. Use -h or --help for help.\n";
        return 1;
    }

    if (argc == 3) {
        std::string portOpt = argv[2];
        if (portOpt.find("-port=") != std::string::npos) {
            port = stoi(portOpt.substr(portOpt.find("-port=")+6));
        } else {
            std::cerr << "Incorrect arguments. Use -h or --help for help.\n";
            return 1;
        }
    }

    // std::cout << "Hostname: " << hostname << std::endl;
    // std::cout << "Port: " << ((port == 0) ? "All Ports" : std::to_string(port)) << std::endl;

    struct sockaddr_in serverAddr;
    struct hostent *server;

    if (isIpAddress(hostname)) {
        // Input is an IP address
        serverAddr.sin_family = AF_INET;
        inet_pton(AF_INET, hostname.c_str(), &serverAddr.sin_addr);
    } else {
        // Input is a hostname, resolve it
        server = gethostbyname(hostname.c_str());
        if (server == NULL) {
            std::cerr << "Error resolving hostname" << std::endl;
            return 1;
        }
        serverAddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
    }

    if (port != 0) {
        connectToServer(serverAddr, port);
    } else { // Scan ports in different threads
        std::vector<std::thread> threads;
        for (int i = 1; i < 65535; i+=255) {
            threads.emplace_back(std::thread(connectToServerMultiplePorts, serverAddr, i, i+255));
        }

        for (std::thread& t : threads) {
            t.join();
        }
    }

    return 0;
}