#include "udp.h"
#include "message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"
#include <sys/time.h>
#include "server_functions.h"


// initializes the RPC connection to the server
struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]) {
    struct rpc_connection rpc;
    struct sockaddr_storage addr;
    socklen_t addrlen;

    // Creates new socket to recieve data
    rpc.recv_socket = init_socket(src_port);

    // Populates destination address
    populate_sockaddr(AF_INET, dst_port, dst_addr, &addr, &addrlen);
    rpc.dst_addr = *((struct sockaddr *)(&addr));
    rpc.dst_len = addrlen;
    
    // Initializes sequence number
    rpc.seq_number = 0;

    // Creates random client ID
    struct timeval tv;
	if(gettimeofday(&tv, NULL) != 0){
		perror("Client_ID Time Error");
		exit(1);
	}
    // Sets microseconds as seed
    srand(tv.tv_usec);
    rpc.client_id = rand();
    printf("Client ID: %d\n", rpc.client_id);

    return rpc;
}


// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time) {
    // 1. Sending
    // 2. Receiving
    // 3. Blocking until timeout 
    // 4. Check for ACK
    // 5. Reattempt if no response
    // 6. Exit when retry over 5 times

    // Where to store time? In buf?


    struct request_message message;

    // Create IDLE message
    message.client_id = rpc->client_id;
    message.seq_number = rpc->seq_number;
    message.type = 1;    // 1 for IDLE
    message.buf[0] = time;

    struct packet_info result;
    int valid = -1;
    
    for (int i = 0; i < RETRY_COUNT; i++) {
        //TODO: replace message with packet
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&message, sizeof(message));
        //struct packet_info result;
        // TODO: Recieving socket or DST socket for recieve packet?
        result = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        // How to access response message?
        if (result.slen < 0) { printf("Timeout: Retrying\n"); }
        
        else if (message.type == 0) {
            printf("ACK: %d\n", message.type);
            //idle(1);
            sleep(1);
            i = 0;
        }
        else if (message.client_id != rpc->client_id) {
            printf("Client_IDs do not match\n");
            continue;
        }
        else if(message.seq_number < rpc->seq_number) {
            printf("Old Sequence Number\n");
            continue;
        }
        else {
            printf("Valid Result\n");
            valid = 0;
            break;
        }

    }

    if (!valid) {
        printf("No Response\n");
        exit(1);
    }
    else { printf("Valid\n"); }

}



// Gets the value of a key on the server store
// Blocks until get RPC is completed on server
int RPC_get(struct rpc_connection *rpc, int key) {

    return -1;
}

// Sets the value of a key on the server store
// Blocks until put RPC is completed on server
int RPC_put(struct rpc_connection *rpc, int key, int value) {
    return -1;
}

// Closes the RPC connection to the server
// Does any cleanup needed for RPC variables
void RPC_close(struct rpc_connection *rpc) {
    // memset((char *) &(rpc->recv_socket.si), 0, sizeof(rpc->recv_socket.si));
    close_socket(rpc->recv_socket);
    // TODO: Other cleanup
    return;
}