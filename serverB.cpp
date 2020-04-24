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

using namespace std;

#define SERVER_B_UDP 31229
#define MAP_FILE "map2.txt"
#define MAX_BUF_LEN 2048
struct MapData{
	char map_id;
	string map_string;
};
int udp_sockfd;
struct sockaddr_in udp_server_addr, udp_client_addr;
MapData maps[52];

void map_construction() {
	ifstream mapFile(MAP_FILE);
	string line;
	int index = -1;//index in MapData struct array
	int i = 0; //line number in map file
	while (getline(mapFile, line)) {
		//each time an alphabet letter is reached in the file means it is a new graph so reset and start over
		//store the alphabet letter since that is the map id of a new map and start building the map string
		//these lines contain non-vertex info so they are delimited by hyphen
		if (isalpha(line.c_str()[0])) {
			index++;
			i = 0;
			maps[index].map_id = line.c_str()[0];
			maps[index].map_string = maps[index].map_string + line + "-";
		}
		//these lines contain non-vertex info so they are delimited by hyphen
		if (i > 0 && i < 3) {
			maps[index].map_string = maps[index].map_string + line + "-";
		}
		//the rest of the lines are vertex info, each edge is delimited by a comma
		else if (i > 2) { 
			maps[index].map_string = maps[index].map_string + line + ",";
		}
		i++;
	}
}
int main(int argc, const char* argv[]) {
	map_construction();

	//from Beej Guide
	//prepare udp socket for receiving from AWS
	if ((udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		cout << "Server B failed to create UDP socket" << endl;
		exit(1);
	}
	memset(&udp_server_addr, 0, sizeof udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port = htons(SERVER_B_UDP);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	socklen_t udp_server_addr_len = sizeof udp_server_addr;
	if(bind(udp_sockfd, (struct sockaddr*) &udp_server_addr, udp_server_addr_len) == -1){
		cout << "Server A failed to bind to UDP socket" << endl;
		exit(1);
	}
	cout << "The Server B is up and running using UDP on port " << SERVER_B_UDP << endl;

	//while loop to listen via udp
	while(1) {
        //from Beej Guide
        //prepare udp socket for receiving from aws
		socklen_t udp_client_addr_len = sizeof udp_client_addr;
		char aws_req_buf[MAX_BUF_LEN];
		int numbytes;
		memset(aws_req_buf, 0, MAX_BUF_LEN);
		if ((numbytes = recvfrom(udp_sockfd,aws_req_buf, sizeof aws_req_buf,0,
			(struct sockaddr *) &udp_client_addr, &udp_client_addr_len)) == -1) {
			cout << "Server B failed to receive data from AWS" << endl;
			exit(1);
		}
		aws_req_buf[numbytes] = '\0';
		vector<string> aws_req_vec;
		string aws_req_str = string(aws_req_buf);
		int i = aws_req_str.find_first_of(" ");
		string parsed_string = aws_req_str.substr(0, i);
		aws_req_vec.push_back(parsed_string);
		char id = atoi(aws_req_vec[0].c_str());

		cout << "The Server B has received input for finding graph of map " << id << endl;

		//check to see if the request map id can be found in the map file in server A
		int index = 0;
		while (maps[index].map_id != id) {
			//server A reached max possibilities, not in this server. sends error to AWS
			if(index == 52){
				cout << "The Server B does not have the required graph id " << id << endl;
				string graph_not_found_message = "Graph not Found";
				//from Beej Guide
				//udp socket for sending error to aws
				if (sendto(udp_sockfd, graph_not_found_message.c_str(), sizeof graph_not_found_message, 0, 
					(const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
					cout << "Server B failed to send error message to AWS" << endl;
					exit(1);
				}
				cout << "The Server B has sent \"" << graph_not_found_message << "\" to AWS." << endl << endl;
				break;
			}
			index++;
		}
		//map id was found, send the string with the map info over to aws
		if(index != 52){
			//udp socket for sending map info string to aws as a char buf
			char map_info[MAX_BUF_LEN];
			memset(map_info, 0, MAX_BUF_LEN);
			memcpy(map_info, &maps[index].map_string, sizeof maps[index]);
			string send_message = (maps[index].map_string);
			if (sendto(udp_sockfd, (void*) send_message.c_str(), sizeof map_info, 0, (const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
					cout << "Server A failed to send map info string to AWS" << endl;
					exit(1);
			}
			cout << "The Server B has sent Graph to AWS."  << endl << endl;
		}
	}
	close(udp_sockfd);
	return 0;
}