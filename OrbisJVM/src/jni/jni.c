/**
 * @file jni.c
 * @brief OrbisJVM JNI Compatibility Layer Implementation
 *
 * Provides JNI (Java Native Interface) compatibility for OrbisJVM.
 * This is a stub implementation - actual JNI requires JVM integration.
 */

#include <orbisjvm/jni.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>

#include <stdlib.h>
#include <string.h>

/* JNI extension functions table */
static OrbisJVM_JNI_Extensions g_jni_extensions = {0};

/**
 * @brief Initialize JNI compatibility layer
 */
int orbisjvm_jni_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_JNI, "Initializing JNI compatibility layer");

    /* Set up extension function pointers */
    g_jni_extensions.orbisjvm_create_jvm = NULL;  /* To be implemented with JVM */
    g_jni_extensions.orbisjvm_destroy_jvm = NULL;
    g_jni_extensions.orbisjvm_attach_thread = NULL;
    g_jni_extensions.orbisjvm_detach_thread = NULL;
    g_jni_extensions.orbisjvm_alloc_jvm_memory = NULL;
    g_jni_extensions.orbisjvm_free_jvm_memory = NULL;
    g_jni_extensions.orbisjvm_create_java_thread = NULL;
    g_jni_extensions.orbisjvm_join_java_thread = NULL;
    g_jni_extensions.orbisjvm_find_class_from_path = NULL;
    g_jni_extensions.orbisjvm_define_class_from_bytes = NULL;
    g_jni_extensions.orbisjvm_dump_heap = NULL;
    g_jni_extensions.orbisjvm_gc = NULL;

    orbisjvm_log_warn(ORBISJVM_LOG_CAT_JNI, "JNI layer initialized (stub - no JVM attached)");
    return 0;
}

/**
 * @brief Shutdown JNI compatibility layer
 */
void orbisjvm_jni_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_JNI, "Shutting down JNI compatibility layer");
    memset(&g_jni_extensions, 0, sizeof(g_jni_extensions));
}

/**
 * @brief Get OrbisJVM JNI extensions
 */
const OrbisJVM_JNI_Extensions* orbisjvm_get_jni_extensions(void) {
    return &g_jni_extensions;
}

/* JNI stub functions - these would be implemented when JVM is integrated */

JNIEXPORT jint JNICALL JNI_GetVersion(JNIEnv* env) {
    (void)env;
    return ORBISJVM_JNI_VERSION_17;
}

JNIEXPORT jclass JNICALL JNI_FindClass(JNIEnv* env, const char* name) {
    (void)env; (void)name;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_JNI, "FindClass called (stub): %s", name ? name : "NULL");
    return NULL;
}

JNIEXPORT jint JNICALL JNI_ThrowNew(JNIEnv* env, jclass clazz, const char* msg) {
    (void)env; (void)clazz; (void)msg;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_JNI, "ThrowNew called (stub): %s", msg ? msg : "NULL");
    return JNI_ERR;
}

JNIEXPORT jthrowable JNICALL JNI_ExceptionOccurred(JNIEnv* env) {
    (void)env;
    return NULL;
}

JNIEXPORT void JNICALL JNI_ExceptionClear(JNIEnv* env) {
    (void)env;
}

JNIEXPORT jobject JNICALL JNI_NewGlobalRef(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
    return NULL;
}

JNIEXPORT void JNICALL JNI_DeleteGlobalRef(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
}

JNIEXPORT jobject JNICALL JNI_NewLocalRef(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
    return NULL;
}

JNIEXPORT void JNICALL JNI_DeleteLocalRef(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
}

JNIEXPORT jobject JNICALL JNI_AllocObject(JNIEnv* env, jclass clazz) {
    (void)env; (void)clazz;
    return NULL;
}

JNIEXPORT jclass JNICALL JNI_GetObjectClass(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
    return NULL;
}

JNIEXPORT jfieldID JNICALL JNI_GetFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    (void)env; (void)clazz; (void)name; (void)sig;
    return NULL;
}

JNIEXPORT jobject JNICALL JNI_GetObjectField(JNIEnv* env, jobject obj, jfieldID fieldID) {
    (void)env; (void)obj; (void)fieldID;
    return NULL;
}

JNIEXPORT jint JNICALL JNI_GetIntField(JNIEnv* env, jobject obj, jfieldID fieldID) {
    (void)env; (void)obj; (void)fieldID;
    return 0;
}

JNIEXPORT void JNICALL JNI_SetObjectField(JNIEnv* env, jobject obj, jfieldID fieldID, jobject val) {
    (void)env; (void)obj; (void)fieldID; (void)val;
}

JNIEXPORT void JNICALL JNI_SetIntField(JNIEnv* env, jobject obj, jfieldID fieldID, jint val) {
    (void)env; (void)obj; (void)fieldID; (void)val;
}

JNIEXPORT jmethodID JNICALL JNI_GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    (void)env; (void)clazz; (void)name; (void)sig;
    return NULL;
}

