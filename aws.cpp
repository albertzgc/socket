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
	set<int> vertice;
	string map_string;
};
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
int udp_sockfd, tcp_sockfd;
struct sockaddr_in udp_addr, udp_client_addr, tcp_addr, tcp_client_addr;
InputArguments user_arg;
MapInfo map_data;
CalculationResults calc_results;

void parse_map_string(string map_string){
    //parse the received map info from server a or b to check whether the requested vertices can be found within
    set<int> vertices;
    int first_end;
    int second_end;
    double dist;
    int stop_index = map_string.find_first_of("-");
    map_data.map_id = map_string.substr(0, stop_index).c_str()[0];
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of(" ");
    //work through the original rows with the vertex data
    while(stop_index != string::npos){
        first_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
        second_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(",");

        vertices.insert(first_end);
        vertices.insert(second_end);
        
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
    }
    map_data.vertice = vertices;
}
void no_map_error_to_client(int new_tcp_sockfd, string no_vertice_found){
    if(sendto(new_tcp_sockfd, no_vertice_found.c_str(), sizeof no_vertice_found, 0, (struct sockaddr *) &tcp_client_addr, sizeof tcp_client_addr) == -1){
        cout << "Failed to send message" << endl;
        close(tcp_sockfd);
        exit(1);
    }
}
void no_map_error_to_client(int new_tcp_sockfd){
    string no_map_found = "No map";
    if(sendto(new_tcp_sockfd, no_map_found.c_str(), sizeof no_map_found, 0, (struct sockaddr *) &tcp_client_addr, sizeof tcp_client_addr) == -1) {
        cout << "Failed to send message" << endl;
        close(tcp_sockfd);
        exit(1);
    }
    cout << "Map id: " << (char)user_arg.map_id << ", not found in the map file in either server, sending error to client using TCP over port " << ntohs(tcp_client_addr.sin_port) << endl;
}

