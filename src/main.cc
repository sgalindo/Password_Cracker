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
#include <vector>
#include <thread>

#define NUM_SERVERS 4

bool master = false;
char servers[NUM_SERVERS][64];
static unsigned int myIndex = 0;

void convertReceive(Message &message) {
    message.num_passwds = ntohl(message.num_passwds);
    message.port = ntohl(message.port);
}

void convertSend(Message &message) {
    message.num_passwds = htonl(message.num_passwds);
    message.port = htonl(message.port);
}

void receiveMessage(Message &message) {
    // UDP MULTICAST ----------------------------------------------------------------------------------------------------------
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Cracker socket creation failed");
        exit(-1);
    }

    struct sockaddr_in serverAddress;
    bzero((char *) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(get_multicast_port());

    if (bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("Cracker socket bind failed");
        exit(-1);
    }

    struct ip_mreq multicastRequest;
    multicastRequest.imr_multiaddr.s_addr = get_multicast_address();
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &multicastRequest, sizeof(multicastRequest)) < 0) {
        perror("Cracker socket options failed");
        exit(-1);
    }

    int rec = recvfrom(sockfd, (void *) &message, sizeof(message), 0, NULL, 0);
    if (rec < 0) {
        perror("Message receive failed");
        exit(-1);
    }
    //convertReceive(message);
    message.num_passwds = ntohl(message.num_passwds);
    //std::cout << "Cracker received: " << message.num_passwds << " hashes on Port: " << message.port << std::endl;
    close(sockfd);
}

void sendMessage(Message &message) {
    // TCP UNICAST ---------------------------------------------------------------------------------------------------------
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Cracker send socket creation failed");
        exit(-1);
    }

    struct hostent *server = gethostbyname(message.hostname);
    //std::cout << "Sending to: " << message.hostname << std::endl;
	if (server == NULL) {
		perror("Cracker could not obtain host address");
		exit(-1);
	}

    struct sockaddr_in sendAddress;
    bzero((char *) &sendAddress, sizeof(sendAddress));
    sendAddress.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *)&sendAddress.sin_addr.s_addr, server->h_length);
    sendAddress.sin_port = message.port;

    //std::cout << "Sending on port: " << message.port << std::endl;

    convertSend(message);

    if (connect(sockfd, (struct sockaddr *) &sendAddress, sizeof(sendAddress)) < 0) {
        perror("Cracker send connection failed");
        exit(-1);
    }
    int s = send(sockfd, (void *) &message, sizeof(message), 0);
    if (s < 0) {
        perror("Cracker send failed");
        exit(-1);
    }
    //std::cout << "Cracker sent: " << ntohl(message.num_passwds) << " items on Port: " << ntohl(message.port) << std::endl;
    close(sockfd);
}

void receiveInnerMessage(Message &message) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("SubCracker receive socket creation failed");
        exit(-1);
    }
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in clientAddress;

    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = INADDR_ANY;
    clientAddress.sin_port = htons(message.port);

    if (bind(sockfd, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) {
        perror("SubCracker receive socket bind failed");
        exit(-1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("SubCracker receive socket listen failed");
        exit(-1);
    }
    struct sockaddr_in recvAddress;
    socklen_t length = sizeof(recvAddress);
    int recvSockfd = accept(sockfd, (struct sockaddr *) &recvAddress, &length);
    if (recvSockfd < 0) {
        perror("SubCracker receive socket accept failed");
        exit(-1);
    }

    int rec = recv(recvSockfd, (void *) &message, sizeof(message), 0);
    if (rec < 0) {
        perror("SubCracker receive failed");
        exit(-1);
    }
    convertReceive(message);
    //std::cout << "Received: "<< message.num_passwds << " on Port: " << message.port << std::endl;
    
    close(sockfd);
    close(recvSockfd);
}

void sendInnerMessage(Message &message) {
    // TCP UNICAST ---------------------------------------------------------------------------------------------------------
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("SubCracker send socket creation failed");
        exit(-1);
    }

    struct hostent *server = gethostbyname("graculus");
    //std::cout << "graculus" << std::endl;
	if (server == NULL) {
		perror("SubCracker could not obtain host address");
		exit(-1);
	}

    struct sockaddr_in sendAddress;
    bzero((char *) &sendAddress, sizeof(sendAddress));
    sendAddress.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *)&sendAddress.sin_addr.s_addr, server->h_length);
    sendAddress.sin_port = htons(message.port);

    convertSend(message);

    if (connect(sockfd, (struct sockaddr *) &sendAddress, sizeof(sendAddress)) < 0) {
        perror("SubCracker send connection failed");
        exit(-1);
    }
    int s = send(sockfd, (void *) &message, sizeof(message), 0);
    if (s < 0) {
        perror("SubCracker send failed");
        exit(-1);
    }
    //std::cout << "SubCracker sent: " << ntohl(message.num_passwds) << " index on Port: " << ntohl(message.port) << std::endl;
    close(sockfd);
}

