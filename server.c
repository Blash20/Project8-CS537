#include "udp.h"
#include "message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "server_functions.h"

struct socket s;

struct call_table_entry {
    int valid; // 0 if entry is empty, 1 if entry is valid
    int in_progress; // 0 if call is not in progress, 1 if call is in progress
    int client_id;
    int seq_number;
    int result; // result of get or put. 0 if ack or idle
};

struct call_table_entry* call_table;

// search table for entry with matching client_id
// return index of entry if found, -1 if not found
int get_call_table_index(int client_id){
    for(int i = 0; i < 100; i++){
        if(call_table[i].valid == 1 && call_table[i].client_id == client_id){
            return i;
        }
    }
    return -1;
}

// modify entry in table
// return 0 if successful, -1 if not found
int modify_call_table_entry(int client_id, int in_progress, int seq_number, int result){
    int index = get_call_table_index(client_id);
    if(index == -1){
        return -1;
    }
    call_table[index].in_progress = in_progress;
    call_table[index].seq_number = seq_number;
    call_table[index].result = result;
    return 0;
}

// add entry to table
// return 0 if successful, -1 if table is full
int add_call_table_entry(int client_id, int seq_number, int result){

    for(int i = 0; i < 100; i++){
        if(call_table[i].valid == 0){
            call_table[i].valid = 1;
            call_table[i].in_progress = 0;
            call_table[i].client_id = client_id;
            call_table[i].seq_number = seq_number;
            call_table[i].result = result;
            return 0;
        }
    }
    return -1;
}

// remove entry from table
// return 0 if successful, -1 if not found
int remove_call_table_entry(int client_id){
    int index = get_call_table_index(client_id);
    if(index == -1){
        return -1;
    }
    call_table[index].valid = 0;
    return 0;
}

struct request_message deserialize_message(char* buf){
    struct request_message m;
    
    m.client_id = *(int *)buf;
    m.seq_number = *(int *)(buf + 4);
    m.type = *(int *)(buf + 8);

    m.buf[0] = *(int *)(buf);
    m.buf[1] = *(int *)(buf + 4);
    //m.buf[1] = buffer[1];

    return m;
}


// response to client request
// arg should point to a packet_info struct with a request_message
void* handle_function_call(void *arg){
    struct packet_info *p = (struct packet_info *)arg;
    struct request_message *m = (struct request_message *)p->buf;
    struct response_message response;
    response.client_id = m->client_id;
    response.seq_number = m->seq_number;
    response.type = 1; // 0 for ack, 1 for result

    if(m->type == 1){ // idle
        idle(m->buf[0]);
        response.result = 0;
    }
    else if(m->type == 2){ // get
        response.result = get(m->buf[0]);
    }
    else if(m->type == 3){ // put
        response.result = put(m->buf[0], m->buf[1]);
    }
    else if(m->type == 4){ // close
        response.result = 0;
        //TODO: HANDLE CLOSE
    } else{
        printf("invalid message type\n");
        exit(1);
    }

    // update call table
    modify_call_table_entry(m->client_id, 0, m->seq_number, response.result);

    //TODO: UNSURE IF SENDING PACKET CORRECTLY
    struct packet_info response_packet;
    response_packet.sock = p->sock;
    response_packet.slen = p->slen;
    response_packet.recv_len = sizeof(response);
    memcpy(response_packet.buf, &response, sizeof(response));

    printf("Result: %d\n", response.result);
    printf("Sqn Num: %d\n", response.seq_number);
    printf("Type: %d\n", response.type);
    printf("\n");


    send_packet(s, response_packet.sock, response_packet.slen, response_packet.buf, response_packet.recv_len);
    return NULL;
}


/*
The RPC server should take one command line argument: port
*/

