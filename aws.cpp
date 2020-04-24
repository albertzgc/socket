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
#include <map>
#include <vector>
#include <set>
#include <math.h>

using namespace std;

#define TCPPORT "33229" //AWS's static TCP port
#define UDPPORT "34229" //AWS's static UDP port


#define AWS_UDP_PORT 33229
#define AWS_TCP_CLIENT_PORT 34229
#define SERVERA_PORT 30229
#define SERVERB_PORT 31229
#define SERVERC_PORT 32229
#define MAX_BUF_LEN 2048
#define SERVERA_STRING "Server A"
#define SERVERB_STRING "Server B"
struct MapInfo{
	char map_id;
	double prop_speed;
	int tran_speed;
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
struct CalculationResults
{
    int         path[11];
    double      distance;
    double      tran_delay;
    double      prop_delay;
};

void create_and_bind_udp_socket();

void create_and_bind_tcp_client();

void send_to_server(string);

int receive_from_server(string);

void send_to_serverC();

void receive_from_serverC();

int udp_sockfd, tcp_sockfd_client;
struct sockaddr_in udp_server_addr, udp_client_addr, tcp_server_addr, tcp_cliser_addr;
ComputeRequestInfo input_msg;
PathForwardInfo forward_msg;
MapInfo received_buff;
CalculationResults received_delay;

void parse_map_string(string map_string){
    set<int> vertices;
    int first_end;
    int second_end;
    double dist;
    int stop_index = map_string.find_first_of("-");
    received_buff.map_id = map_string.substr(0, stop_index).c_str()[0];
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.prop_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.tran_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of(" ");

    while(stop_index != string::npos){
        first_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
        second_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(",");

        received_buff.num_edges++;
        vertices.insert(first_end);
        vertices.insert(second_end);
        
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
    }
    received_buff.vertice = vertices;
}
void no_map_error_to_client(int new_tcp_sockfd, string no_vertice_found){
    int sent = sendto(new_tcp_sockfd, no_vertice_found.c_str(), sizeof no_vertice_found, 0, (struct sockaddr *) &tcp_cliser_addr, sizeof tcp_cliser_addr);
    // check whether send successfully
    if (sent == -1) {
        perror("Connection Error");
        close(tcp_sockfd_client);
        exit(1);
    }
    // print onscreen message
    //cout << "Map id: " << (char)input_msg.map_id << ", not found in the map file in server A, sending error to client using TCP over port " << ntohs(tcp_cliser_addr.sin_port) << endl;
}
void no_map_error_to_client(int new_tcp_sockfd){
    string no_map_found = "No map";
    int sent = sendto(new_tcp_sockfd, no_map_found.c_str(), sizeof no_map_found, 0, (struct sockaddr *) &tcp_cliser_addr, sizeof tcp_cliser_addr);
    // check whether send successfully
    if (sent == -1) {
        perror("Connection Error");
        close(tcp_sockfd_client);
        exit(1);
    }
    // print onscreen message
    cout << "Map id: " << (char)input_msg.map_id << ", not found in the map file in either server, sending error to client using TCP over port " << ntohs(tcp_cliser_addr.sin_port) << endl;
}
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
		cout << "The AWS has received map ID " << (char)input_msg.map_id << ", start vertex " << input_msg.src_vertex_idx << ", destination vertex " << input_msg.dest_vertex_idx << ", and file size " << input_msg.file_size << " from the client using TCP over port " << ntohs(tcp_cliser_addr.sin_port) << endl;

		// send message to serverA
		send_to_server(SERVERA_STRING);

		if (!receive_from_server(SERVERA_STRING)){
            send_to_server(SERVERB_STRING);
            if(!receive_from_server(SERVERB_STRING)){
                no_map_error_to_client(new_tcp_sockfd);
            }
        }


