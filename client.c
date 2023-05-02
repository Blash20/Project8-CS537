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

// int sqn = 0;

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
    printf("IDLE:\n");


    struct request_message message;

    rpc->seq_number++;

    // Create IDLE message
    message.client_id = rpc->client_id;
    message.seq_number = rpc->seq_number;
    message.type = 1;    // 1 for IDLE
    message.buf[0] = time;

    struct packet_info result;
    struct response_message rm;
    int valid = -1;

    for (int i = 0; i < RETRY_COUNT; i++) {
        //TODO: replace message with packet
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&message, sizeof(message));
        //struct packet_info result;
        // TODO: Recieving socket or DST socket for recieve packet?
        result = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        
        rm = *(struct response_message *) result.buf;
        
        // printf("RM Client ID: %d\n", rm.client_id);


        if (result.slen < 0) { printf("Timeout: Retrying\n"); }
        
        else if (rm.type == 0) {
            printf("ACK: %d\n", rm.type);
            //idle(1);
            sleep(1);
            i = 0;
        }
        else if (rm.client_id != rpc->client_id) {
            printf("Client_IDs do not match\n");
            continue;
        }
        else if(rm.seq_number < rpc->seq_number) {
            printf("Old Sequence Number\n");
            continue;
        }
        else {
            printf("Valid Result\n");
            valid = 0;
            break;
        }

    }

    if (valid != 0) {
        printf("No Response\n");
        exit(1);
    }
    else { printf("Valid\n"); }

}



// Gets the value of a key on the server store
// Blocks until get RPC is completed on server
int RPC_get(struct rpc_connection *rpc, int key) {
    printf("GET:\n");
    struct request_message message;

    rpc->seq_number++;

    // Create IDLE message
    message.client_id = rpc->client_id;
    message.seq_number = rpc->seq_number;
    message.type = 2;    // 2 for GET
    message.buf[0] = key;

    struct packet_info result;
    struct response_message rm;
    int valid = -1;
    
    for (int i = 0; i < RETRY_COUNT; i++) {
        //TODO: replace message with packet
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&message, sizeof(message));
        //struct packet_info result;
        // TODO: Recieving socket or DST socket for recieve packet?
        result = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        
        rm = *(struct response_message *) result.buf;
        // printf("RM Client ID: %d\n", rm.client_id);


        if (result.slen < 0) { printf("Timeout: Retrying\n"); }
        
        else if (rm.type == 0) {
            printf("ACK: %d\n", rm.type);
            //idle(1);
            sleep(1);
            i = 0;
        }
        else if (rm.client_id != rpc->client_id) {
            printf("Client_IDs do not match\n");
            continue;
        }
        else if(rm.seq_number < rpc->seq_number) {
            printf("Old Sequence Number\n");
            continue;
        }
        else {
            printf("Valid Result\n");
            valid = 0;
            break;
        }

    }

    if (valid != 0) {
        printf("No Response\n");
        exit(1);
    }
    else { 
        printf("Result: %d\n", rm.result);
        return rm.result; 
    }


    return -1;
    
}

// Sets the value of a key on the server store
// Blocks until put RPC is completed on server
int RPC_put(struct rpc_connection *rpc, int key, int value) {
    printf("PUT:\n");
    struct request_message message;

    rpc->seq_number++;

    // Create IDLE message
    message.client_id = rpc->client_id;
    message.seq_number = rpc->seq_number;
    message.type = 3;    // 3 for PUT
    message.buf[0] = key;
    message.buf[1] = value;

    struct packet_info result;
    struct response_message rm;
    int valid = -1;
    
    for (int i = 0; i < RETRY_COUNT; i++) {
        //TODO: replace message with packet
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&message, sizeof(message));
        //struct packet_info result;
        // TODO: Recieving socket or DST socket for recieve packet?
        result = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        
        rm = *(struct response_message *) result.buf;
        // printf("RM Client ID: %d\n", rm.client_id);


        if (result.slen < 0) { printf("Timeout: Retrying\n"); }
        
        else if (rm.type == 0) {
            printf("ACK: %d\n", rm.type);
            sleep(1);
            i = 0;
        }
        else if (rm.client_id != rpc->client_id) {
            printf("Client_IDs do not match\n");
            continue;
        }
        else if(rm.seq_number < rpc->seq_number) {
            printf("Old Sequence Number\n");
            continue;
        }
        else {
            printf("Valid Result\n");
            valid = 0;
            break;
        }

    }

    if (valid != 0) {
        printf("No Response\n");
        exit(1);
    }
    else { 
        printf("Result: %d\n", rm.result);
        return rm.result; 
    }


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