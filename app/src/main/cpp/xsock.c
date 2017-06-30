#include "xsock.h"
//http://www.tuicool.com/articles/7BNZJz2
void openRoom(Room *room, RoomListener *listener){
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        (*listener->onError)("socket error");
        return;
    }

    const int opt = -1;
    int ret = 0;
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if(ret == -1){
        (*listener->onError)("set socket error");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6000);
    socklen_t len = sizeof(addr);

    ret = bind(sock, (struct sockaddr*)&addr, len);
    if(ret == -1){
        (*listener->onError)("bind socket error");
        return;
    }

    char bf[512] = {0};
    ssize_t size;
    while (true){
        size = recvfrom(sock, bf, 512, 0, (struct sockaddr*)&addr, &len);
        if(size <= 0){
            goto normal;
        }else{
            char *ip = inet_ntoa(addr.sin_addr);
            RoomMember *member = (RoomMember*)malloc (sizeof(RoomMember));
            member->ip = addr.sin_addr.s_addr;
            member->port = addr.sin_port;
            member->name = strcat(ip, "\0");
            member->next = room->members;
            room->members = member;
            room->curMember++;
            if(listener != NULL){
                (*listener->onEnterMember)(member);
            }

            if(room->curMember >= room->maxMember){
                return;
            }
        }
    }


    normal:
        return;


}