/*void crackSegment(Message &message, unsigned int i, unsigned int segment, char *tokens, bool &cracked) {
    while (cracked == false) {

    }
}

void crackAll(Message &message) {
    std::vector<std::thread> threads;
    
    char tokens[63];
    strcpy(tokens, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    unsigned int numSegments = 62 / message.num_passwds;
    for (unsigned int i = 0; i < message.num_passwds; i++) {
        bool cracked = false;
        for (unsigned int j = 0; j < numSegments; j++) {
            threads.push_back(crackSegment, std::ref(message), i, j, tokens, cracked);
        }
    }
}*/

void crackPassword(Message &message, const unsigned int i) {
    
    //std::cout << "Cracking hash: " << message.passwds[i] << std::endl;
    char pw[5];
    crack(message.passwds[i], pw);
    strcpy(message.passwds[i], pw);
    //std::cout << "    Password cracked: " << message.passwds[i] << std::endl;
}

void crackAll(Message &message) {
    unsigned int sizes[NUM_SERVERS] = {0,0,0,0};
    unsigned int i = 0;
    while (i < message.num_passwds) {
        unsigned int s = 0;
        while (s < NUM_SERVERS && i < message.num_passwds) {
            sizes[s++] += 1;
            i++;
        }
    }
    Message innerMessage;
    innerMessage.port = get_unicast_port();
    std::vector<std::thread> threads;
    if (master) {
        for (unsigned int i = 0; i < sizes[0]; i++) {
            threads.push_back(std::thread(crackPassword, std::ref(message), i));
        }
        bool sorted = false;
        if (message.num_passwds > 1) {
            unsigned int received = (message.num_passwds >= NUM_SERVERS) ? 0 : NUM_SERVERS - message.num_passwds;
            while (!sorted) {
                receiveInnerMessage(innerMessage);
                if (innerMessage.num_passwds == 1) {
                    for (unsigned int i = 0; i < sizes[1]; i++) {
                        strcpy(message.passwds[i+sizes[0]], innerMessage.passwds[i+sizes[0]]);
                    }
                }
                else if (innerMessage.num_passwds == 2) {
                    for (unsigned int i = 0; i < sizes[2]; i++) {
                        strcpy(message.passwds[i+sizes[0]+sizes[1]], innerMessage.passwds[i+sizes[0]+sizes[1]]);
                    }
                }
                else if (innerMessage.num_passwds == 3) {
                    for (unsigned int i = 0; i < sizes[3]; i++) {
                        strcpy(message.passwds[i+sizes[0]+sizes[1]+sizes[2]], innerMessage.passwds[i+sizes[0]+sizes[1]+sizes[2]]);
                    }
                }
                received++;
                if (received == NUM_SERVERS-1) {
                    sorted = true;
                }
            } 
        }  
        for (std::thread &thread: threads) {
            thread.join();
        }
    }
    else if (myIndex == 1 && myIndex < message.num_passwds) {
        for (unsigned int i = 0; i < sizes[1]; i++) {
            threads.push_back(std::thread(crackPassword, std::ref(message), i+sizes[0]));
        }
        for (std::thread &thread: threads) {
            thread.join();
        }
        strcpy(message.hostname, "thor");
        message.port = get_unicast_port();
        message.num_passwds = myIndex;
        sendInnerMessage(message);
    }
    else if (myIndex == 2 && myIndex < message.num_passwds) {
        for (unsigned int i = 0; i < sizes[2]; i++) {
            threads.push_back(std::thread(crackPassword, std::ref(message), i+sizes[0]+sizes[1]));
        }
        for (std::thread &thread: threads) {
            thread.join();
        }
        strcpy(message.hostname, "olaf");
        message.port = get_unicast_port();
        message.num_passwds = myIndex;
        sendInnerMessage(message);  
    }
    else if (myIndex == 3 && myIndex < message.num_passwds) {
        for (unsigned int i = 0; i < sizes[3]; i++) {
            threads.push_back(std::thread(crackPassword, std::ref(message), i+sizes[0]+sizes[1]+sizes[2]));
        }
        for (std::thread &thread: threads) {
            thread.join();
        }
        strcpy(message.hostname, "grolliffe");
        message.port = get_unicast_port();
        message.num_passwds = myIndex;
        sendInnerMessage(message);
    }
    //std::cout << "Cracking finished." << std::endl;
}

int initialize() {
    char name[64];
    unsigned int index = 0;
    gethostname(name, 63);
    //std::cout << "Running on server: " << name << std::endl;
    if (strcmp(name, "graculus") == 0) {
        master = true;
    }
    else if (strcmp(name, "thor") == 0) {
        index = 1;
    }
    else if (strcmp(name, "olaf") == 0) {
        index = 2;
    }
    else if (strcmp(name, "grolliffe") == 0) {
        index = 3;
    }
    //std::cout << "Master? " << master << std::endl;
    //std::cout << "Server index: " << index << std::endl;

    strcpy(servers[0], "graculus");
    strcpy(servers[1], "thor");
    strcpy(servers[2], "olaf");
    strcpy(servers[3], "grolliffe");

    return index;
}

int main(int argc, char *argv[]) {
    myIndex = initialize();
    Message message;
    for (;;) {
        receiveMessage(message);
        if (strcmp(message.cruzid, "sagalind") == 0) {
            crackAll(message);
            if (master) {
                sendMessage(message);
            }
        }
    }
}