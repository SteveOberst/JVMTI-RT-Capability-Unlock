# JVMTI Capability Unlocker

The **JVMTI Capability Unlocker** is a project that enables unlocking JVM capabilities during runtime, which are
normally restricted or inaccessible. By leveraging hooks, patches, and precise memory manipulation, this project allows
access to advanced debugging and event generation features of the Java Virtual Machine Tool Interface (JVMTI).

## Features

- **Unlock All JVMTI Capabilities**: Overrides internal checks and grants access to all available JVMTI capabilities,
  even those not normally accessible.
- **Advanced Hooking and Patching**:
    - Hooks into JVM functions to bypass capability restrictions.
    - Patches checks in the JVM to enable capabilities without internal validation.
- **Dynamic Capability Modification**: Modifies the capabilities structure dynamically to ensure maximum compatibility
  across JVM versions.
- **Low-Level Memory Manipulation**: Implements signature-based pattern searching to locate and patch critical sections
  of the JVM.

## How It Works

### Hooks

The project uses hooks and patches certain JVM functionality in order to allow access to normally restricted parts of the
JVMTI interface. By detouring and patching specific functions, the unlocker can bypass internal checks and restrictions.

For Example, we're detouring the internal get_capabilities function in the jvm (not the one exposed through JVMTI), making
it seem like all capabilities are enabled.

```cpp
jvmtiCapabilities* DetourGetCapabilities(jvmtiEnv* jvmti)
{
    if (!g_AllCapabilitiesInitialized.load())
    {
        std::lock_guard<std::mutex> lock(g_CapabilitiesMutex);
        if (!g_AllCapabilitiesInitialized.load())
        {
            jvmtiCapabilities* capabilities = (jvmtiCapabilities*)malloc(sizeof(jvmtiCapabilities));

            if (!capabilities)
            {
                // eh, this is fatal... we'll try not to crash the process, no guarantee tho :D
                printf("[HookLibraryNative] Failed to allocate memory for capabilities in detour! "
                       "This is fatal, trying to recover...");
                MH_DisableHook(MH_ALL_HOOKS);
                return g_oCapabilitiesPtr;
            }

            g_AllCapabilities = capabilities;
            g_AllCapabilitiesInitialized.store(true);
        }
    }

    enable_all_capabilities(g_AllCapabilities);
    return g_AllCapabilities; // Return the pointer to the capabilities object
}
```

Unsurprisingly, this is not enough to unlock all functionality as the JVM has additional checks in place. We also need to
trigger an internal update by requesting some arbitrary capability in order for the JVM to update internal mechanisms.

### Patches

Patches are applied to bypass internal JVM checks:

- **Capability Validation Patch**: Disables checks that restrict the addition or modification of capabilities.
- **Set Event Notifications Patch**: Removes checks that verify if certain capabilities are set before enabling specific
  event notifications.

### Memory Pattern Scanning

Signatures and pattern masks are used to locate addresses in the JVM memory dynamically.
These signatures were manually identified and verified across multiple JVM versions to ensure compatibility.

## Code Overview

### Core Interface

The **JVMTI Capability Unlocker** provides a clean and user-friendly interface to interact with its core functionality.
Below are the primary components and their purposes:

### Enums

#### `JVMTI_UNLOCK_STATUS`

This enum defines the status codes returned by the interface functions to indicate success or error states. The possible
values are:

- **`JVMTIU_UNKNOWN`**: An unknown error occurred.
- **`JVMTIU_SUCCESS`**: Operation completed successfully.
- **`JVMTIU_ERROR`**: A general error occurred.
- **`JVMTIU_JNI_ENV_NOT_ACCESSIBLE`**: The JNI environment could not be accessed.
- **`JVMTIU_JVMTI_ENV_NOT_ACCESSIBLE`**: The JVMTI environment could not be accessed.
- **`JVMTIU_JVM_NOT_INITIALIZED`**: The JVM is not yet initialized.
- **`JVMTIU_ALREADY_INITIALIZED`**: The unlocker is already initialized.
- **`JVMTIU_INVALID_PARAMETER`**: An invalid parameter was passed to a function.

