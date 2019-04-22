
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <algorithm>
using std::cout;
using std::endl;
using std::ifstream;
using std::vector;
using std::max;

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
// usage ./send senderIP senderPort agentIP agentPort fileName
int main(int argc, char* argv[]) {
	int senderSocket;
	int agentPort;
	agentPort = atoi(argv[4]);
	int senderPort;
	senderPort = atoi(argv[2]);
	char ipAgent[50];
	strcpy(ipAgent, argv[3]);
	char ipsender[50];
	strcpy(ipsender, argv[1]);
	struct sockaddr_in agent, sender, tempAddr;
	socklen_t agentSize, senderSize, tempSize;

	// agent config
	agent.sin_family = AF_INET;
	agent.sin_port = htons(agentPort);
	agent.sin_addr.s_addr = inet_addr(ipAgent);
	memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));
	agentSize = sizeof(agent);

	// sender config
	senderSocket = socket(PF_INET, SOCK_DGRAM, 0);
	sender.sin_family = AF_INET;
	sender.sin_port = htons(senderPort);
	sender.sin_addr.s_addr = inet_addr(ipsender);
	memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));
	senderSize = sizeof(sender);
	bind(senderSocket,(struct sockaddr *)&sender,sizeof(sender));

	// size
	tempSize = sizeof(tempAddr);
	int segmentSize;
	segment sTemp;
	ifstream isRead(argv[5], ifstream::binary);
	if(isRead.fail()) {
		cout << "fail open";
	}

	// segment
	int sequenceNum = 0;
	vector<segment> segV;

	// read
	while(isRead) {
		memset(&sTemp, 0, sizeof(sTemp));
		isRead.read(sTemp.data, sizeof(sTemp.data));
		sTemp.head.length = 1000;
		sTemp.head.seqNumber = sequenceNum;
		sequenceNum++;
		sTemp.head.ackNumber = -1;
		sTemp.head.fin = 0;
		sTemp.head.syn = 0;
		sTemp.head.ack = 0;
		segV.push_back(sTemp);
	}

	// setting
	int firstUnaskedNum = 0;
	int alreadySendNum = -1;
	float timeout = 1.5;
	int congestionWindow = 1;
	int threshold = 16;
	bool belowThreshold = true;
	segment rTemp;
	time_t startTime = time(NULL);
	time_t currentTime = time(NULL);
	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 100;
	setsockopt(senderSocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	while(true) {
		if(firstUnaskedNum >= segV.size()) {
			break;
		}
		int packetSendNum = 0;
		for(int i = 0; i < congestionWindow; i++) {
			if(firstUnaskedNum+i >= segV.size()) {
				break;
			}
			if(firstUnaskedNum+i > alreadySendNum) {
				printf("send  data  #%d,    winSize = %d\n", segV[firstUnaskedNum+i].head.seqNumber, congestionWindow);
				alreadySendNum = segV[firstUnaskedNum+i].head.seqNumber;
			} else {
				printf("resnd data  #%d     winSize = %d,\n", segV[firstUnaskedNum+i].head.seqNumber, congestionWindow);
			}
			sendto(senderSocket, &segV[firstUnaskedNum+i], sizeof(segV[firstUnaskedNum+i]), 0, (struct sockaddr *)&agent,agentSize);
			packetSendNum++;
		}
		startTime = time(NULL);
		int packetRecvNum = 0;
		while(true) {
			if(packetRecvNum == packetSendNum) {
				if(!belowThreshold) {
					congestionWindow++;
				}
				break;
			}
			currentTime = time(NULL);
			if(currentTime - startTime > timeout) {
				threshold = max(congestionWindow/2, 1);
				printf("time  out,         threshold = %d\n", threshold);
				congestionWindow = 1;
				break;
			}
			memset(&rTemp, 0, sizeof(rTemp));
			segmentSize = recvfrom(senderSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&tempAddr, &tempSize);
			if(congestionWindow < threshold) {
				belowThreshold = true;
			} else if (congestionWindow >= threshold) {
				belowThreshold = false;
			}
			if(segmentSize > 0) {
				if(rTemp.head.ackNumber == firstUnaskedNum) {
					printf("recv  ack   #%d\n", rTemp.head.ackNumber);
					firstUnaskedNum++;
					packetRecvNum++;
					if(belowThreshold) {
						congestionWindow++;
						belowThreshold = true;
					}
				}
			}
		}
	}
	memset(&rTemp, 0, sizeof(rTemp));
	rTemp.head.length = 1000;
	rTemp.head.seqNumber = -1;
	rTemp.head.ackNumber = -1;
	rTemp.head.fin = 1;
	rTemp.head.syn = 0;
	rTemp.head.ack = 0;
	printf("send  fin\n");
	sendto(senderSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&agent,agentSize);
	while(true) {
		memset(&rTemp, 0, sizeof(rTemp));
		segmentSize = recvfrom(senderSocket, &rTemp, sizeof(rTemp), 0, (struct sockaddr *)&tempAddr, &tempSize);
		if(segmentSize > 0) {
			if(rTemp.head.fin == 1) {
				printf("recv  finack\n");
				break;
			} else {
				continue;
			}
		}
	}
	return 0;






}
