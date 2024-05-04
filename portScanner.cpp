#include <iostream>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <fcntl.h>

#include <thread>
#include <vector>
#include <regex>

bool isIpAddress(const std::string& str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str.c_str(), &(sa.sin_addr)) != 0;
}

std::vector<std::string> extractHostnames(const std::string &input) {
    std::vector<std::string> hosts;
    // std::regex pattern(R"(\b(?:\d{1,3}\.){3}\d{1,3}\b|\b(?:[a-zA-Z0-9\-]+\.)+[a-zA-Z]{2,}\b)");
    // std::regex pattern(R"(\b(?:\d{1,3}\.){3}\d{1,3}\b|\b(?:[a-zA-Z0-9\-]+\.)+[a-zA-Z]{2,}\b|\blocalhost\b)");
    // std::regex pattern(R"(\b(?:\d{1,3}|\*)\.(?:\d{1,3}|\*)\.(?:\d{1,3}|\*)\.(?:\d{1,3}|\*)\b|\b(?:[a-zA-Z0-9\-]+\.)+[a-zA-Z]{2,}\b|\blocalhost\b)");
	std::regex pattern(R"((?:(?:\d{1,3}|\*)\.){3}(?:\d{1,3}|\*)|\b(?:[a-zA-Z0-9\-]+\.)+[a-zA-Z]{2,}\b|\blocalhost\b)");

    std::sregex_iterator next(input.begin(), input.end(), pattern);
    std::sregex_iterator end;
    while (next != end) {
        std::smatch match = *next;
        hosts.push_back(match.str());
        next++;
    }

    return hosts;
}

std::vector<std::string> generateIPs(const std::string& baseIP) {
    std::vector<std::string> ips;

    std::string firstPart, secondPart;

    size_t pos = baseIP.find("*");
    if (pos != std::string::npos) {
        firstPart = baseIP.substr(0, pos);
        secondPart = baseIP.substr(pos + 1);
    }

    for (int i = 0; i < 256; i++) {
        ips.push_back(firstPart + std::to_string(i) + secondPart);
    }

    return ips;
}

void connectToServer(struct sockaddr_in serverAddr, int port) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // Initialize server address structure
    serverAddr.sin_port = htons(port);

    // Start connecting to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        if (errno == EINPROGRESS) {
            // Connection attempt is in progress, wait for it to complete or timeout
            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(sockfd, &writefds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 0.5 seconds

            int result = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
            if (result > 0 && FD_ISSET(sockfd, &writefds)) {
                // Connection successful, check for errors
                int err;
                socklen_t errlen = sizeof(err);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen);
                if (err == 0) {
                    std::cout << "Port: " << port << " is open\n";
                } else {
                    std::cerr << "Connection error: " << std::strerror(err) << std::endl;
                }
            } else {
                // std::cerr << "Connection timed out\n";
            }
        } else {
            // std::cerr << "Connect error: " << std::strerror(errno) << std::endl;
        }
    } else {
        // Connection successful (unlikely to reach here for non-blocking connect)
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

    std::string hostInput{};
    bool sweep = false;
    int port{};

    std::string hostOpt = argv[1];
    
    if (hostOpt.find("-host=") != std::string::npos) {
        hostInput = hostOpt.substr(hostOpt.find("-host=")+6);
    } else {
        std::cerr << "Incorrect arguments. Use -h or --help for help.\n";
        return 1;
    }

    std::vector<std::string> hostnames = extractHostnames(hostInput);
    if (hostnames.size() > 1) {
        sweep = true;
    }

    // for (const auto& hostname : hostnames) {
    //     std::cout << hostname << std::endl;
    // }

    // If the user is trying to do port sweep, then they shouldn't provide the port number to scan for.
    // The program will by default scan for specific ports in case of port sweep
    if (argc == 3 && !sweep) {
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

    std::vector<std::string> newHosts;

    for (const auto& hostname : hostnames) {
        if (hostname.find("*") != std::string::npos) {
            std::vector<std::string> tmpHosts = generateIPs(hostname);
            newHosts.insert(newHosts.end(), tmpHosts.begin(), tmpHosts.end());
            continue;
        }

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

        if (sweep) {
            std::cout << hostname << ": ";
            connectToServer(serverAddr, 443);
        } else {
            if (port != 0) {
                std::cout << "Scanning port " << port << std::endl;
                connectToServer(serverAddr, port);
            } else { // Scan ports in different threads
                std::cout << "Scanning all ports..." << std::endl;
                std::vector<std::thread> threads;
                for (int i = 1; i < 65535; i+=255) {
                    threads.emplace_back(std::thread(connectToServerMultiplePorts, serverAddr, i, i+255));
                }

                for (std::thread& t : threads) {
                    t.join();
                }
            }
        }
    }

    for (const auto& hostname : newHosts) {
        serverAddr.sin_family = AF_INET;
        inet_pton(AF_INET, hostname.c_str(), &serverAddr.sin_addr);

        std::cout << hostname << ": " << std::flush;
        connectToServer(serverAddr, 443);
        std::cout << "\n";
    }

    return 0;
}
