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
#include <set>
#include <math.h>

using namespace std;

#define PORTC "32229" //server C's static port

#define AWS_UDP_PORT 33229
#define SERVERC_PORT 32229
#define MAX_BUF_LEN 2048
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
    double shortest_path_len;
    int file_size;
};
struct CalculationResults
{
    int         path[11];
    double      distance;
    double      tran_delay;
    double      prop_delay;
};

void create_and_bind_udp_socket();

void path_finding(int, int, map<int, double>&, int*);

void show_path_finding_msg(map<int, int>&, int);

int udp_sockfd;
struct sockaddr_in udp_server_addr, udp_client_addr;

MapInfo received_buff;
CalculationResults output_msg;
const int INF = (int)1000000000;
void parse_map_string(string map_string){
    int first_end;
    int second_end;
    double dist;
    streamsize ss = cout.precision();
    int stop_index = map_string.find_first_of("-");
    received_buff.src_vertex = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.dest_vertex = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.file_size = atoi(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.map_id = map_string.substr(0, stop_index).c_str()[0];
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.prop_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of("-");
    received_buff.tran_speed = atof(map_string.substr(0, stop_index).c_str());
    map_string = map_string.substr(stop_index+1);
    stop_index = map_string.find_first_of(" ");

    cout << "The Server C has received data for calculation: " << endl;
    cout << "* Propagation Speed: " << setprecision(2) << fixed << received_buff.prop_speed << " km/s" << endl;
    cout.precision(ss);
    cout << "* Transmission speed: " << received_buff.tran_speed << " KB/s" << endl;
    cout << "* map ID: " << received_buff.map_id << endl;
    cout << "* Source ID: " << received_buff.src_vertex << "    Destination ID:" << received_buff.dest_vertex << endl;

    while(stop_index != string::npos){
        first_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
        second_end = atoi(map_string.substr(0, stop_index).c_str());
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(",");
        dist = atof(map_string.substr(0, stop_index).c_str());
        pair<int, double> pair1(second_end, dist);

        if (received_buff.graph.count(first_end) > 0) {
            vector<pair<int, double> > cur_vec = received_buff.graph[first_end];
            cur_vec.push_back(pair1);
            received_buff.graph[first_end] = cur_vec;
        }
        else {
            vector<pair<int, double> > cur_vec;
            cur_vec.push_back(pair1);
            received_buff.graph[first_end] = cur_vec;
        }
        pair<int,double> pair2(first_end, dist);

        if (received_buff.graph.count(second_end) > 0) {
            vector<pair<int, double> > cur_vec = received_buff.graph[second_end];
            cur_vec.push_back(pair2);
            received_buff.graph[second_end] = cur_vec;
        }
        else {
            vector<pair<int, double> > cur_vec;
            cur_vec.push_back(pair2);
            received_buff.graph[second_end] = cur_vec;
        }
        received_buff.num_edges++;
        received_buff.vertice.insert(first_end);
        received_buff.vertice.insert(second_end);
        
        map_string = map_string.substr(stop_index + 1);
        stop_index = map_string.find_first_of(" ");
    }
}
void clear_map(){
    received_buff.graph.clear();
    received_buff.vertice.clear();
}
int main(int argc, const char* argv[]) {
	// create a udp socket
	create_and_bind_udp_socket();

	while(true) {
		// receive related message from aws and put it into received_buff
        // reference: Beej Guide
        socklen_t udp_client_addr_len = sizeof udp_client_addr;
        char recv_buff[MAX_BUF_LEN];
        memset(recv_buff, 0, MAX_BUF_LEN);
        int numbytes;
		if ((numbytes = recvfrom (udp_sockfd, recv_buff, MAX_BUF_LEN,0, (struct sockaddr *) &udp_client_addr, &udp_client_addr_len)) == -1){
            perror("Server C Receive Error");
            exit(1);
		}
        recv_buff[numbytes] = '\0';
        received_buff.map_string = string(recv_buff);
        parse_map_string(received_buff.map_string);

        map<int, double> result;
		int path[11];
        int src_vertex = received_buff.src_vertex;
        //todo change src_vertex
		path_finding(received_buff.src_vertex, received_buff.dest_vertex, result, path);
        cout << "The Server C has finished the calculation: " << endl;
        cout << "Shortest path: ";
		for(int l = path[0]; l > 0; l--){
            if(l == 1){
                output_msg.path[l] = path[l];
                cout << path[l];
            }
            else{
                output_msg.path[l] = path[l];
			    cout << path[l] << " -- "; 
            }
		}
        cout << endl;
        int round_precision = 100;
        streamsize ss = cout.precision();
        cout.precision(2);
        cout << "Shortest distance: " << received_buff.shortest_path_len << " km" << endl;
        double transmission_delay = received_buff.shortest_path_len / received_buff.tran_speed;
        double propagation_delay = received_buff.file_size / received_buff.prop_speed;
        cout << "Transmission delay: " << round(transmission_delay*round_precision)/round_precision << " s" << endl;
        cout << "Propagation delay: " << round(propagation_delay*round_precision)/round_precision << " s" << endl << endl;
        cout.precision(ss);
        output_msg.path[0] = path[0];
        output_msg.distance = received_buff.shortest_path_len;
        output_msg.tran_delay = transmission_delay;
        output_msg.prop_delay = propagation_delay;
		// print out dijkstra's result on screen
		//show_path_finding_msg(result, src_vertex);

        // send three delays(output message) to AWS server
        // reference: Beej Guide
        char send_buff[MAX_BUF_LEN];
        memset(send_buff, 0, MAX_BUF_LEN);
        memcpy(send_buff, &output_msg, sizeof output_msg);
		if (sendto(udp_sockfd, send_buff, sizeof send_buff, 0, (const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
			perror("Server C Response Error");
			exit(1);
		}
        clear_map();
		cout << "The Server C has finished sending the output to AWS" << endl << endl;
    	}
	close(udp_sockfd);
	return 0;
}




/* Create a udp socket and bind it with udp server address */
void create_and_bind_udp_socket() {
	// create a udp socket
	// reference: Beej Guide
	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ServerA UDP Socket Create Error");
        	exit(1);
    }
    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_port   = htons(SERVERC_PORT);
	udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind the socket with server address
    if(bind(udp_sockfd, (struct sockaddr*)&udp_server_addr, sizeof udp_server_addr) == -1) {
		perror("ServerA UDP Bind Error");
		exit(1);
	}

    // print the onscreen message
    cout << "The Server C is up and running using UDP on port " << SERVERC_PORT << endl << endl;
}


// Function to print shortest 
// path from source to dest j 
// using parent array 
// reference: Geeks for Geeks
void printPath(int parent[], int j, int path[], int k) 
{ 
    // Base Case : If j is source 
    if (parent[j] == 101) {
		path[0] = k-1;
        return; 
	}
    printPath(parent, parent[j], path, k+1); 
	path[k] = j;
} 
/* find the shortest path from given source vertex to any other vertices */
void path_finding(int src, int dest, map<int, double> &result, int path[]) {
	// store the vertices involved
	int parent[100];
	for(int l = 0; l < 100; l++){
		parent[l] = 0;
	}
	parent[0] = 101;
	set<int> vertices = received_buff.vertice;

	// cur_graph stores every vertex and its adjacent vertices along with the distance between them
	map<int, vector<pair<int, double>>> cur_graph = received_buff.graph;

	set<int>::iterator iter = vertices.begin();
	// isVisited map stores whether a vertex has beeb visited
	map<int, bool> isVisited;
	// init every node distance to INF
	while(iter != vertices.end()) {
		int cur_ver = *iter;
		if (cur_ver != src) {
			result[cur_ver] = INF;
		}
		isVisited[cur_ver] = false;
		iter++;
	}

	map<int, bool>::iterator iter_vis = isVisited.begin();
	while(iter_vis != isVisited.end()) {
		int u = -1;
		double MIN = INF;
		map<int, bool>::iterator iter_vis1 = isVisited.begin();
		while (iter_vis1 != isVisited.end()) {
			int cur_ver = iter_vis1 -> first;
			bool state = iter_vis1 -> second;
			if (state == false && result[cur_ver] < MIN) {
				u = cur_ver;
				MIN = result[cur_ver];
			}
			iter_vis1++;
		}
		if (u == -1){
			return;
		}
		isVisited[u] = true;

		vector<pair<int, double> > neighbors = cur_graph[u];

		for (uint i = 0; i < neighbors.size(); i++) {
			int v = neighbors[i].first;
			double dist = neighbors[i].second;
			// if the vertex hasn't been visted and we find a shorter path than the previous one
			if (isVisited[v] == false && result[u] + dist < result[v]) {
				parent[v] = u;
				result[v] = result[u] + dist;
			}
		}
		iter_vis++;
	}
    //1 is used because path[0] == path size, so filling in path starts at [1]
	printPath(parent,dest, path, 1);
    cout << endl;
    map<int, double>::iterator dist_iter = result.begin();
    int cur_des = dist_iter->first;
    double cur_len = dist_iter->second;
	while (dist_iter != result.end()) {
		if (cur_des == dest) {
			dist_iter++;
			break;
		}
		dist_iter++;
        cur_des = dist_iter->first;
        cur_len = dist_iter->second;
	}
    received_buff.shortest_path_len = cur_len;

}
