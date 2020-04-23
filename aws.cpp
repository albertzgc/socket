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
#include <map>
#include <vector>
#include <set>

using namespace std;

#define TCPPORT "33229" //AWS's static TCP port
#define UDPPORT "34229" //AWS's static UDP port


#define AWS_UDP_PORT 33229
#define AWS_TCP_CLIENT_PORT 34229
#define SERVERA_PORT 30229
#define SERVERB_PORT 31229
#define SERVERC_PORT 32229
#define MAX_BUF_LEN 2048

struct MapInfo{
	char map_id;
	double prop_speed;
	double tran_speed;
	set<int> vertice;
	int num_edges;
	map<int, vector<pair<int, double> > >	graph;
	int src_vertex;
    int dest_vertex;
	string map_string;
};
struct ComputeRequestInfo
{
	int			map_id;
	int			src_vertex_idx;
	int			dest_vertex_idx;
	int         file_size;
};

struct PathForwardInfo
{
	int			map_id;
	int			src_vertex_idx;
};

struct PathResponseInfo
{
	int			min_path_vertex[10];
	int 		min_path_dist[10];
	double 		prop_speed;
	double 		tran_speed;
};

struct CalculationRequestInfo
{
	int			min_path_vertex[10];
	int			min_path_dist[10];
	double		prop_speed;
	double		tran_speed;
	double	    file_size;
};

struct CalculationResponseInfo
{
	double		trans_time;
	int			vertex[10];
	int			min_path_dist[10];
	double  	prop_time[10];
	double  	end_to_end[10];
};

void create_and_bind_udp_socket();

void create_and_bind_tcp_client();

void send_to_serverA();

void receive_from_serverA();

void send_to_serverC();

void receive_from_serverC();

int udp_sockfd, tcp_sockfd_client;
struct sockaddr_in udp_server_addr, udp_client_addr, tcp_server_addr, tcp_cliser_addr;
ComputeRequestInfo input_msg;
PathForwardInfo forward_msg;
MapInfo received_buff;
CalculationRequestInfo cal_request;
CalculationResponseInfo received_delay;

int main(int argc, const char* argv[]){
	create_and_bind_udp_socket();
	create_and_bind_tcp_client();

	printf("The AWS is up and running.\n");

	// listen on the socket just created for tcp client
	listen(tcp_sockfd_client, 5);

	// accept a call
	while (true) {
		// accept an incoming tcp connection
		// reference: Beej Guide
    		socklen_t tcp_client_addr_len = sizeof tcp_cliser_addr;
    		int new_tcp_sockfd = accept(tcp_sockfd_client, (struct sockaddr *)&tcp_cliser_addr, &tcp_client_addr_len);

		// check whether accept successfully
		if(new_tcp_sockfd == -1 ){
			perror("Accept Error");
			exit(1);
		}

		// receive message from the client and put it into input_msg
		// reference: Beej Guide
		char recv_buf1[MAX_BUF_LEN];
		memset(recv_buf1, 0, MAX_BUF_LEN);
		int num_of_bytes_read = recv(new_tcp_sockfd, recv_buf1, sizeof recv_buf1, 0);
		if(num_of_bytes_read < 0) {
			perror("Reading Stream Message Error");
			exit(1);
		} else if (num_of_bytes_read == 0) {
			printf("Connection Has Ended\n");
		}

		memset(&input_msg, 0, sizeof input_msg);
		memcpy(&input_msg, recv_buf1, sizeof recv_buf1);

		socklen_t tcp_length = sizeof tcp_cliser_addr;
		getsockname(tcp_sockfd_client,(struct sockaddr*) &tcp_cliser_addr, &tcp_length);
		// print onscreen message
		printf("The AWS has received map ID <%c>, start vertex <%d> and file size <%d> bits from the client using TCP over port <%d>.\n", (char)input_msg.map_id, input_msg.src_vertex_idx, input_msg.file_size, ntohs(tcp_cliser_addr.sin_port));	

		// send message to serverA
		send_to_serverA();

		// receive from serverA
		receive_from_serverA();
        /*
		// construct the information which will be sent to serverC
		cal_request.prop_speed = received_buff.prop_speed;
		cal_request.tran_speed = received_buff.tran_speed;
		cal_request.file_size = input_msg.file_size;

		for (int idx = 0; idx < 10; idx++) {
			cal_request.min_path_vertex[idx] = received_buff.min_path_vertex[idx];
			cal_request.min_path_dist[idx] = received_buff.min_path_dist[idx];
		}
        */
		// send request to serverC
		send_to_serverC();

		// receive from serverC
		receive_from_serverC();

		char send_buf3[MAX_BUF_LEN];
		memset(send_buf3, 0, MAX_BUF_LEN);
		memcpy(send_buf3, &received_delay, sizeof received_delay);
		int sent = sendto(new_tcp_sockfd, send_buf3, sizeof send_buf3, 0, (struct sockaddr *) &tcp_cliser_addr, sizeof tcp_cliser_addr);
		// check whether send successfully
		if (sent == -1) {
			perror("Connection Error");
			close(tcp_sockfd_client);
			exit(1);
		}
		// print onscreen message
		printf("The AWS has sent calculated delay to client using TCP over port <%d>.\n", ntohs(tcp_cliser_addr.sin_port));
	}
	close(tcp_sockfd_client);
	return 0;
}

