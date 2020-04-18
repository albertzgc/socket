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
#include <map>
#include <string>
#include <vector>
#include <set>


//TAKEN FROM BEEJ-TUTORIAL START
using namespace std;

#define INT_MAX 2147483647
#define SERVERA_UDP_PORT 21532
#define MAXDATASIZE 10000
#define PORTA "3490" //server A's static port
// format to store the vertex infomation from map.txt
struct MapInfo{
	char map_id;
	double prop_speed;
	double tran_speed;
	set<int> vertice;
	int num_edges;
	map<int, vector<pair<int, int> > >	graph;
};

struct PathForwardInfo{
	int		map_id;
	int		src_vertex_idx;
};

struct PathReponseInfo{
	int		min_path_vertex[10];
	int		min_path_dist[10];
	double	prop_speed;
	double	tran_speed;
};

void map_construction();

void create_and_bind_udp_socket();

void path_finding(int, int, map<int, int>&);

void show_path_finding_msg(map<int, int>&, int);

int udp_sockfd;
int num_of_maps;
const int INF = (int)1000000000;
struct sockaddr_in udp_server_addr, udp_client_addr;
MapInfo maps[52];
PathForwardInfo received_buff;
PathReponseInfo response;

int main(int argc, const char* argv[]) {
	// print boot up msg
	printf("The Server A is up and running using UDP on port <%d>.\n",SERVERA_UDP_PORT);

	map_construction();

	create_and_bind_udp_socket();

	// catch any possible message
	while(true) {
		// receive map id and starting vertex from aws
		socklen_t udp_client_addr_len = sizeof udp_client_addr;
		// reference: Beej Guide
		char recv_buf[1024];
		memset(recv_buf, 'z', 1024);
		if (recvfrom(udp_sockfd,recv_buf, sizeof recv_buf,0,
			(struct sockaddr *) &udp_client_addr, &udp_client_addr_len) == -1) {
			perror("ServerA Receive Error");
			exit(1);
		}

		memset(&received_buff, 0, sizeof received_buff);
		memcpy(&received_buff, recv_buf, sizeof recv_buf);

		printf("The Server A has received input for finding shortest paths: starting vertex <%d> of map <%c>.\n", received_buff.src_vertex_idx, (char)received_buff.map_id);
		
		char id = (char)received_buff.map_id;
		int src_vertex = received_buff.src_vertex_idx;

		// locate the index of the map whose map_id is required id
		int index = 0;
		while (maps[index].map_id != id) {
			index++;
		}

		// using Dijkstra's to find the shortest path and store into result
		map<int, int> result;
		path_finding(src_vertex, index, result);

		// print out dijkstra's result on screen
		show_path_finding_msg(result, src_vertex);

		// construct response message
		response.prop_speed = maps[index].prop_speed;
		response.tran_speed = maps[index].tran_speed;

		map<int, int>::iterator iter = result.begin();
		int idx = 0;
    	while(iter != result.end()) {
			int cur_ver = iter -> first;
			int cur_dist = iter -> second;
			response.min_path_vertex[idx] = cur_ver;
			response.min_path_dist[idx] = cur_dist;
			idx++;
			iter++;
    	}

		char send_buf[1024];
		memset(send_buf, 0, 1024);
		memcpy(send_buf, &response, sizeof response);
		// send path finding result to the AWS
		// reference: Beej Guide
		if (sendto(udp_sockfd, send_buf, sizeof send_buf, 0, 
		    (const struct sockaddr *) &udp_client_addr, (socklen_t)sizeof udp_client_addr) == -1) {
			perror("ServerA Response Error");
			exit(1);
		}
		printf("The Server A has sent the shortest paths to AWS.\n");
	}
	close(udp_sockfd);
	return 0;
}

