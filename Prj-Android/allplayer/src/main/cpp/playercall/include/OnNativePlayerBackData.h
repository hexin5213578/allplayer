//
// Created by lhf on 5/28/21.
//

#ifndef PRJ_ANDROID_ONNATIVEPLAYERBACKDATA_H
#define PRJ_ANDROID_ONNATIVEPLAYERBACKDATA_H

#include "JavaListener.h"

class OnNativePlayerBackData : public JavaListener {

private:
    const char *LOG_TAG = "_OnNativePlayerState";

public:
    OnNativePlayerBackData(JavaVM *jvm, JNIEnv *mainEnv, jobject obj) : JavaListener(jvm, mainEnv, obj ) {
    }

    ~OnNativePlayerBackData() {
    };

    const char *getMethodName() {
        return "onNativePlayerBackTime";
    }

    const char *getMethodSignature() {
        return "(I)V";
    }

    void reallyCallback(JNIEnv *env, jobject obj, jmethodID methodId, va_list args) {
        int status = va_arg(args, int);
        env->CallVoidMethod(obj, methodId, status);
    }

};


#endif //PRJ_ANDROID_ONNATIVEPLAYERBACKDATA_H