/* Create and bind UDP socket */
void create_and_bind_udp_socket() {
	// create a udp socket
	// reference: Beej Guide
	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("AWS UDP Socket Create Error");
		exit(1);
	}

	memset(&udp_server_addr, 0, sizeof udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port = htons(AWS_UDP_PORT);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// bind the created udp socket with udp_serv_addr
	if (bind(udp_sockfd, (struct sockaddr*) &udp_server_addr, sizeof udp_server_addr) == -1) {
		perror("AWS UDP Bind Error");
    		exit(1);
    	}
}

/* Create and bind tcp socket of client */
void create_and_bind_tcp_client() {
	// create a socket
	// reference: Beej Guide
	if ((tcp_sockfd_client = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("AWS TCP Client Socket Create Error");
		exit(1);
	}

	memset(&tcp_server_addr, 0, sizeof tcp_server_addr);
	tcp_server_addr.sin_family = AF_INET;
	tcp_server_addr.sin_port = htons(AWS_TCP_CLIENT_PORT);
	tcp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// bind the created socket with tcp server addr
	if(bind(tcp_sockfd_client, (struct sockaddr*) &tcp_server_addr, sizeof tcp_server_addr) == -1)
	{
       	perror("AWS TCP Client Bind Error");
        exit(1);
    }
}

/* Send message to serverA */
void send_to_serverA() {
	// build message which will be sent to server A

    string send_message = std::to_string(input_msg.map_id) + " " + std::to_string(input_msg.src_vertex_idx);
	// send message to serverA via udp
	memset(&udp_server_addr,0, sizeof udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port = htons(SERVERA_PORT);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	sendto(udp_sockfd, (void*) send_message.c_str(), sizeof send_message, 0, (struct sockaddr *) &udp_server_addr, sizeof udp_server_addr);
	
	// print onscreen message
	printf("The AWS has sent Map ID <%c> and starting vertex <%d> to server A using UDP over port <%d>.\n", (char)forward_msg.map_id, forward_msg.src_vertex_idx, AWS_UDP_PORT);
}

/* Receive results from serverA */
void receive_from_serverA() {
	// build the container that will receive the msg from A and receive from A
	socklen_t server_udp_len = sizeof udp_client_addr;
	char recv_buf2[MAX_BUF_LEN];
	memset(recv_buf2, 'z', MAX_BUF_LEN);
    int numbytes;
	if((numbytes = recvfrom(udp_sockfd, recv_buf2, sizeof recv_buf2, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len)) == -1){
			perror("ServerA Receive Error");
			exit(1);
	}
    char not_found[] = "Graph not Found";
    //Graph is not in server A
    if(!(strcmp(recv_buf2,not_found))){
        cout << "The AWS has received map information from server A: " << recv_buf2 << endl;
    }
    else{
        recv_buf2[numbytes] = '\0';
        received_buff.map_string = string(recv_buf2);
    }
}

/* Send request to serverC */
void send_to_serverC() {
	memset(&udp_server_addr,0, sizeof(udp_server_addr));
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port = htons(SERVERC_PORT);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    string send_message2 = std::to_string(input_msg.src_vertex_idx) + "-" + std::to_string(input_msg.dest_vertex_idx) + "-" + std::to_string(input_msg.file_size) + "-" + (received_buff.map_string);
	sendto(udp_sockfd, (void*) send_message2.c_str(), MAX_BUF_LEN, 0, (struct sockaddr *) &udp_server_addr, sizeof udp_server_addr);

	//print the onscreen message
	printf("The AWS has sent path length, propagation speed and transmission speed to server C using UDP over port <%d>.\n", 
		AWS_UDP_PORT);
    //todo remove
    //cout << "MapInfo: " << received_buff.map_string << endl;
}

/* Receive response from serverC */
void receive_from_serverC() {
	// build the container that will receive the msg from A and receive from A
	char recv_buf3[MAX_BUF_LEN];
	memset(recv_buf3, 'z', MAX_BUF_LEN);
	socklen_t server_udp_len = sizeof udp_client_addr;
	recvfrom(udp_sockfd, recv_buf3, MAX_BUF_LEN, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len);
	memset(&received_delay, 0, sizeof received_delay);
	memcpy(&received_delay, recv_buf3, sizeof received_delay);

	// print onscreen message
	printf("The AWS has received delays from serverC:\n");
	printf("---------------------------------------------------------------\n");
	printf("%-8s\t%-8s\t%-8s\t%-8s\n", "Destination", "Tt", "Tp", "Delay");
	printf("---------------------------------------------------------------\n");
	double trans_time = received_delay.trans_time;
	for (int idx = 0; idx < 10; idx++) {
		int cur_des = received_delay.vertex[idx];
		int cur_len = received_delay.min_path_dist[idx];
		double prop_time = received_delay.prop_time[idx];
		double end_to_end = received_delay.end_to_end[idx];
		if ((cur_len > 0) || (idx == 0 && cur_len == 0)) {
			printf("%-8d\t%-8.2f\t%-8.2f\t%-8.2f\n", cur_des, trans_time, prop_time, end_to_end);
		}
	}
	printf("---------------------------------------------------------------\n");
}