void send_to_server(string server) {
	//put the user map id arg into string to send to server A or B
    string send_message = std::to_string(user_arg.map_id) + " ";
    string server_string = "";//string for printing respective server names
	//prepare udp socket for sending
	memset(&udp_addr,0, sizeof udp_addr);
	udp_addr.sin_family = AF_INET;
    //statements to determine which server is being used
    if(server == SERVERA_STRING){
	    udp_addr.sin_port = htons(SERVERA_PORT);
        server_string = SERVERA_STRING;
    }
    else{
	    udp_addr.sin_port = htons(SERVERB_PORT);
        server_string = SERVERB_STRING;
    }
	udp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sendto(udp_sockfd, (void*) send_message.c_str(), sizeof send_message, 0, (struct sockaddr *) &udp_addr, sizeof udp_addr);
	
	cout << "The AWS has sent Map ID to " << server_string << " using UDP over port " << AWS_UDP_PORT << endl;
}
int receive_from_server(string server) {
	//prepare udp socket for receiving
	socklen_t server_udp_len = sizeof udp_client_addr;
	char map_info[MAX_BUF_LEN];
	memset(map_info, 0, MAX_BUF_LEN);
    //from Beej Guide
    int numbytes;
    string server_string = "";//string for printing respective server names
    //statements to determine which server is being used
    if(server == SERVERA_STRING){
        server_string = SERVERA_STRING;
    }
    else{
        server_string = SERVERB_STRING;
    }
	if((numbytes = recvfrom(udp_sockfd, map_info, sizeof map_info, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len)) == -1){
			cout << "Error receiving from server " << server_string << endl;
			exit(1);
	}
    //check whether map id could be found in the current server
    char not_found[] = "Graph not Found";
    if(!(strcmp(map_info,not_found))){
        cout << "The AWS has received map information from "<< server_string << ", " << (char)user_arg.map_id << " not found in " << server_string << endl << endl;
        return 0;
    }
    else{
        cout << "The AWS has received map information from " << server_string << endl << endl;
        //transferring string data to our own struct to parse and hold necessary information
        map_info[numbytes] = '\0';
        map_data.map_string = string(map_info);
        return 1;
    }
}
void send_to_serverC() {
    //prepare udp socket for sending
	memset(&udp_addr,0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(SERVERC_PORT);
	udp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //placing the source and dest vertex, requested file size, and map info into a string to be parsed and reconstructed again at server C
    string complete_user_req = std::to_string(user_arg.src_vertex_idx) + "-" + std::to_string(user_arg.dest_vertex_idx) + "-" + std::to_string(user_arg.file_size) + "-" + (map_data.map_string);
	sendto(udp_sockfd, (void*) complete_user_req.c_str(), MAX_BUF_LEN, 0, (struct sockaddr *) &udp_addr, sizeof udp_addr);

	//print the onscreen message
	cout << "The AWS has sent map, source ID, destination ID, propagation speed, and transmission speed to server C using UDP over port " << AWS_UDP_PORT << endl;
}

void receive_from_serverC() {
    //prepare udp socket for receving
	socklen_t server_udp_len = sizeof udp_client_addr;
	char calc_results_buf[MAX_BUF_LEN];
	memset(calc_results_buf, 0, MAX_BUF_LEN);
	recvfrom(udp_sockfd, calc_results_buf, MAX_BUF_LEN, 0, (struct sockaddr*)&udp_client_addr, &server_udp_len);
	memset(&calc_results, 0, sizeof calc_results);
    //unpackage the char buffer data with the calculated results into our own struct
	memcpy(&calc_results, calc_results_buf, sizeof calc_results);
    cout << endl << "The AWS has received results from server C:" << endl;
    cout << "The Server C has finished the calculation: " << endl;
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
    int round_precision = 100;
    streamsize ss = cout.precision();
    cout << "Shortest distance: " << calc_results.distance << " km" << endl;
    cout.precision(2);
    cout << "Transmission delay: " << round(calc_results.tran_delay*round_precision)/round_precision << " s" << endl;
    cout << "Propagation delay: " << round(calc_results.prop_delay*round_precision)/round_precision << " s" << endl << endl;
    cout.precision(ss);
}
int main(int argc, const char* argv[]){

	//from Beej Guide
    //set up udp socket
	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		cout << "AWS failed to create UDP socket" << endl;
		exit(1);
	}
	memset(&udp_addr, 0, sizeof udp_addr);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(AWS_UDP_PORT);
	udp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(udp_sockfd, (struct sockaddr*) &udp_addr, sizeof udp_addr) == -1) {
            cout << "AWS failed to bind to UDP socket" << endl;
    		exit(1);
    	}
	//set up tcp socket
	if ((tcp_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		cout << "AWS failed to create TCP socket" << endl;
		exit(1);
	}
	memset(&tcp_addr, 0, sizeof tcp_addr);
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(AWS_TCP_CLIENT_PORT);
	tcp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if(bind(tcp_sockfd, (struct sockaddr*) &tcp_addr, sizeof tcp_addr) == -1){
       	cout << "AWS failed to bind to TCP socket" << endl;
        exit(1);
    }
	listen(tcp_sockfd, 10);

	printf("The AWS is up and running.\n");

	while (1) {// reference: Beej Guide
        socklen_t tcp_client_addr_len = sizeof tcp_client_addr;
        int new_tcp_sockfd = accept(tcp_sockfd, (struct sockaddr *)&tcp_client_addr, &tcp_client_addr_len);
		if(new_tcp_sockfd == -1 ){
			perror("accept");
			exit(1);
		}

		//receive packaged user arguments from client
		char user_req[MAX_BUF_LEN];
		memset(user_req, 0, MAX_BUF_LEN);
		int num_of_bytes_read = recv(new_tcp_sockfd, user_req, sizeof user_req, 0);
		if(num_of_bytes_read < 0) {
			cout << "Failed to receive message" << endl;
			exit(1);
		}
        //unpackage the user arguments from the char buffer and into our own struct
		memset(&user_arg, 0, sizeof user_arg);
		memcpy(&user_arg, user_req, sizeof user_req);

        //prepare tcp socket to send calculated data when ready
		socklen_t tcp_length = sizeof tcp_client_addr;
		getsockname(tcp_sockfd,(struct sockaddr*) &tcp_client_addr, &tcp_length);

		cout << "The AWS has received map ID " << (char)user_arg.map_id << ", start vertex " << user_arg.src_vertex_idx << ", destination vertex " << user_arg.dest_vertex_idx << ", and file size " << user_arg.file_size << " from the client using TCP over port " << ntohs(tcp_client_addr.sin_port) << endl;

		//check server A's map to see if it contains the map id first
		send_to_server(SERVERA_STRING);
        //if it is not in server A, check server B
		if (!receive_from_server(SERVERA_STRING)){
            send_to_server(SERVERB_STRING);
            //if map id still can't be found in server B, send error message to client
            if(!receive_from_server(SERVERB_STRING)){
                no_map_error_to_client(new_tcp_sockfd);
            }
        }
        //parse the map data received as a string to get a set of vertices
        parse_map_string(map_data.map_string);
        //if source vertex cannot be found in the set, let client know it is an unknown source
        if(!map_data.vertice.count(user_arg.src_vertex_idx)){
            cout << user_arg.src_vertex_idx << " (source) vertex not found in the graph, sending error to client using TCP over port "<< ntohs(tcp_client_addr.sin_port) << endl;
            no_map_error_to_client(new_tcp_sockfd, "no source");
        }
        //if destination vertex cannot be found in the set, let client know it is an unknown destination
        else if(!map_data.vertice.count(user_arg.dest_vertex_idx)){
            cout << user_arg.dest_vertex_idx << " (destination) vertex not found in the graph, sending error to client using TCP over port "<< ntohs(tcp_client_addr.sin_port) << endl;
            no_map_error_to_client(new_tcp_sockfd, "no destination");
        }
        //if both vertices can be found in the set, forward request info to server 3 for calculation
        else{
            cout << "The source and destination vertex are in the graph" << endl;
            send_to_serverC();
            receive_from_serverC();
            //package the calculated results from server C into a char buffer to send back to client
            char aws_response[MAX_BUF_LEN];
            memset(aws_response, 0, MAX_BUF_LEN);
            memcpy(aws_response, &calc_results, sizeof calc_results);
            if(sendto(new_tcp_sockfd, aws_response, sizeof aws_response, 0, (struct sockaddr *) &tcp_client_addr, sizeof tcp_client_addr) == -1) {
                cout << "Failed to send data" << endl;
                close(tcp_sockfd);
                exit(1);
            }
            cout << "The AWS has sent calculated results to client using TCP over port " << ntohs(tcp_client_addr.sin_port) << endl << endl;
        }
        map_data.vertice.clear();
	}
	close(tcp_sockfd);
	return 0;
}