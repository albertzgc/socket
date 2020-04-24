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

#include <iostream>
#include <vector>
#include <string>
#include <math.h>

using namespace std;

#define PORTA "3490" // the port client will be connecting to 
//TAKEN FROM BEEJ-TUTORIAL START
#define SERVERPORT "4950"	// the port users will be connecting to
#define AWS_TCP_CLIENT_PORT 34229
struct ComputeRequestInfo
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

void show_results(CalculationResults);

CalculationResults received_info;

int main(int argc, const char* argv[]){

	int tcp_sockfd;
	struct sockaddr_in tcp_server_addr;
	struct addrinfo hints;
	ComputeRequestInfo cur_request;
	
	if (argc != 5) {
		fprintf(stderr, "Input Error\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	cur_request.map_id = argv[1][0];
	cur_request.src_vertex_idx = atoi(argv[2]);
	cur_request.dest_vertex_idx = atoi(argv[3]);
	cur_request.file_size = atoi(argv[4]);

	// create tcp client;
	// reference: Beej Guide
	if ((tcp_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Client TCP Socket Create Error");
		exit(1);
	}

	/*// reference: EE450 Fall 2019 Project
	socklen_t tcp_client_len = sizeof tcp_client_addr;
	// reference: Beej Guide
	if(bind(tcp_sockfd, (struct sockaddr*) &tcp_client_addr, tcp_client_len) == -1){
		perror("Client Bind Error");
		exit(1);
	}
	int getsock_check = getsockname(tcp_sockfd,(struct sockaddr*) &tcp_client_addr, &tcp_client_len);
	if (getsock_check == -1) {
		perror("Getsockname Error");
		exit(1);
	}*/
	
	memset(&tcp_server_addr, 0, sizeof tcp_server_addr);
	tcp_server_addr.sin_family = AF_INET;
	tcp_server_addr.sin_port = htons(AWS_TCP_CLIENT_PORT);
	tcp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	printf("The client is up and running.\n");

	// do TCP connection
	// reference: Beej Guide
	if (connect(tcp_sockfd, (struct sockaddr*)&tcp_server_addr, sizeof tcp_server_addr) == -1) {
		perror("Connection Error");
		close(tcp_sockfd);
		exit(1);
	}

	// send message to aws
	// cur_request is the buffer that read info into
	// string send_buf;
	char send_buf[1024];
	memset(send_buf, 0, 1024);
	memcpy(send_buf, &cur_request, sizeof(cur_request));
	if (send(tcp_sockfd, send_buf, sizeof(send_buf), 0) == -1) {
		perror("Message Send Error");
		close(tcp_sockfd);
		exit(1);
	}
	printf("The client has sent query to AWS using TCP: start vertex <%d>; map id <%c>; file size <%d> bits.\n", cur_request.src_vertex_idx, (char)cur_request.map_id, cur_request.file_size);

	char recv_buf[1024];
	memset(recv_buf, 0, 1024);
	if (recv(tcp_sockfd, recv_buf, sizeof recv_buf, 0) == -1) {
		perror("Receive Error");
		close(tcp_sockfd);
		exit(1);
	}
	char no_map_found[] = "No map";
	char no_source[] = "no source";
	char no_destination[] = "no destination";
	if(!(strcmp(recv_buf, no_map_found))){
		cout << "No map id " << (char)cur_request.map_id << " found" << endl;
		close(tcp_sockfd);
	}
	else if(!(strcmp(recv_buf, no_source))){
		cout << "No vertex id " << cur_request.src_vertex_idx << " found" << endl;
		close(tcp_sockfd);
	}
	else if(!(strcmp(recv_buf, no_destination))){
		cout << "No vertex id " << cur_request.dest_vertex_idx << " found" << endl;
		close(tcp_sockfd);
	}
	else{
		memset(&received_info, 0, sizeof received_info);
		memcpy(&received_info, recv_buf, sizeof received_info);

		cout << endl << "The client has recevied results from AWS:" << endl;
		cout << "------------------------------------------------------" << endl;
		cout << "Source  Destination    Min Length    Tt    Tp    Delay" << endl;
		cout << "------------------------------------------------------" << endl;
		cout << cur_request.src_vertex_idx << "      " << cur_request.dest_vertex_idx << "             ";
		int round_precision = 100;
		streamsize ss = cout.precision();
		cout << received_info.distance << "       ";
		cout.precision(2);
		cout << round(received_info.tran_delay*round_precision)/round_precision << "  ";
		cout << round(received_info.prop_delay*round_precision)/round_precision << "   ";
		cout << round((received_info.tran_delay+received_info.prop_delay)*round_precision)/round_precision << endl;
		cout.precision(ss);
		cout << "------------------------------------------------------" << endl;
		cout << "Shortest path: ";
		for(int l = received_info.path[0]; l > 0; l--){
			if(l == 1){
				cout << received_info.path[l];
			}
			else{
				cout << received_info.path[l] << " -- "; 
			}
		}
		cout << endl;
		close(tcp_sockfd);
	}
}