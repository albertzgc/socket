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

#include <sstream>
#include <fstream>
#include <vector>
#include <ios>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <ctype.h>
#include <map>
#include <math.h>

using namespace std;

#define AWS_UDP_PORT 33229
#define SERVER_C_UDP 32229
#define MAX_BUF_LEN 2048
#define MAX_DISTANCE 10000;
struct MapData{
	char map_id;
	double prop_speed;
	int tran_speed;
	map<int, vector<pair<int, double>>>	graph;
	int source_vertex;
    int dest_vertex;
	string map_string;
    double shortest_path_len;
    int file_size;
};
struct CalculationResults
{
    int         path[11];
    double      distance;
    double      tran_delay;
    double      prop_delay;
    double      total_delay;
};
int udp_sockfd;
struct sockaddr_in udp_server_addr, udp_client_addr;
MapData complete_user_req;
CalculationResults calc_results;

void parse_map_string(string map_string){
    //from https://stackoverflow.com/questions/53184552/dijkstras-algorithm-w-adjacency-list-map-c
    //parse the string containing the complete user request, also contains requested map info and construct the graph
    int first_end;
    int second_end;
    double distance;
    streamsize ss = cout.precision();
    int stop_index = map_string.find_first_of("-");
    complete_user_req.source_vertex = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    complete_user_req.dest_vertex = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    complete_user_req.file_size = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    complete_user_req.map_id = map_string.substr(0, stop_index).c_str()[0];
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    complete_user_req.prop_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    complete_user_req.tran_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of(" ");
    //print the received data message
    cout << "The Server C has received data for calculation: " << endl;
    cout << "* Propagation Speed: " << setprecision(2) << fixed << complete_user_req.prop_speed << " km/s" << endl;
    cout.precision(ss);
    cout << "* Transmission speed: " << complete_user_req.tran_speed << " KB/s" << endl;
    cout << "* map ID: " << complete_user_req.map_id << endl;
    cout << "* Source ID: " << complete_user_req.source_vertex << "    Destination ID:" << complete_user_req.dest_vertex << endl;
    //work through the original rows with the vertex data
    while(stop_index != string::npos){
        first_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
        second_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(",");
        distance = atof(map_string.substr(0, stop_index).c_str());

        //need to do 2 pairs for both ends for undirected graph
        pair<int, double> pair1(second_end, distance);
        vector<pair<int, double> > edge1 = complete_user_req.graph[first_end];
        edge1.push_back(pair1);
        complete_user_req.graph[first_end] = edge1;

        pair<int,double> pair2(first_end, distance);
        vector<pair<int, double> > edge2 = complete_user_req.graph[second_end];
        edge2.push_back(pair2);
        complete_user_req.graph[second_end] = edge2;
    
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
    }
}
// Function to print shortest 
// path from source to dest j 
// using parent array 
//from https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
void printPath(int parent[], int dest, int path[], int index) 
{ 
    // Base Case : If dest is source 
    if (parent[dest] == 101) {
		path[0] = index-1;
        return; 
	}
    printPath(parent, parent[dest], path, index+1); 
	path[index] = dest;
} 
//from https://stackoverflow.com/questions/53184552/dijkstras-algorithm-w-adjacency-list-map-c
//from https://algorithmsinfinite.blogspot.com/2018/10/dijkstras-algorithm-shortest-path_10.html
//from https://en.wikipedia.org/wiki/Dijkstra's_algorithm
void dijkstra(int source, int dest, int path[]) {
	//array stores shortest vertex path with each index assigned to vertex id
	int parent[100];
	for(int l = 0; l < 100; l++){
		parent[l] = 0;
	}
	parent[0] = 101;

	//add all vertices from map file to dijkstra map and iniatlize their distances from each other to effectively infinite
    map<int, double> dijkstra_map;
	map<int, int> visited;//visited map implements the relax step of dijkstra
    map<int, vector<pair<int, double>>>::iterator iter_map_val = complete_user_req.graph.begin();
	while(iter_map_val != complete_user_req.graph.end()) {
        pair<int, double> vertex_info;
        vertex_info.first = iter_map_val->first;
		if (vertex_info.first != source) {
			dijkstra_map[vertex_info.first] = MAX_DISTANCE;
		}
		visited[vertex_info.first] = 0;
		iter_map_val++;
	}
    //main dijkstra algorithm loop, each iteration is the relaxing step using the visited map to visited the vertex as used
	map<int, int>::iterator iter_relax = visited.begin();
	while(iter_relax != visited.end()) {
		int current_vertex = -1;
		double min_distance = MAX_DISTANCE;
        //loop to get the next vertex, with the shortest distance and hasn't been used to check distances from yet
		map<int, int>::iterator iter_next_close = visited.begin();
		while (iter_next_close != visited.end()) {
			int vertex = iter_next_close -> first;
			int marked = iter_next_close -> second;
            //if statement to determine next closest vertex that hasn't been used yet
			if (marked == 0 && dijkstra_map[vertex] < min_distance) {
				current_vertex = vertex;
				min_distance = dijkstra_map[vertex];
			}
			iter_next_close++;
		}
        //loop to update the new minimum path to the between 2 vertices in this iteration
		vector<pair<int, double>> adjacent_vertices = complete_user_req.graph[current_vertex];
		for (int i = 0; i < adjacent_vertices.size(); i++) {
			int adjacent_vertex = adjacent_vertices[i].first;
			double distance = adjacent_vertices[i].second;
			if (dijkstra_map[adjacent_vertex] > dijkstra_map[current_vertex] + distance) {
				parent[adjacent_vertex] = current_vertex;
				dijkstra_map[adjacent_vertex] = dijkstra_map[current_vertex] + distance;
			}
		}
        //remove current vertex from checking distance from again
		visited[current_vertex] = 1;
		iter_relax++;
	}
    //1 is used because path[0] == path size, so filling in path starts at [1]
    //this will also fill the path array with the path hops from source to destination
	printPath(parent,dest, path, 1);
    cout << endl;
    //after finding shortest paths for all nodes from source, iterate to destination node to store in our struct
    map<int, double>::iterator dist_iter = dijkstra_map.begin();
    int cur_des = dist_iter->first;
    double cur_len = dist_iter->second;
	while (dist_iter != dijkstra_map.end()) {
		if (cur_des == dest) {
			dist_iter++;
			break;
		}
		dist_iter++;
        cur_des = dist_iter->first;
        cur_len = dist_iter->second;
	}
    complete_user_req.shortest_path_len = cur_len;
}
int main(int argc, const char* argv[]) {
	//from Beej Guide
	//set up udp socket
	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		cout << "Server C failed to create UDP socket" << endl;
        exit(1);
    }
    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port   = htons(SERVER_C_UDP);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(bind(udp_sockfd, (struct sockaddr*)&udp_server_addr, sizeof udp_server_addr) == -1) {
		cout << "Server C failed to bind to UDP socket" << endl;
		exit(1);
	}
    cout << "The Server C is up and running using UDP on port " << SERVER_C_UDP << endl << endl;

	while(1) {
        //from Beej Guide
        //prepare udp socket for receiving from aws
        socklen_t udp_client_addr_len = sizeof udp_client_addr;
        char complete_user_req_buf[MAX_BUF_LEN];
        memset(complete_user_req_buf, 0, MAX_BUF_LEN);
        int numbytes;
		if ((numbytes = recvfrom (udp_sockfd, complete_user_req_buf, MAX_BUF_LEN,0, (struct sockaddr *) &udp_client_addr, &udp_client_addr_len)) == -1){
            cout << "Server C error receiving complete user arg string from AWS" << endl;
            exit(1);
		}
        //reconstruct the complete user request string from the received char buffer
        complete_user_req_buf[numbytes] = '\0';
        complete_user_req.map_string = string(complete_user_req_buf);
        parse_map_string(complete_user_req.map_string);
        //11 indexes for path since max hops is 10, and index 0 holds number of hops
		int path[11];
        int source_vertex = complete_user_req.source_vertex;
		dijkstra(complete_user_req.source_vertex, complete_user_req.dest_vertex, path);
        cout << "The Server C has finished the calculation: " << endl;
        cout << "Shortest path: ";
		for(int l = path[0]; l > 0; l--){
            if(l == 1){
                calc_results.path[l] = path[l];
                cout << path[l];
            }
            else{
                calc_results.path[l] = path[l];
			    cout << path[l] << " -- "; 
            }
		}
        cout << endl;
        //calculate and store the information in our struct
        int round_precision = 100;
        streamsize ss = cout.precision();
        cout.precision(2);
        cout << "Shortest distance: " << complete_user_req.shortest_path_len << " km" << endl;
        double transmission_delay = complete_user_req.shortest_path_len / complete_user_req.tran_speed;
        double propagation_delay = complete_user_req.file_size / complete_user_req.prop_speed;
        cout << "Transmission delay: " << round(transmission_delay*round_precision)/round_precision << " s" << endl;
        cout << "Propagation delay: " << round(propagation_delay*round_precision)/round_precision << " s" << endl << endl;
        cout.precision(ss);
        calc_results.path[0] = path[0];
        calc_results.distance = complete_user_req.shortest_path_len;
        calc_results.tran_delay = transmission_delay;
        calc_results.prop_delay = propagation_delay;
        calc_results.total_delay = transmission_delay + propagation_delay;

        //from Beej  Guide
        //placing the calculated results into a char buffer to be unpackaged in aws into our own struct
        char calc_results_buf[MAX_BUF_LEN];
        memset(calc_results_buf, 0, MAX_BUF_LEN);
        memcpy(calc_results_buf, &calc_results, sizeof calc_results);
		if (sendto(udp_sockfd, calc_results_buf, sizeof calc_results_buf, 0, (const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
			cout << "Server C failed to send data to AWS" << endl;
			exit(1);
		}
        complete_user_req.graph.clear();
		cout << "The Server C has finished sending the output to AWS" << endl << endl;
    	}
	close(udp_sockfd);
	return 0;
}