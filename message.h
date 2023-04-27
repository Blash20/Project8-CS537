/*
RCP does not define the format of messages. For this reason, we define a message struct here.
*/


struct request_message{
    int seq_number;
    int client_id;

    //TODO: Figure out whether we need to have a number for init and close
    int type; // 0 for init, 1 for idle, 2 for get, 3 for put, 4 for close

    // other data like arguments
    int buf[2];    
};