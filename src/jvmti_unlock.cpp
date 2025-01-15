
#include "jvmti_unlock.h"
#include "hooks.h"

jvmtiEnv* g_jvmti;
JNIEnv*   g_env;

JVMTI_UNLOCK_STATUS JVMTIU_Initialize() { return JVMTIU_Initialize(&g_env, &g_jvmti); }

JVMTI_UNLOCK_STATUS JVMTIU_Initialize(JNIEnv** jni, jvmtiEnv** jvmti)
{
    jsize   vm_count;
    JavaVM* jvm = NULL;

    if (JNI_GetCreatedJavaVMs(&jvm, 1, &vm_count) != JNI_OK || vm_count == 0)
    {
        return JVMTIU_JNI_ENV_NOT_ACCESSIBLE;
    }

    if (jvm->GetEnv((void**)&jni, JNI_VERSION_1_6) != JNI_OK)
    {
        return JVMTIU_JNI_ENV_NOT_ACCESSIBLE;
    }

    if (jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2) != JNI_OK || jvmti == NULL)
    {
        return JVMTIU_JVMTI_ENV_NOT_ACCESSIBLE;
    }

    g_jvmti = *jvmti;
    g_env   = *jni;

    return JVMTIU_SUCCESS;
}

JVMTI_UNLOCK_STATUS JVMTIU_UseExisting(JNIEnv* jni, jvmtiEnv* g_jvmti)
{
    if (jni == NULL || g_jvmti == NULL)
    {
        return JVMTIU_INVALID_PARAMETER;
    }

    g_env   = jni;
    g_jvmti = g_jvmti;
    return JVMTIU_SUCCESS;
}

JVMTI_UNLOCK_STATUS Install() 
{
    if (HookGetCapabilities(g_jvmti) &&
        PatchAddCapabilities(g_jvmti) &&
        PatchSetEventNotifications(g_jvmti))
    {
        // we need to trigger an update of the capabilities in order for the jvm
        // to recognize the changes, so we just request whichever capabilities 
        // are available atm.
        jvmtiCapabilities caps;
        g_jvmti->GetPotentialCapabilities(&caps);
        g_jvmti->AddCapabilities(&caps);
        return JVMTIU_SUCCESS;
    }

    Uninstall();
    return JVMTIU_ERROR;
}

jvmtiEnv* GetJVMTI() { return g_jvmti; }

JNIEnv* GetJNI() { return g_env; }