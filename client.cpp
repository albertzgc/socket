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

#include <ios>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <math.h>

using namespace std;

#define AWS_TCP_CLIENT_PORT 34229
#define MAX_BUF_LEN 2048
struct InputArguments
{
	int			map_id;
	int			src_vertex_idx;
	int			dest_vertex_idx;
	int			file_size;
};

struct CalculationResults
{
    int         path[11];
    double      distance;
    double      tran_delay;
    double      prop_delay;
};
CalculationResults calc_results;

int main(int argc, const char* argv[]){

	int sockfd;
	struct sockaddr_in addr_len;
	struct addrinfo hints;
	InputArguments user_arg;
	
	//retrieve user input arguments
	if (argc != 5) {
		cout << "Wrong number of inputs" << endl;
		exit(1);
	}
	user_arg.map_id = argv[1][0];
	user_arg.src_vertex_idx = atoi(argv[2]);
	user_arg.dest_vertex_idx = atoi(argv[3]);
	user_arg.file_size = atoi(argv[4]);

	//from Beej Guide
	//create client tcp socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		cout << "Failed to create client TCP socket" << endl;
		exit(1);
	}
	memset(&addr_len, 0, sizeof addr_len);
	addr_len.sin_family = AF_INET;
	addr_len.sin_port = htons(AWS_TCP_CLIENT_PORT);
	addr_len.sin_addr.s_addr = inet_addr("127.0.0.1");//assign hard-coded loopback address
	cout << "The client is up and running" << endl;
	if (connect(sockfd, (struct sockaddr*)&addr_len, sizeof addr_len) == -1) {
		cout << " Could not make connection" << endl;
		close(sockfd);
		exit(1);
	}

	//package user arguments into char buffer to send to aws for requested data
	char user_req[MAX_BUF_LEN];
	memset(user_req, 0, MAX_BUF_LEN);
	memcpy(user_req, &user_arg, sizeof(user_arg));
	if (send(sockfd, user_req, sizeof(user_req), 0) == -1) {
		cout << "Failed to send message" << endl;
		close(sockfd);
		exit(1);
	}
	cout << "The client has sent query to AWS using TCP: start vertex "<< user_arg.src_vertex_idx <<"; destination vertex " << user_arg.dest_vertex_idx << ", map " << (char)user_arg.map_id << "; file size " << user_arg.file_size << endl;
	//receive the aws result data in char buffer form
	char aws_response[MAX_BUF_LEN];
	memset(aws_response, 0, MAX_BUF_LEN);
	if (recv(sockfd, aws_response, sizeof aws_response, 0) == -1) {
		cout << "Failed to receive packet" << endl;
		close(sockfd);
		exit(1);
	}
	char no_map_found[] = "No map";
	char no_source[] = "no source";
	char no_destination[] = "no destination";
	if(!(strcmp(aws_response, no_map_found))){
		cout << "No map id " << (char)user_arg.map_id << " found" << endl;
		close(sockfd);
	}
	else if(!(strcmp(aws_response, no_source))){
		cout << "No vertex id " << user_arg.src_vertex_idx << " found" << endl;
		close(sockfd);
	}
	else if(!(strcmp(aws_response, no_destination))){
		cout << "No vertex id " << user_arg.dest_vertex_idx << " found" << endl;
		close(sockfd);
	}
	else{
		//unpackage the aws result data from char buffer to my own struct
		memset(&calc_results, 0, sizeof calc_results);
		memcpy(&calc_results, aws_response, sizeof calc_results);

		cout << endl << "The client has recevied results from AWS:" << endl;
		cout << "------------------------------------------------------" << endl;
		cout << "Source  Destination    Min Length    Tt    Tp    Delay" << endl;
		cout << "------------------------------------------------------" << endl;
		cout << user_arg.src_vertex_idx << "      " << user_arg.dest_vertex_idx << "             ";
		int round_precision = 100;
		streamsize ss = cout.precision();
		cout << calc_results.distance << "       ";
		cout.precision(2);
		cout << fixed << round(calc_results.tran_delay*round_precision)/round_precision << "  ";
		cout << fixed << round(calc_results.prop_delay*round_precision)/round_precision << "   ";
		cout << fixed << round((calc_results.tran_delay+calc_results.prop_delay)*round_precision)/round_precision << endl;
		cout.precision(ss);
		cout << "------------------------------------------------------" << endl;
		cout << "Shortest path: ";
		for(int l = calc_results.path[0]; l > 0; l--){
			if(l == 1){
				cout << calc_results.path[l];
			}
			else{
				cout << calc_results.path[l] << " -- "; 
			}
		}
		cout << endl;
		close(sockfd);
	}
}