/* Read map.txt and save the information into result map, then create the onscreen message */
void map_construction() {
	ifstream mapFile("map1.txt");
	string line;
	int idx = -1;
	int i = 0; // store the # of line counting from map_id line
	while (getline(mapFile, line)) {
		// if the first item is alphabet, reset i to 0
		if (isalpha(line.c_str()[0])) {
			num_of_maps++;
			idx++;
			i = 0;
		}
		if (i == 0) { // if i = 0, current line has map id
			maps[idx].map_id = line.c_str()[0];
		}
		else if (i == 1) { // if i = 1, current line is for propagation speed
			maps[idx].prop_speed = stold(line.c_str());
		}
		else if (i == 2) { // if i = 2, current line is for transmission speed
			maps[idx].tran_speed = stold(line.c_str());
			}
		else if (i > 2) { // if i > 2, then current line has vertex information 
			// split current line with space and save each line into fileValues
			vector<string> fileValues;
			fileValues.push_back(line);
			string ind = fileValues[0];
			int stop_index = ind.find_first_of(" ");
			int first_end = atoi(ind.substr(0, stop_index).c_str());
			ind = ind.substr(stop_index + 1);
			stop_index = ind.find_first_of(" ");
			int second_end = atoi(ind.substr(0, stop_index).c_str());
			ind = ind.substr(stop_index + 1);
			int dist = atoi(ind.c_str());
			// build adjacency map
			pair<int, int> pair1(second_end, dist);

			if (maps[idx].graph.count(first_end) > 0) {
				vector<pair<int, int> > cur_vec = maps[idx].graph[first_end];
				cur_vec.push_back(pair1);
				maps[idx].graph[first_end] = cur_vec;
			}
			else {
				vector<pair<int, int> > cur_vec;
				cur_vec.push_back(pair1);
				maps[idx].graph[first_end] = cur_vec;
			}
			pair<int, int> pair2(first_end, dist);

			if (maps[idx].graph.count(second_end) > 0) {
				vector<pair<int, int> > cur_vec = maps[idx].graph[second_end];
				cur_vec.push_back(pair2);
				maps[idx].graph[second_end] = cur_vec;
			}
			else {
				vector<pair<int, int> > cur_vec;
				cur_vec.push_back(pair2);
				maps[idx].graph[second_end] = cur_vec;
			}
			maps[idx].num_edges++;
			maps[idx].vertice.insert(first_end);
			maps[idx].vertice.insert(second_end);
		}
		i++;
	}
	// print onscreen message
	printf("The Server A has constructed a list of <%d> maps:\n", num_of_maps);
	printf("--------------------------------------------------\n");
	printf("%-8s\t%-16s\t%-16s\n", "Map ID", "Num Vertices", "Num Edges");
	printf("--------------------------------------------------\n");
	for (int idx = 0; idx < num_of_maps; idx++) {
		char map_id = maps[idx].map_id;
		int num_vertices = maps[idx].vertice.size();
		int num_edges = maps[idx].num_edges;
		printf("%-8c\t%-16d\t%-16d\t\n", map_id, num_vertices, num_edges);
	}
	printf("--------------------------------------------------\n");
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

/* find the shortest path from given source vertex to any other vertices */
void path_finding(int src, int index, map<int, int> &result) {
	// store the vertices involved
	set<int> vertices = maps[index].vertice;

	// cur_graph stores every vertex and its adjacent vertices along with the distance between them
	map<int, vector<pair<int, int> > > cur_graph = maps[index].graph;

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
		int MIN = INF;
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

		vector<pair<int, int> > neighbors = cur_graph[u];

		for (uint i = 0; i < neighbors.size(); i++) {
			int v = neighbors[i].first;
			int dist = neighbors[i].second;
			// if the vertex hasn't been visted and we find a shorter path than the previous one
			if (isVisited[v] == false && result[u] + dist < result[v]) {
				result[v] = result[u] + dist;
			}
		}
		iter_vis++;
	}
}

/* print the path finding results on the screen */
void show_path_finding_msg(map<int, int>&result, int src_vertex) {
	printf("The Server A has identified the shortest paths:\n");
	printf("------------------------------------------------\n");
	printf("%-16s\t%-16s\t\n", "Destination", "Min Length");
	printf("------------------------------------------------\n");

	// iterate through the result
	map<int, int>::iterator iter = result.begin();
	while (iter != result.end()) {
		int cur_des = iter->first;
		int cur_len = iter->second;
		if (cur_des == src_vertex) {
			iter++;
			continue;
		}
		printf("%-16d\t%-16d\t\t\n", cur_des, cur_len);
		iter++;
	}
	printf("------------------------------------------------\n");
}