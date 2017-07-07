#ifndef XSOCK_H
#define XSOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/epoll.h>

#define true 1

typedef struct Message Message;
typedef struct Room Room;
typedef struct RoomMember RoomMember;
typedef struct RoomListener RoomListener;
typedef struct AddressNode AddressNode;
struct Message{
    __be32 ip;
    __be16 port;
    char msg[512];
    Message *next;
    Message *pre;
};

struct RoomMember{
    int fd_in;
    int fd_out;
    __be32 ip;
    __be16 port;
    char *name;
    RoomMember *next;
};

struct Room{
    int fd_broadcast_in;
    int fd_broadcast_out;
    int fd_in;
    int fd_out;
    int minMember;
    int maxMember;
    int curMember;
    RoomMember *members;
    RoomMember *host;
    Message *sendMsgHead;
    Message *sendMsgTail;

    Message *broadcastHead;
    Message *broadcastTail;

    Message *idleMsgs;
    RoomListener *listener;
    Room *next;
};

struct RoomListener{
    void (*onPrintLog)(const char *);
    void (*onEnterMember)(RoomMember *);
    void (*onLeaveMember)(RoomMember *);
    void (*onError)(const char *);
};

struct AddressNode{
    __be32 ip;
    __be16 port;
    AddressNode *next;
    AddressNode *pre;
};


extern void startServer(Room *room, RoomListener *listener);
extern void startClient(RoomListener *listener);


extern void closeRoom(Room *room);
extern void requestEnter(Room *room, RoomListener *listener);
extern void exitRoom(Room *room, RoomListener *listener);


extern void addBroadcastMsg(Room *room, Message *message);
extern void addSendMsg(Room *room, Message *message);
extern void addIdleMsg(Room *room, Message *message);

extern Message* popBroadcastMsg(Room *room);
extern Message* popSendMsg(Room *room);
extern Message* popIdleMsg(Room *room);
extern int openSocket(__be32 ip, __be16 port);

extern void sendBroadcast(Room *room,const char *str);
extern void receiveBroadcast(Room *room);
//设置非阻塞
static void setnonblocking(int sockfd);







#ifdef __cplusplus
}
#endif
#endif //XSOCK_H
