#include <jni.h>
#include <string>
#include "xsock.h"
#include <android/log.h>



extern "C" {

Room *room = NULL;
char log[512][512];
int logIndex = 0;

void onPrintLog(const char *str){
    strcpy(log[logIndex], str);
    logIndex++;
    __android_log_print(ANDROID_LOG_DEBUG, "xsock","%s", str);

}

void onError(const char *str){
    strcpy(log[logIndex], str);
    logIndex++;
    __android_log_print(ANDROID_LOG_ERROR, "xsock","%s", str);
}

JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_getOnLog(JNIEnv *env, jobject instance) {
    if(logIndex > 0){
        jclass cls = env->GetObjectClass(instance);
        jmethodID callback = env->GetMethodID(cls,"onLog","(Ljava/lang/String;)V");
        int i = 0;
        for(i;i<logIndex;i++){
            env->CallVoidMethod(instance,callback, env->NewStringUTF(log[i]));
        }

        logIndex = 0;
    }
}


JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_broadcastMsg(JNIEnv *env, jobject instance, jstring sendMsg_) {
    const char *sendMsg = env->GetStringUTFChars(sendMsg_, 0);

    if(room != NULL){
//        Message *message = popIdleMsg(room);
//        strcpy(message->msg, sendMsg);
//        addBroadcastMsg(room, message);
//        room->listener->onPrintLog("add broadcast");
        sendBroadcast(room, sendMsg);
    }

    env->ReleaseStringUTFChars(sendMsg_, sendMsg);
}

JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_startServer(JNIEnv *env, jobject instance, jstring name_) {
    const char *name = env->GetStringUTFChars(name_, 0);
    room = (Room*)malloc(sizeof(Room));
    room->broadcastHead = NULL;
    room->broadcastTail = NULL;
    room->sendMsgHead = NULL;
    room->sendMsgTail = NULL;
    room->idleMsgs = NULL;

    RoomListener *listener = (RoomListener*)malloc(sizeof(RoomListener));
    listener->onPrintLog = onPrintLog;
    listener->onError = onError;
    listener->onEnterMember = NULL;
    listener->onLeaveMember = NULL;

    room->listener = listener;
    receiveBroadcast(room);

    //startServer(room, listener);
    env->ReleaseStringUTFChars(name_, name);
}


JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_startClient(JNIEnv *env, jobject instance) {
    RoomListener *listener = (RoomListener*)malloc(sizeof(RoomListener));
    listener->onPrintLog = onPrintLog;
    listener->onError = onError;
    listener->onEnterMember = NULL;
    listener->onLeaveMember = NULL;
    startClient(listener);
}


JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_receive(JNIEnv *env, jobject instance) {

    // TODO
    room = (Room*)malloc(sizeof(Room));
    room->fd_out = -1;
    room->broadcastHead = NULL;
    room->broadcastTail = NULL;
    room->sendMsgHead = NULL;
    room->sendMsgTail = NULL;
    room->idleMsgs = NULL;

    RoomListener *listener = (RoomListener*)malloc(sizeof(RoomListener));
    listener->onPrintLog = onPrintLog;
    listener->onError = onError;
    listener->onEnterMember = NULL;
    listener->onLeaveMember = NULL;

    room->listener = listener;
    receiveMsg(room);
}


JNIEXPORT void JNICALL
Java_com_forthe_xsock_MainActivity_send(JNIEnv *env, jobject instance, jstring sendMsg_) {
    const char *msg = env->GetStringUTFChars(sendMsg_, 0);

    if(room != NULL){
        sendMsg(room, msg);
    }

    env->ReleaseStringUTFChars(sendMsg_, msg);
}

}