        parse_map_string(received_buff.map_string);
        if(!received_buff.vertice.count(input_msg.src_vertex_idx)){
            cout << input_msg.src_vertex_idx << " (source) vertex not found in the graph, sending error to client using TCP over port "<< ntohs(tcp_cliser_addr.sin_port) << endl;
            no_map_error_to_client(new_tcp_sockfd, "no source");
        }
        else if(!received_buff.vertice.count(input_msg.dest_vertex_idx)){
            cout << input_msg.dest_vertex_idx << " (destination) vertex not found in the graph, sending error to client using TCP over port "<< ntohs(tcp_cliser_addr.sin_port) << endl;
            no_map_error_to_client(new_tcp_sockfd, "no destination");
        }
        else{
            cout << "The source and destination vertex are in the graph" << endl;
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
            cout << "The AWS has sent calculated results to client using TCP over port " << ntohs(tcp_cliser_addr.sin_port) << endl << endl;
        }
        received_buff.vertice.clear();
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
void send_to_server(string server) {
	// build message which will be sent to server A
    string send_message = std::to_string(input_msg.map_id) + " ";
    char server_char[] = "Server A";
    string server_string = "";
	// send message to serverA via udp
	memset(&udp_server_addr,0, sizeof udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
    if(server == SERVERA_STRING){
	    udp_server_addr.sin_port = htons(SERVERA_PORT);
        server_string = SERVERA_STRING;
    }
    else{
	    udp_server_addr.sin_port = htons(SERVERB_PORT);
        server_string = SERVERB_STRING;
    }
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	sendto(udp_sockfd, (void*) send_message.c_str(), sizeof send_message, 0, (struct sockaddr *) &udp_server_addr, sizeof udp_server_addr);
	
	// print onscreen message
	cout << "The AWS has sent Map ID to " << server_string << " using UDP over port " << AWS_UDP_PORT << endl;
}
/* Receive results from serverA */
int receive_from_server(string server) {
	// build the container that will receive the msg from A and receive from A
	socklen_t server_udp_len = sizeof udp_client_addr;
	char recv_buf2[MAX_BUF_LEN];
	memset(recv_buf2, 0, MAX_BUF_LEN);
    int numbytes;
    string server_string = "";
    if(server == SERVERA_STRING){
        server_string = SERVERA_STRING;
    }
    else{
        server_string = SERVERB_STRING;
    }
	if((numbytes = recvfrom(udp_sockfd, recv_buf2, sizeof recv_buf2, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len)) == -1){
			perror("Server Receive Error");
			exit(1);
	}
    char not_found[] = "Graph not Found";
    //Graph is not in server A
    if(!(strcmp(recv_buf2,not_found))){
        cout << "The AWS has received map information from "<< server_string << ", " << (char)input_msg.map_id << " not found in " << server_string << endl << endl;
        return 0;
    }
    else{
        cout << "The AWS has received map information from " << server_string << endl << endl;
        recv_buf2[numbytes] = '\0';
        received_buff.map_string = string(recv_buf2);
        return 1;
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
	cout << "The AWS has sent map, source ID, destination ID, propagation speed, and transmission speed to server C using UDP over port " << AWS_UDP_PORT << endl;
}

/* Receive response from serverC */
void receive_from_serverC() {
	// build the container that will receive the msg from A and receive from A
	char recv_buf3[MAX_BUF_LEN];
	memset(recv_buf3, 0, MAX_BUF_LEN);
	socklen_t server_udp_len = sizeof udp_client_addr;
	recvfrom(udp_sockfd, recv_buf3, MAX_BUF_LEN, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len);
	memset(&received_delay, 0, sizeof received_delay);
	memcpy(&received_delay, recv_buf3, sizeof received_delay);
    cout << endl << "The AWS has received results from server C:" << endl;
    cout << "The Server C has finished the calculation: " << endl;
        cout << "Shortest path: ";
		for(int l = received_delay.path[0]; l > 0; l--){
            if(l == 1){
                cout << received_delay.path[l];
            }
            else{
			    cout << received_delay.path[l] << " -- "; 
            }
		}
    cout << endl;
    int round_precision = 100;
    streamsize ss = cout.precision();
    cout << "Shortest distance: " << received_delay.distance << " km" << endl;
    cout.precision(2);
    cout << "Transmission delay: " << round(received_delay.tran_delay*round_precision)/round_precision << " s" << endl;
    cout << "Propagation delay: " << round(received_delay.prop_delay*round_precision)/round_precision << " s" << endl << endl;
    cout.precision(ss);
}