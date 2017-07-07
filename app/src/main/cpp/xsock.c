#include "xsock.h"
void startServer(Room *room, RoomListener *listener){
    room->listener = listener;
    room->fd_broadcast_in = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_broadcast_in == -1){
        (*listener->onError)("socket error");
        return;
    }

    const int opt = -1;
    int ret = 0;
    ret = setsockopt(room->fd_broadcast_in, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(ret == -1){
        listener->onError("set socket error");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(55556);
    socklen_t len = sizeof(addr);

    ret = bind(room->fd_broadcast_in, (struct sockaddr*)&addr, len);
    if(ret == -1){
        listener->onError("bind socket error");
        return;
    }

    //setnonblocking(room->fd_broadcast_in);

    room->fd_broadcast_out = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_broadcast_out == -1){
        listener->onError("socket error");
        return;
    }

    ret = setsockopt(room->fd_broadcast_out, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(ret == -1){
        listener->onError("set socket error");
        return;
    }



    room->fd_in = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_in == -1){
        listener->onError("socket error");
        return;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6002);
    ret = bind(room->fd_in, (struct sockaddr*)&addr, len);
    if(ret == -1){
        listener->onError("bind socket error");
        return;
    }

    room->fd_out = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_out == -1){
        listener->onError("socket error");
        return;
    }

    int maxfdp = room->fd_in;
    if(maxfdp < room->fd_out){
        maxfdp = room->fd_out;
    }
    if(maxfdp < room->fd_broadcast_in){
        maxfdp = room->fd_broadcast_in;
    }
    if(maxfdp < room->fd_broadcast_out){
        maxfdp = room->fd_broadcast_out;
    }
    maxfdp++;

    char bf[512];
    room->listener->onPrintLog("server start");
    fd_set readset, writeset;     // 读描述符集
    while (true){
        FD_ZERO(&readset);   // 在使用之前总是要清空
        FD_ZERO(&writeset);   // 在使用之前总是要清空

        // 开始使用select
        FD_SET(room->fd_broadcast_in, &readset);
        FD_SET(room->fd_in, &readset);
        //FD_SET(room->fd_broadcast_out, &writeset);
        //FD_SET(room->fd_out, &writeset);

        ret = select(maxfdp, &readset, &writeset, NULL, NULL);// 检测是否有套接口是否可读写
        if(ret < 0){
            listener->onError("select error");
            return;
        }else if(ret == 0){
            listener->onError("select timeout");
        }else{
            if(FD_ISSET(room->fd_broadcast_in, &readset)){
                Message *message = popIdleMsg(room);
                memset(&addr, 0, sizeof(addr));
                len = sizeof(addr);
                int ret = recvfrom(room->fd_broadcast_in, message->msg, 512, 0, (struct sockaddr*)&addr, &len);
                if(ret < 0){
                    sprintf(bf, "receive error :%d" ,errno);
                }else{
                    sprintf(bf, "receive from[%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), message->msg);
                }
                listener->onPrintLog(bf);
                message->ip = addr.sin_addr.s_addr;
                message->port = addr.sin_port;

                strcpy(message->msg, "permit!");
                addSendMsg(room, message);
            }

            if(FD_ISSET(room->fd_broadcast_out, &writeset)){
                Message *message = popBroadcastMsg(room);
                if(message != NULL){
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
                    addr.sin_port = htons(message->port);
                    len = sizeof(addr);
                    int ret = sendto(room->fd_broadcast_out, message->msg, strlen(message->msg), 0, (struct sockaddr*)&addr, len);
                    if(ret < 0){
                        sprintf(bf, "send error :%d" ,errno);
                    }else{
                        sprintf(bf, "send to [%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), message->msg);
                    }
                    listener->onPrintLog(bf);
                    addIdleMsg(room, message);
                }
            }

            if(FD_ISSET(room->fd_in, &readset)){
                memset(&addr, 0, sizeof(addr));
                Message *message = popIdleMsg(room);
                recvfrom(room->fd_in, message->msg, 512, 0, (struct sockaddr*)&addr, &len);
                message->ip = addr.sin_addr.s_addr;
                message->port = addr.sin_port;
                room->listener->onPrintLog(message->msg);

                strcpy(message->msg, "ack!");
                addSendMsg(room, message);
            }

            if(FD_ISSET(room->fd_out, &writeset)){
                Message *message = popSendMsg(room);
                if(message != NULL){
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_addr.s_addr = htonl(message->ip);
                    addr.sin_port = htons(message->port);

                    sendto(room->fd_out, message->msg, 512, 0, (struct sockaddr*)&addr, len);

                    addIdleMsg(room, message);
                }
            }
        }
    }
}


void addBroadcastMsg(Room *room, Message *message) {
    if(room->broadcastTail != NULL){
        room->broadcastTail->next = message;
        message->pre = room->broadcastTail;
        message->next = NULL;
        room->broadcastTail = message;
    }else{
        message->pre = NULL;
        message->next = NULL;
        room->broadcastHead = room->broadcastTail = message;
    }
}

void addSendMsg(Room *room, Message *message) {
    if(room->sendMsgTail != NULL){
        room->sendMsgTail->next = message;
        message->pre = room->sendMsgTail;
        message->next = NULL;
        room->sendMsgTail = message;
    }else{
        message->pre = NULL;
        message->next = NULL;
        room->sendMsgHead = room->sendMsgTail = message;
    }
}


void addIdleMsg(Room *room, Message *message) {
    if(room->idleMsgs != NULL){
        room->idleMsgs->pre = message;
        message->next = room->idleMsgs;
        message->pre = NULL;
        room->idleMsgs = message;
    }else{
        room->idleMsgs = message;
        room->idleMsgs->next = NULL;
        room->idleMsgs->pre = NULL;
    }
}



Message * popBroadcastMsg(Room *room) {
    if(room->broadcastHead != NULL){
        Message *message = room->broadcastHead;
        room->broadcastHead = message->next;
        if(room->broadcastHead == NULL){
            room->broadcastTail = NULL;
        }
        message->next = NULL;
        message->pre = NULL;
        message->port = 55555;
        return message;
    }
    return NULL;
}

Message * popSendMsg(Room *room) {
    if(room->sendMsgHead != NULL){
        Message *message = room->sendMsgHead;
        room->sendMsgHead = message->next;
        if(room->sendMsgHead == NULL){
            room->sendMsgTail = NULL;
        }
        message->next = NULL;
        message->pre = NULL;
        return message;
    }
    return NULL;
}


Message * popIdleMsg(Room *room) {
    if(room->idleMsgs != NULL){
        Message *message = room->idleMsgs;
        room->idleMsgs = message->next;
        if(room->idleMsgs != NULL){
            room->idleMsgs->pre = NULL;
        }
        message->next = NULL;
        message->pre = NULL;
        return message;
    }else{
        Message *message = (Message *)malloc(sizeof(Message));
        memset(message, 0, sizeof(Message));
        message->next = NULL;
        message->pre = NULL;
        return message;
    }
}

int openSocket(__be32 ip, __be16 port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6000);
    socklen_t len = sizeof(addr);
    return 0;
}