### Functions

#### `JVMTIU_Initialize()`

Initializes the unlocker using default settings. It detects and attaches to the current JVM environment.

- **Returns**: `JVMTIU_SUCCESS` if initialization is successful, otherwise an appropriate error code.

#### `JVMTIU_Initialize(JNIEnv** jni, jvmtiEnv** jvmti)`

Initializes the unlocker using explicitly provided `JNIEnv` and `jvmtiEnv` pointers.

- **Parameters**:
    - `jni`: Pointer to the JNI environment.
    - `jvmti`: Pointer to the JVMTI environment.

- **Returns**: `JVMTIU_SUCCESS` if initialization is successful, otherwise an appropriate error code.

#### `JVMTIU_UseExisting(JNIEnv* jni, jvmtiEnv* jvmti)`

Allows the unlocker to use an existing `JNIEnv` and `jvmtiEnv`.

- **Parameters**:
    - `jni`: The existing JNI environment.
    - `jvmti`: The existing JVMTI environment.

- **Returns**: `JVMTIU_SUCCESS` if the existing environments are valid, otherwise an appropriate error code.

#### `Install()`

Installs the hooks and patches required to enable all JVMTI capabilities.

- **Returns**: `JVMTIU_SUCCESS` if installation is successful, otherwise an appropriate error code.

#### `Uninstall()`

Cleans up the unlocker, removes hooks, and restores the JVM to its original state.

#### `GetJVMTI()`

Retrieves the current `jvmtiEnv` pointer.

- **Returns**: A pointer to the active JVMTI environment.

#### `GetJNI()`

Retrieves the current `JNIEnv` pointer.

- **Returns**: A pointer to the active JNI environment.

## Pitfalls I Encountered
Something I found myself stuck with for a couple of hours was manipulating the jvmtiCapabilities structure itself.
I was facing weird errors when trying to memset the whole struct to 1, but it turns out that the compiler optimizes
the struct to represent each unsigned int in a bitmask, so I was essentially only setting every 8th field to 1 
(and corrupting some memory). So I came up with this function to set each bit to 1:
```cpp
void enable_all_capabilities(jvmtiCapabilities* capa)
{
    // The compiler optimizes the unsigned integers in the jvmtiCapabilities struct to
    // single bits, so we need to set each *bit* to 1

    // This actually took me longer to figure out than I'd like to admit. I was trying to memset
    // the whole struct to 1, but the compiler optimizes the struct to single bits, so I was
    // essentially only setting every 8th field to 1 (and corrupting some memory).

    // we just don't give a flying f about the current or prohibited capabilities and
    // just allow everything
    for (int i = 0; i < JVMTI_INTERNAL_CAPABILITY_COUNT; i++)
    {
        int byte_index = i / 8; // Index of the byte in the array
        int bit_index  = i % 8; // The specific bit within that byte

        // Set the specific bit to 1
        ((char*)capa)[byte_index] |= (1 << bit_index);
    }
}
```

### Important Notes

1. **Capability Structure Optimization**: The `jvmtiCapabilities` structure is optimized by the compiler, which stores
   individual bits instead of full bytes. This requires bit-by-bit manipulation rather than using functions
   like `memset`.
2. **Compatibility**: The project works across multiple JVM versions but may not support newly introduced capabilities
   in the latest JVMs.
3. **Thread Safety**: Uses mutexes and atomic flags to ensure thread-safe initialization of global capabilities.

## Dependencies

- **[MinHook](https://github.com/TsudaKageyu/minhook)**: A lightweight and fast hooking library used to create function
  detours.
- **C++11 or Higher**: Required for atomic operations, mutexes, and modern C++ features.

## Limitations

- Modifying JVM memory can lead to instability if misused.
- Some capabilities might still be restricted by the operating system or JVM safeguards.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Disclaimer

This tool is for educational and research purposes only. Use it responsibly and ensure compliance with applicable laws
and regulations.

