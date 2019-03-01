// Salvador Galindo
// sagalind
// Professor David Harrison
// CMPS 109
// Lab 9
// -------------------------------------------------------------------------------------------------

#include "crack.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

void convertSend(Message &message) {
    message.num_passwds = htonl(message.num_passwds);
    message.port = htonl(message.port);
}

void convertRecv(Message &message) {
    message.num_passwds = ntohl(message.num_passwds);
    message.port = ntohl(message.port);
}

void sendMessage(Message &message) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Server socket creation failed");
        exit(-1);
    }
    int ttl = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &ttl, sizeof(ttl)) < 0) {
        perror("Server socket options failed");
        exit(-1);
    }
    
    struct sockaddr_in multicastAddress;
    memset(&multicastAddress, 0, sizeof(multicastAddress));
    multicastAddress.sin_family = AF_INET;
    multicastAddress.sin_addr.s_addr = get_multicast_address();
    multicastAddress.sin_port = htons(get_multicast_port());

    strcpy(message.cruzid, "sagalind");
    message.num_passwds = 6;
    strcpy(message.passwds[0], "xxo0q4QVK0mOg"); //cmps
    strcpy(message.passwds[1], "00Pp9Oy0VWmn2"); //ucsc
    strcpy(message.passwds[2], "zzOzL0bB0ocqo"); //lab9
    strcpy(message.passwds[3], "yyNhnfhEpDmTY"); //CMPS
    strcpy(message.passwds[4], "5tQvIqEDV1gzw"); //UCSC
    strcpy(message.passwds[5], "lqFucz6Kp.jPE"); //LAB9
    strcpy(message.hostname, "grolliffe");
    message.port = get_unicast_port();

    convertSend(message);
    
    int send = sendto(sockfd, (void *) &message, sizeof(message), 0, (struct sockaddr *) &multicastAddress, sizeof(multicastAddress));
    if (send < 0) {
        perror("Server multicast failed");
        exit(-1);
    }
    std::cout << "Server passwords sent: " << ntohl(message.num_passwds) << " on port: " << ntohl(message.port) << std::endl;
    close(sockfd);
}

void receiveMessage(Message &message) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Server receive socket creation failed");
        exit(-1);
    }
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in clientAddress;

    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = INADDR_ANY;
    clientAddress.sin_port = htons(get_unicast_port());

    if (bind(sockfd, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) {
        perror("Server receive socket bind failed");
        exit(-1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Server receive socket listen failed");
        exit(-1);
    }

    struct sockaddr_in recvAddress;
    socklen_t length = sizeof(recvAddress);
    int recvSockfd = accept(sockfd, (struct sockaddr *) &recvAddress, &length);
    if (recvSockfd < 0) {
        perror("Server receive socket accept failed");
        exit(-1);
    }

    int rec = recv(recvSockfd, (void *) &message, sizeof(message), 0);
    if (rec < 0) {
        perror("Server receive failed");
        exit(-1);
    }
    convertRecv(message);
    std::cout << "Server received: " << std::endl; 
    for (unsigned int i = 0; i < message.num_passwds; i++) {
        std::cout << "    " << message.passwds[i] << std::endl;
    }
    std::cout << "on Port: " << message.port << std::endl;
    
    close(sockfd);
    close(recvSockfd);
}

int main(int argc, char *argv[]) {
    Message message;
    sendMessage(message);
    for (;;) {
        receiveMessage(message);
    }
}