void startClient(RoomListener *listener) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1){
        (*listener->onError)("socket error");
        return;
    }

    const int opt = -1;
    int ret = 0;
    ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(ret == -1){
        listener->onError("set socket error");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(55555);
    socklen_t len = sizeof(addr);

    ret = bind(fd, (struct sockaddr*)&addr, len);
    if(ret == -1){
        listener->onError("bind socket error");
        return;
    }
    char bf[512];
    listener->onPrintLog("client start");
    while (true){
        int ret = recvfrom(fd, bf, 512, 0, (struct sockaddr*)&addr, &len);
        if(ret <=0){
            listener->onPrintLog("recv error");
            return;
        }else{
            listener->onPrintLog(bf);
        }
    }
}


void setnonblocking(int sockfd) {
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag < 0) {
        return;
    }
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
}

void sendBroadcast(Room *room, const char *str) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(55555);
    socklen_t len = sizeof(addr);
    char bf[128];
    size_t ret = sendto(room->fd_broadcast_out, str, strlen(str), 0, (struct sockaddr*)&addr, len);
    if(ret < 0){
        sprintf(bf, "send error :%d" ,errno);
    }else{
        sprintf(bf, "to[%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),str);
    }
    room->listener->onPrintLog(bf);
}

void receiveBroadcast(Room *room) {
    room->fd_broadcast_in = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_broadcast_in == -1){
        room->listener->onError("socket error");
        return;
    }

    const int opt = -1;
    int ret = 0;
    ret = setsockopt(room->fd_broadcast_in, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(ret == -1){
        room->listener->onError("set socket error");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(55555);
    socklen_t len = sizeof(addr);

    ret = bind(room->fd_broadcast_in, (struct sockaddr*)&addr, len);
    if(ret == -1){
        room->listener->onError("bind socket error");
        return;
    }

    char tmp[1024];
    char bf[1024];
    while(true){
        memset(&addr, 0, sizeof(addr));
        len = sizeof(addr);
        size_t ret = recvfrom(room->fd_broadcast_in, bf, 1024, 0, (struct sockaddr*)&addr, &len);
        if(ret < 0){
            sprintf(tmp, "receive error :%d" ,errno);
            return;
        }else{
            sprintf(tmp, "from[%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), bf);
        }

        room->listener->onPrintLog(tmp);
    }




}

void receiveMsg(Room *room) {
    char tmp[1024];
    room->fd_in = socket(AF_INET, SOCK_DGRAM, 0);
    if(room->fd_in == -1){
        sprintf(tmp, "socket error :%d" ,errno);
        room->listener->onError(tmp);
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("172.28.77.4");
    addr.sin_port = htons(55556);
    socklen_t len = sizeof(addr);

    int ret = bind(room->fd_in, (struct sockaddr*)&addr, len);
    if(ret == -1){
        sprintf(tmp, "bind socket error :%d" ,errno);
        room->listener->onError(tmp);
        return;
    }


    sprintf(tmp, "start receive[%s:%d]", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    room->listener->onPrintLog(tmp);

    char bf[1024];
    while(true){
        memset(bf, 0, sizeof(bf));
        ssize_t ret = recvfrom(room->fd_in, bf, 1024, 0, (struct sockaddr*)&addr, &len);
        if(ret < 0){
            sprintf(tmp, "receive error :%d" ,errno);
            return;
        }else{
            sprintf(tmp, "from[%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), bf);
        }

        room->listener->onPrintLog(tmp);
    }
}

void sendMsg(Room *room, const char *str) {
   if( room->fd_out < 0){
       room->fd_out = socket(AF_INET, SOCK_DGRAM, 0);
       if(room->fd_out == -1){
           room->listener->onError("socket error");
           return;
       }
   }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("172.28.77.4");
    addr.sin_port = htons(55556);
    socklen_t len = sizeof(addr);
    char bf[128];
    size_t ret = sendto(room->fd_out, str, strlen(str), 0, (struct sockaddr*)&addr, len);
    if(ret < 0){
        sprintf(bf, "send error :%d" ,errno);
    }else{
        sprintf(bf, "to[%s:%d]: %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),str);
    }
    room->listener->onPrintLog(bf);
}


