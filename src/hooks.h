#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <cstdbool>
#include <string.h>
#include <atomic>
#include <mutex>

#include <Windows.h>

#include "MinHook.h"
#include "jvmti.h"
#include "jni.h"

#include "mem_scan.h"

typedef void (*f_GetPotentialCapabilities)(
    const jvmtiCapabilities* current,
    const jvmtiCapabilities* prohibited,
    jvmtiCapabilities*       result);

typedef jvmtiCapabilities* (*f_DirectGetCapabilities)(jvmtiEnv* env);

bool HookGetCapabilities(jvmtiEnv* jvmti);

bool PatchSetEventNotifications(jvmtiEnv* jvmti);

bool PatchAddCapabilities(jvmtiEnv* jvmti);