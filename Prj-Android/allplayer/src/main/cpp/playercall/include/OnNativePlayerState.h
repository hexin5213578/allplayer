//
// Created by lhf on 5/18/21.
//

#ifndef PRJ_ANDROID_ONNATIVEPLAYERSTATE_H
#define PRJ_ANDROID_ONNATIVEPLAYERSTATE_H


#include "JavaListener.h"

class OnNativePlayerState : public JavaListener {

private:
    const char *LOG_TAG = "_OnNativePlayerState";

public:
    OnNativePlayerState(JavaVM *jvm, JNIEnv *mainEnv, jobject obj)
            : JavaListener(jvm, mainEnv, obj) {
    }

    ~OnNativePlayerState() {
    };

    const char *getMethodName() {
        return "onNativePlayerState";
    }

    const char *getMethodSignature() {
        return "(Ljava/lang/String;I)V";
    }

    void reallyCallback(JNIEnv *env, jobject obj, jmethodID methodId, va_list args) {
        char *msg = va_arg(args, char *);
        int status = va_arg(args, int);
        jstring stringUtf = env->NewStringUTF(msg);
        env->CallVoidMethod(obj, methodId, stringUtf,status);
        env->DeleteLocalRef(stringUtf);
    }

};


#endif //PRJ_ANDROID_ONNATIVEPLAYERSTATE_H