/*
The server will track clients that have connected to the server. When a client
requests to have the server run a function it will execute the chosen function
if the server has a definition for the function. To prevent duplicate requests
and enforce the at-most once semantics, if it receives a message that is less
than the current tracked sequence number for that client it will simply
discard it. If it receives a message that is equal to the sequence number it
will reply with either the old value that was returned from the function, or a
message indicating that it is working on the current requests. If it receives a
message with a sequence number that is greater than the current tracked
sequence number this indicates a new request and it will start a thread to run
this task that will reply independently to the client.
*/
int main(int argc, char *argv[]){
    if (argc != 2) 
    {
        printf("expected usage: ./server port\n");
        exit(1);
    }
    
    int port = atoi(argv[1]);

    if(port < 2049 || port > 65536){
        printf("port must be between 2049 and 65536 inclusive\n");
        exit(1);
    }

    s = init_socket(port);

    call_table = (struct call_table_entry*)calloc(100, sizeof(struct call_table_entry));

    while(1) {
        struct packet_info request_packet = receive_packet(s);
        if(request_packet.recv_len == -1){
            continue;
        }

        struct request_message m = deserialize_message(request_packet.buf);

        int call_table_index = get_call_table_index(m.client_id);

        // when this is the cliet's first call, we need to skip the check for duplicat sequence number or low sequence number

        struct call_table_entry entry;
        if(call_table_index != -1){
            entry = call_table[call_table_index];
        } 

        printf("received message from client %d with seq number %d\n", m.client_id, m.seq_number);

        if(call_table_index != -1 && m.seq_number < entry.seq_number){
            // discard message
            printf("discarded message from client %d with seq number %d\n", m.client_id, m.seq_number);
            continue;
        }
        else if(call_table_index != -1 && m.seq_number == entry.seq_number) {
            // send ack or last result
            if (entry.in_progress == 1) {

                //send_packet will send a packet to a target destination. If you do not already have a target destination (possibly from receiving a packet) you will need to call populate_sockaddr to generate the destination to fill in the target and slen parameters.

                //TODO: THIS CODE IS TENTATIVE, NOT SURE IF I'M SENDING THE ACK CORRECTLY
                struct response_message response;
                response.seq_number = m.seq_number;
                response.client_id = m.client_id;
                response.type = 0; // 0 for ack
                response.result = -1;

                struct packet_info response_packet;
                response_packet.sock = request_packet.sock;
                response_packet.slen = request_packet.slen;
                response_packet.recv_len = sizeof(response);
                memcpy(response_packet.buf, &response, sizeof(response));

                
                send_packet(s, response_packet.sock, response_packet.slen, response_packet.buf, response_packet.recv_len);

                printf("sent ack to client %d with seq number %d\n", m.client_id, m.seq_number);
            } else {
                //TODO: THIS CODE IS TENTATIVE, NOT SURE IF I'M SENDING THE RESULT BACK CORRECTLY
                struct response_message response;
                response.seq_number = m.seq_number;
                response.client_id = m.client_id;
                response.type = 1; // 1 for last result
                response.result = entry.result;

                struct packet_info response_packet;
                response_packet.sock = request_packet.sock;
                response_packet.slen = request_packet.slen;
                response_packet.recv_len = sizeof(response);
                memcpy(response_packet.buf, &response, sizeof(response));

                send_packet(s, response_packet.sock, response_packet.slen, response_packet.buf, response_packet.recv_len);

                printf("sent last result to client %d with seq number %d\n", m.client_id, m.seq_number);
            }
        
        } else{
            call_table[call_table_index].seq_number = m.seq_number;
            call_table[call_table_index].in_progress = 1;

            //call thread to handle function call
            pthread_t thread;
            struct packet_info* p = (struct packet_info*)malloc(sizeof(struct packet_info));
            p->sock = request_packet.sock;
            p->slen = request_packet.slen;
            p->recv_len = request_packet.recv_len;
            memcpy(p->buf, request_packet.buf, request_packet.recv_len);
            printf("starting thread to handle function call\n");

            if(call_table_index == -1){
                add_call_table_entry(m.client_id, m.seq_number, 0);
            }
            pthread_create(&thread, NULL, handle_function_call, (void*)p);
        }

       
    }

}


