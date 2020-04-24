#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <ctype.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


//TAKEN FROM BEEJ-TUTORIAL START
using namespace std;

#define SERVERA_UDP_PORT 30229
#define MAP_FILE "map1.txt"
// format to store the vertex infomation from map.txt
#define MAX_BUF_LEN 2048
struct MapInfo{
	char map_id;
	string map_string;
};

void map_construction();

void create_and_bind_udp_socket();

int udp_sockfd;
struct sockaddr_in udp_server_addr, udp_client_addr;
MapInfo maps[52];

int main(int argc, const char* argv[]) {
	// print boot up msg
	cout << "The Server A is up and running using UDP on port " << SERVERA_UDP_PORT << endl;

	map_construction();

	create_and_bind_udp_socket();

	// catch any possible message
	while(true) {
		// receive map id and starting vertex from aws
		socklen_t udp_client_addr_len = sizeof udp_client_addr;
		// reference: Beej Guide
		char recv_buf[MAX_BUF_LEN];
		int numbytes;
		memset(recv_buf, 0, MAX_BUF_LEN);
		if ((numbytes = recvfrom(udp_sockfd,recv_buf, sizeof recv_buf,0,
			(struct sockaddr *) &udp_client_addr, &udp_client_addr_len)) == -1) {
			perror("ServerA Receive Error");
			exit(1);
		}
		recv_buf[numbytes] = '\0';
		vector<string> recv_payload;
		string recv_input = string(recv_buf);
		int i = recv_input.find_first_of(" ");
		string parsed_string = recv_input.substr(0, i);
		recv_payload.push_back(parsed_string);
		char id = atoi(recv_payload[0].c_str());

		cout << "The Server A has received input for finding graph of map " << id << endl;

		// locate the index of the map whose map_id is required id
		int index = 0;
		while (maps[index].map_id != id) {
			//todo check for error
			if(index == 52){
				cout << "The Server A does not have the required graph id " << id << endl;
				string graph_not_found_message = "Graph not Found";
				// send path finding result to the AWS
				// reference: Beej Guide
				if (sendto(udp_sockfd, graph_not_found_message.c_str(), sizeof graph_not_found_message, 0, 
					(const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
					perror("ServerA Response Error");
					exit(1);
				}
				cout << "The Server A has sent \"" << graph_not_found_message << "\" to AWS." << endl << endl;
				break;
			}
			index++;
		}
		if(index != 52){
			char send_buf[MAX_BUF_LEN];
			memset(send_buf, 0, MAX_BUF_LEN);
			memcpy(send_buf, &maps[index].map_string, sizeof maps[index]);
			string send_message = (maps[index].map_string);
			// send path finding result to the AWS
			// reference: Beej Guide
			if (sendto(udp_sockfd, (void*) send_message.c_str(), sizeof send_buf, 0, 
				(const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
					perror("ServerA Response Error");
					exit(1);
			}
			cout << "The Server A has sent Graph to AWS."  << endl << endl;
		}
	}
	close(udp_sockfd);
	return 0;
}

/* Read map.txt and save the information into result map, then create the onscreen message */
void map_construction() {
	ifstream mapFile(MAP_FILE);
	string line;
	int idx = -1;
	int i = 0; // store the # of line counting from map_id line
	while (getline(mapFile, line)) {
		// if the first item is alphabet, reset i to 0
		if (isalpha(line.c_str()[0])) {
			idx++;
			i = 0;
			maps[idx].map_string = "";
		}
		if (i == 0) { // if i = 0, current line has map id
			maps[idx].map_id = line.c_str()[0];
			maps[idx].map_string = maps[idx].map_string + line + "-";
		}
		else if (i == 1) { // if i = 1, current line is for propagation speed
			maps[idx].map_string = maps[idx].map_string + line + "-";
		}
		else if (i == 2) { // if i = 2, current line is for transmission speed
			maps[idx].map_string = maps[idx].map_string + line + "-";
		}
		else if (i > 2) { // if i > 2, then current line has vertex information 
			// split current line with space and save each line into fileValues
			maps[idx].map_string = maps[idx].map_string + line + ",";
		}
		i++;
	}
}

/* create a udp socket and bind the socket with address */
void create_and_bind_udp_socket() {
	// create udp client
	// reference: Beej Guide
	if ((udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ServerA UDP Socket Create Error");
		exit(1);
	}

	memset(&udp_server_addr, 0, sizeof udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port = htons(SERVERA_UDP_PORT);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// bind the socket with udp server address
	socklen_t udp_server_addr_len = sizeof udp_server_addr;
	if(bind(udp_sockfd, (struct sockaddr*) &udp_server_addr, udp_server_addr_len) == -1){
		perror("ServerA UDP Bind Error");
		exit(1);
	}
}