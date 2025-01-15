#pragma once

#include "jni.h"
#include "jvmti.h"

typedef enum _JVMTI_UNLOCK_STATUS
{
    JVMTIU_UNKNOWN,
    JVMTIU_SUCCESS,
    JVMTIU_ERROR,
    JVMTIU_JNI_ENV_NOT_ACCESSIBLE,
    JVMTIU_JVMTI_ENV_NOT_ACCESSIBLE,
    JVMTIU_JVM_NOT_INITIALIZED,
    JVMTIU_ALREADY_INITIALIZED,
    JVMTIU_INVALID_PARAMETER,
} JVMTI_UNLOCK_STATUS, jvmti_unlock_status_t;

JVMTI_UNLOCK_STATUS JVMTIU_Initialize();

JVMTI_UNLOCK_STATUS JVMTIU_Initialize(JNIEnv** jni, jvmtiEnv** jvmti);

JVMTI_UNLOCK_STATUS JVMTIU_UseExisting(JNIEnv* jni, jvmtiEnv* jvmti);

JVMTI_UNLOCK_STATUS Install();

void Uninstall();

jvmtiEnv* GetJVMTI();

JNIEnv* GetJNI();