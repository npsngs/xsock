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

#define true 1

typedef struct Message Message;
typedef struct Room Room;
typedef struct RoomMember RoomMember;
typedef struct RoomListener RoomListener;

struct Message{
    __be32 ip;
    __be16 port;
    char msg[512];
    Message *next;
    Message *pre;
};

struct RoomMember{
    __be32 ip;
    __be16 port;
    char *name;
    RoomMember *next;
};

struct Room{
    int minMember;
    int maxMember;
    int curMember;
    RoomMember *members;
    RoomMember *host;

};

struct RoomListener{
    void (*onEnterMember)(RoomMember *);
    void (*onLeaveMember)(RoomMember *);
    void (*onError)(const char *);
};


extern void openRoom(Room *room, RoomListener *listener);
extern void closeRoom(Room *room);

















#ifdef __cplusplus
}
#endif
#endif //XSOCK_H
