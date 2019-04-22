#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
using std::vector;
using std::cout;
using std::endl;
using std::ofstream;

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[1000];
} segment;

// usage ./recv receiverIP receiverPort agentIP agentPort fileName
int main(int argc, char* argv[]) {
    int receiverSocket;
    int receiverPort;
		receiverPort = atoi(argv[2]);
    int agentPort;
		agentPort = atoi(argv[4]);
    char ipReceiver[50];
		strcpy(ipReceiver, argv[1]);
    char ipAgent[50];
		strcpy(ipAgent, argv[3]);
    struct sockaddr_in agent, receiver, tempAddr;
    socklen_t agentSize, receiverSize, tempSize;

		// agent config
		agent.sin_family = AF_INET;
    agent.sin_port = htons(agentPort);
    agent.sin_addr.s_addr = inet_addr(ipAgent);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));
		agentSize = sizeof(agent);

    // receiver config
    receiverSocket = socket(PF_INET, SOCK_DGRAM, 0);
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(receiverPort);
    receiver.sin_addr.s_addr = inet_addr(ipReceiver);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero));
		bind(receiverSocket,(struct sockaddr *)&receiver,sizeof(receiver));
		receiverSize = sizeof(receiver);

    //size
		tempSize = sizeof(tempAddr);
    int segmentSize;

    // setting
		int buffSize = 32;
		vector<segment> buffer;
		int waitAckedNum = 0;
    segment rTemp;
		ofstream isWrite(argv[5], ofstream::binary| ofstream::out | ofstream:: trunc);
    while(true) {
			memset(&rTemp, 0, sizeof(rTemp));
      segmentSize = recvfrom(receiverSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&tempAddr, &tempSize);
			if(segmentSize > 0) {
				if(rTemp.head.fin == 1) {
					printf("recv  fin\n");
					for(int i = 0; i < buffer.size()-1; i++) {
						isWrite.write(buffer[i].data, sizeof(buffer[i].data));
					}
					isWrite.write(buffer[buffer.size()-1].data, 529);
					isWrite.flush();
					buffer.clear();
					memset(&rTemp, 0, sizeof(rTemp));
					rTemp.head.length = 1000;
					rTemp.head.seqNumber = -1;
					rTemp.head.ackNumber = -1;
					rTemp.head.fin = 1;
					rTemp.head.syn = 0;
					rTemp.head.ack = 1;
					printf("send  finack\nflush\n");
					sendto(receiverSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&agent,agentSize);
					break;
				}
				if(buffer.size() == 32) {
					printf("drop  data  #%d\n", rTemp.head.seqNumber);
					for(int i = 0; i < buffer.size(); i++) {
						isWrite.write(buffer[i].data, sizeof(buffer[i].data));
					}
					isWrite.flush();
					buffer.clear();
					memset(&rTemp, 0, sizeof(rTemp));
					rTemp.head.length = 1000;
					rTemp.head.seqNumber = -1;
					rTemp.head.ackNumber = waitAckedNum-1;
					rTemp.head.fin = 0;
					rTemp.head.syn = 0;
					rTemp.head.ack = 1;
					printf("send  ack   #%d\nflush\n", waitAckedNum-1);
					sendto(receiverSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&agent,agentSize);
					continue;
				}
				if(rTemp.head.seqNumber == waitAckedNum) {
					printf("recv  data  #%d\n", waitAckedNum);
					buffer.push_back(rTemp);
					memset(&rTemp, 0, sizeof(rTemp));
					rTemp.head.length = 1000;
					rTemp.head.seqNumber = -1;
					rTemp.head.ackNumber = waitAckedNum;
					rTemp.head.fin = 0;
					rTemp.head.syn = 0;
					rTemp.head.ack = 1;
					printf("send  ack   #%d\n", waitAckedNum);
					sendto(receiverSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&agent,agentSize);
					waitAckedNum++;
				} else {
					printf("drop  data  #%d\n", rTemp.head.seqNumber);
					memset(&rTemp, 0, sizeof(rTemp));
					rTemp.head.length = 1000;
					rTemp.head.seqNumber = -1;
					rTemp.head.ackNumber = waitAckedNum-1;
					rTemp.head.fin = 0;
					rTemp.head.syn = 0;
					rTemp.head.ack = 1;
					printf("send  ack   #%d\n", waitAckedNum-1);
					sendto(receiverSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&agent,agentSize);
				}
			}
    }
    return 0;







}