JNIEXPORT jobject JNICALL JNI_CallObjectMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) {
    (void)env; (void)obj; (void)methodID;
    return NULL;
}

JNIEXPORT jint JNICALL JNI_CallIntMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) {
    (void)env; (void)obj; (void)methodID;
    return 0;
}

JNIEXPORT void JNICALL JNI_CallVoidMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) {
    (void)env; (void)obj; (void)methodID;
}

JNIEXPORT jmethodID JNICALL JNI_GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    (void)env; (void)clazz; (void)name; (void)sig;
    return NULL;
}

JNIEXPORT jobject JNICALL JNI_CallStaticObjectMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) {
    (void)env; (void)clazz; (void)methodID;
    return NULL;
}

JNIEXPORT jint JNICALL JNI_CallStaticIntMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) {
    (void)env; (void)clazz; (void)methodID;
    return 0;
}

JNIEXPORT void JNICALL JNI_CallStaticVoidMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) {
    (void)env; (void)clazz; (void)methodID;
}

JNIEXPORT jfieldID JNICALL JNI_GetStaticFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    (void)env; (void)clazz; (void)name; (void)sig;
    return NULL;
}

JNIEXPORT jint JNICALL JNI_GetStaticIntField(JNIEnv* env, jclass clazz, jfieldID fieldID) {
    (void)env; (void)clazz; (void)fieldID;
    return 0;
}

JNIEXPORT void JNICALL JNI_SetStaticIntField(JNIEnv* env, jclass clazz, jfieldID fieldID, jint val) {
    (void)env; (void)clazz; (void)fieldID; (void)val;
}

JNIEXPORT jstring JNICALL JNI_NewStringUTF(JNIEnv* env, const char* utf) {
    (void)env; (void)utf;
    return NULL;
}

JNIEXPORT const char* JNICALL JNI_GetStringUTFChars(JNIEnv* env, jstring str, jboolean* isCopy) {
    (void)env; (void)str; (void)isCopy;
    return NULL;
}

JNIEXPORT void JNICALL JNI_ReleaseStringUTFChars(JNIEnv* env, jstring str, const char* chars) {
    (void)env; (void)str; (void)chars;
}

JNIEXPORT jsize JNICALL JNI_GetArrayLength(JNIEnv* env, jarray array) {
    (void)env; (void)array;
    return 0;
}

JNIEXPORT jintArray JNICALL JNI_NewIntArray(JNIEnv* env, jsize len) {
    (void)env; (void)len;
    return NULL;
}

JNIEXPORT jint* JNICALL JNI_GetIntArrayElements(JNIEnv* env, jintArray array, jboolean* isCopy) {
    (void)env; (void)array; (void)isCopy;
    return NULL;
}

JNIEXPORT void JNICALL JNI_ReleaseIntArrayElements(JNIEnv* env, jintArray array, jint* elems, jint mode) {
    (void)env; (void)array; (void)elems; (void)mode;
}

JNIEXPORT void JNICALL JNI_GetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint* buf) {
    (void)env; (void)array; (void)start; (void)len; (void)buf;
}

JNIEXPORT void JNICALL JNI_SetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, const jint* buf) {
    (void)env; (void)array; (void)start; (void)len; (void)buf;
}

JNIEXPORT jint JNICALL JNI_GetJavaVM(JNIEnv* env, JavaVM** vm) {
    (void)env; (void)vm;
    return JNI_ERR;
}

JNIEXPORT jboolean JNICALL JNI_ExceptionCheck(JNIEnv* env) {
    (void)env;
    return JNI_FALSE;
}

JNIEXPORT jobject JNICALL JNI_NewDirectByteBuffer(JNIEnv* env, void* address, jlong capacity) {
    (void)env; (void)address; (void)capacity;
    return NULL;
}

JNIEXPORT void* JNICALL JNI_GetDirectBufferAddress(JNIEnv* env, jobject buf) {
    (void)env; (void)buf;
    return NULL;
}

JNIEXPORT jlong JNICALL JNI_GetDirectBufferCapacity(JNIEnv* env, jobject buf) {
    (void)env; (void)buf;
    return 0;
}

/* JavaVM stubs */

JNIEXPORT jint JNICALL JNI_DestroyJavaVM(JavaVM* vm) {
    (void)vm;
    return JNI_ERR;
}

JNIEXPORT jint JNICALL JNI_AttachCurrentThread(JavaVM* vm, JNIEnv** penv, void* args) {
    (void)vm; (void)penv; (void)args;
    return JNI_ERR;
}

JNIEXPORT jint JNICALL JNI_DetachCurrentThread(JavaVM* vm) {
    (void)vm;
    return JNI_ERR;
}

JNIEXPORT jint JNICALL JNI_GetEnv(JavaVM* vm, void** penv, unsigned int version) {
    (void)vm; (void)penv; (void)version;
    return JNI_ERR;
}

JNIEXPORT jint JNICALL JNI_AttachCurrentThreadAsDaemon(JavaVM* vm, JNIEnv** penv, void* args) {
    (void)vm; (void)penv; (void)args;
    return JNI_ERR;
}