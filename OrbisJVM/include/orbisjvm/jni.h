/**
 * @file jni.h
 * @brief OrbisJVM JNI Compatibility Layer
 *
 * Provides JNI (Java Native Interface) compatibility for OrbisJVM.
 * This is a compatibility bridge - actual JNI implementation depends on the JVM port.
 */

#ifndef ORBISJVM_JNI_H
#define ORBISJVM_JNI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* JNI Version */
#define ORBISJVM_JNI_VERSION_1_1  0x00010001
#define ORBISJVM_JNI_VERSION_1_2  0x00010002
#define ORBISJVM_JNI_VERSION_1_4  0x00010004
#define ORBISJVM_JNI_VERSION_1_6  0x00010006
#define ORBISJVM_JNI_VERSION_1_7  0x00010007
#define ORBISJVM_JNI_VERSION_1_8  0x00010008
#define ORBISJVM_JNI_VERSION_9    0x00090000
#define ORBISJVM_JNI_VERSION_10   0x000a0000
#define ORBISJVM_JNI_VERSION_11   0x000b0000
#define ORBISJVM_JNI_VERSION_12   0x000c0000
#define ORBISJVM_JNI_VERSION_13   0x000d0000
#define ORBISJVM_JNI_VERSION_14   0x000e0000
#define ORBISJVM_JNI_VERSION_15   0x000f0000
#define ORBISJVM_JNI_VERSION_16   0x00100000
#define ORBISJVM_JNI_VERSION_17   0x00110000
#define ORBISJVM_JNI_VERSION_18   0x00120000
#define ORBISJVM_JNI_VERSION_19   0x00130000
#define ORBISJVM_JNI_VERSION_20   0x00140000
#define ORBISJVM_JNI_VERSION_21   0x00150000

/* JNI Types */
typedef int8_t     jbyte;
typedef uint8_t    jboolean;
typedef int16_t    jchar;
typedef int16_t    jshort;
typedef int32_t    jint;
typedef int64_t    jlong;
typedef float      jfloat;
typedef double     jdouble;
typedef int        jsize;

/* JNI Reference Types */
typedef struct _jobject*      jobject;
typedef struct _jclass*       jclass;
typedef struct _jstring*      jstring;
typedef struct _jarray*       jarray;
typedef struct _jobjectArray* jobjectArray;
typedef struct _jbooleanArray* jbooleanArray;
typedef struct _jbyteArray*   jbyteArray;
typedef struct _jcharArray*   jcharArray;
typedef struct _jshortArray*  jshortArray;
typedef struct _jintArray*    jintArray;
typedef struct _jlongArray*   jlongArray;
typedef struct _jfloatArray*  jfloatArray;
typedef struct _jdoubleArray* jdoubleArray;
typedef struct _jthrowable*   jthrowable;
typedef struct _jweak*        jweak;

/* JNI Method ID and Field ID */
typedef struct _jmethodID*    jmethodID;
typedef struct _jfieldID*     jfieldID;

/* JNI Interface Pointer */
struct JNINativeInterface_;
typedef const struct JNINativeInterface_* JNIEnv;

struct JavaVM_;
typedef struct JavaVM_ JavaVM;

/* JNI Function Table - simplified for compatibility layer */
typedef struct {
    unsigned int version;

    /* Version functions */
    jint (*GetVersion)(JNIEnv* env);

    /* Class operations */
    jclass (*FindClass)(JNIEnv* env, const char* name);
    jclass (*DefineClass)(JNIEnv* env, const char* name, jobject loader, const jbyte* buf, jsize len);
    jclass (*GetSuperclass)(JNIEnv* env, jclass clazz);
    jboolean (*IsAssignableFrom)(JNIEnv* env, jclass clazz1, jclass clazz2);
    jint (*Throw)(JNIEnv* env, jthrowable obj);
    jint (*ThrowNew)(JNIEnv* env, jclass clazz, const char* msg);
    jthrowable (*ExceptionOccurred)(JNIEnv* env);
    void (*ExceptionDescribe)(JNIEnv* env);
    void (*ExceptionClear)(JNIEnv* env);
    void (*FatalError)(JNIEnv* env, const char* msg);

    /* Global/Local references */
    jobject (*NewGlobalRef)(JNIEnv* env, jobject obj);
    void (*DeleteGlobalRef)(JNIEnv* env, jobject obj);
    jobject (*NewLocalRef)(JNIEnv* env, jobject obj);
    void (*DeleteLocalRef)(JNIEnv* env, jobject obj);
    jint (*EnsureLocalCapacity)(JNIEnv* env, jint capacity);

    /* Object operations */
    jobject (*AllocObject)(JNIEnv* env, jclass clazz);
    jobject (*NewObject)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jobject (*NewObjectV)(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args);
    jobject (*NewObjectA)(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args);
    jclass (*GetObjectClass)(JNIEnv* env, jobject obj);
    jboolean (*IsInstanceOf)(JNIEnv* env, jobject obj, jclass clazz);
    jboolean (*IsSameObject)(JNIEnv* env, jobject obj1, jobject obj2);

    /* Field access */
    jfieldID (*GetFieldID)(JNIEnv* env, jclass clazz, const char* name, const char* sig);
    jobject (*GetObjectField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jboolean (*GetBooleanField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jbyte (*GetByteField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jchar (*GetCharField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jshort (*GetShortField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jint (*GetIntField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jlong (*GetLongField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jfloat (*GetFloatField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    jdouble (*GetDoubleField)(JNIEnv* env, jobject obj, jfieldID fieldID);
    void (*SetObjectField)(JNIEnv* env, jobject obj, jfieldID fieldID, jobject val);
    void (*SetBooleanField)(JNIEnv* env, jobject obj, jfieldID fieldID, jboolean val);
    void (*SetByteField)(JNIEnv* env, jobject obj, jfieldID fieldID, jbyte val);
    void (*SetCharField)(JNIEnv* env, jobject obj, jfieldID fieldID, jchar val);
    void (*SetShortField)(JNIEnv* env, jobject obj, jfieldID fieldID, jshort val);
    void (*SetIntField)(JNIEnv* env, jobject obj, jfieldID fieldID, jint val);
    void (*SetLongField)(JNIEnv* env, jobject obj, jfieldID fieldID, jlong val);
    void (*SetFloatField)(JNIEnv* env, jobject obj, jfieldID fieldID, jfloat val);
    void (*SetDoubleField)(JNIEnv* env, jobject obj, jfieldID fieldID, jdouble val);

    /* Method calls */
    jmethodID (*GetMethodID)(JNIEnv* env, jclass clazz, const char* name, const char* sig);
    jobject (*CallObjectMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jobject (*CallObjectMethodV)(JNIEnv* env, jobject obj, jmethodID methodID, va_list args);
    jobject (*CallObjectMethodA)(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args);
    jboolean (*CallBooleanMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jbyte (*CallByteMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jchar (*CallCharMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jshort (*CallShortMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jint (*CallIntMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jlong (*CallLongMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jfloat (*CallFloatMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    jdouble (*CallDoubleMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);
    void (*CallVoidMethod)(JNIEnv* env, jobject obj, jmethodID methodID, ...);

    /* Static methods */
    jmethodID (*GetStaticMethodID)(JNIEnv* env, jclass clazz, const char* name, const char* sig);
    jobject (*CallStaticObjectMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jboolean (*CallStaticBooleanMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jbyte (*CallStaticByteMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jchar (*CallStaticCharMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jshort (*CallStaticShortMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jint (*CallStaticIntMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jlong (*CallStaticLongMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jfloat (*CallStaticFloatMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    jdouble (*CallStaticDoubleMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
    void (*CallStaticVoidMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);

    /* Static fields */
    jfieldID (*GetStaticFieldID)(JNIEnv* env, jclass clazz, const char* name, const char* sig);
    jobject (*GetStaticObjectField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jboolean (*GetStaticBooleanField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jbyte (*GetStaticByteField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jchar (*GetStaticCharField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jshort (*GetStaticShortField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jint (*GetStaticIntField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jlong (*GetStaticLongField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jfloat (*GetStaticFloatField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    jdouble (*GetStaticDoubleField)(JNIEnv* env, jclass clazz, jfieldID fieldID);
    void (*SetStaticObjectField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jobject val);
    void (*SetStaticBooleanField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jboolean val);
    void (*SetStaticByteField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jbyte val);
    void (*SetStaticCharField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jchar val);
    void (*SetStaticShortField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jshort val);
    void (*SetStaticIntField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jint val);
    void (*SetStaticLongField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jlong val);
    void (*SetStaticFloatField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jfloat val);
    void (*SetStaticDoubleField)(JNIEnv* env, jclass clazz, jfieldID fieldID, jdouble val);

    /* String operations */
    jstring (*NewString)(JNIEnv* env, const jchar* unicode, jsize len);
    jsize (*GetStringLength)(JNIEnv* env, jstring str);
    const jchar* (*GetStringChars)(JNIEnv* env, jstring str, jboolean* isCopy);
    void (*ReleaseStringChars)(JNIEnv* env, jstring str, const jchar* chars);
    jstring (*NewStringUTF)(JNIEnv* env, const char* utf);
    jsize (*GetStringUTFLength)(JNIEnv* env, jstring str);
    const char* (*GetStringUTFChars)(JNIEnv* env, jstring str, jboolean* isCopy);
    void (*ReleaseStringUTFChars)(JNIEnv* env, jstring str, const char* chars);

    /* Array operations */
    jsize (*GetArrayLength)(JNIEnv* env, jarray array);
    jobjectArray (*NewObjectArray)(JNIEnv* env, jsize len, jclass clazz, jobject init);
    jobject (*GetObjectArrayElement)(JNIEnv* env, jobjectArray array, jsize index);
    void (*SetObjectArrayElement)(JNIEnv* env, jobjectArray array, jsize index, jobject val);

    /* Primitive arrays */
    jbooleanArray (*NewBooleanArray)(JNIEnv* env, jsize len);
    jbyteArray (*NewByteArray)(JNIEnv* env, jsize len);
    jcharArray (*NewCharArray)(JNIEnv* env, jsize len);
    jshortArray (*NewShortArray)(JNIEnv* env, jsize len);
    jintArray (*NewIntArray)(JNIEnv* env, jsize len);
    jlongArray (*NewLongArray)(JNIEnv* env, jsize len);
    jfloatArray (*NewFloatArray)(JNIEnv* env, jsize len);
    jdoubleArray (*NewDoubleArray)(JNIEnv* env, jsize len);

    jboolean* (*GetBooleanArrayElements)(JNIEnv* env, jbooleanArray array, jboolean* isCopy);
    jbyte* (*GetByteArrayElements)(JNIEnv* env, jbyteArray array, jboolean* isCopy);
    jchar* (*GetCharArrayElements)(JNIEnv* env, jcharArray array, jboolean* isCopy);
    jshort* (*GetShortArrayElements)(JNIEnv* env, jshortArray array, jboolean* isCopy);
    jint* (*GetIntArrayElements)(JNIEnv* env, jintArray array, jboolean* isCopy);
    jlong* (*GetLongArrayElements)(JNIEnv* env, jlongArray array, jboolean* isCopy);
    jfloat* (*GetFloatArrayElements)(JNIEnv* env, jfloatArray array, jboolean* isCopy);
    jdouble* (*GetDoubleArrayElements)(JNIEnv* env, jdoubleArray array, jboolean* isCopy);

    void (*ReleaseBooleanArrayElements)(JNIEnv* env, jbooleanArray array, jboolean* elems, jint mode);
    void (*ReleaseByteArrayElements)(JNIEnv* env, jbyteArray array, jbyte* elems, jint mode);
    void (*ReleaseCharArrayElements)(JNIEnv* env, jcharArray array, jchar* elems, jint mode);
    void (*ReleaseShortArrayElements)(JNIEnv* env, jshortArray array, jshort* elems, jint mode);
    void (*ReleaseIntArrayElements)(JNIEnv* env, jintArray array, jint* elems, jint mode);
    void (*ReleaseLongArrayElements)(JNIEnv* env, jlongArray array, jlong* elems, jint mode);
    void (*ReleaseFloatArrayElements)(JNIEnv* env, jfloatArray array, jfloat* elems, jint mode);
    void (*ReleaseDoubleArrayElements)(JNIEnv* env, jdoubleArray array, jdouble* elems, jint mode);

    void (*GetBooleanArrayRegion)(JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean* buf);
    void (*GetByteArrayRegion)(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte* buf);
    void (*GetCharArrayRegion)(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar* buf);
    void (*GetShortArrayRegion)(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort* buf);
    void (*GetIntArrayRegion)(JNIEnv* env, jintArray array, jsize start, jsize len, jint* buf);
    void (*GetLongArrayRegion)(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong* buf);
    void (*GetFloatArrayRegion)(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat* buf);
    void (*GetDoubleArrayRegion)(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble* buf);

    void (*SetBooleanArrayRegion)(JNIEnv* env, jbooleanArray array, jsize start, jsize len, const jboolean* buf);
    void (*SetByteArrayRegion)(JNIEnv* env, jbyteArray array, jsize start, jsize len, const jbyte* buf);
    void (*SetCharArrayRegion)(JNIEnv* env, jcharArray array, jsize start, jsize len, const jchar* buf);
    void (*SetShortArrayRegion)(JNIEnv* env, jshortArray array, jsize start, jsize len, const jshort* buf);
    void (*SetIntArrayRegion)(JNIEnv* env, jintArray array, jsize start, jsize len, const jint* buf);
    void (*SetLongArrayRegion)(JNIEnv* env, jlongArray array, jsize start, jsize len, const jlong* buf);
    void (*SetFloatArrayRegion)(JNIEnv* env, jfloatArray array, jsize start, jsize len, const jfloat* buf);
    void (*SetDoubleArrayRegion)(JNIEnv* env, jdoubleArray array, jsize start, jsize len, const jdouble* buf);

    /* Critical arrays */
    void* (*GetPrimitiveArrayCritical)(JNIEnv* env, jarray array, jboolean* isCopy);
    void (*ReleasePrimitiveArrayCritical)(JNIEnv* env, jarray array, void* carray, jint mode);

    /* JavaVM operations */
    jint (*GetJavaVM)(JNIEnv* env, JavaVM** vm);

    /* Reflection */
    jobject (*FromReflectedMethod)(JNIEnv* env, jobject method);
    jobject (*FromReflectedField)(JNIEnv* env, jobject field);
    jobject (*ToReflectedMethod)(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic);
    jobject (*ToReflectedField)(JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic);

    /* Weak references */
    jweak (*NewWeakGlobalRef)(JNIEnv* env, jobject obj);
    void (*DeleteWeakGlobalRef)(JNIEnv* env, jweak ref);

    /* JNI 1.6+ */
    jboolean (*ExceptionCheck)(JNIEnv* env);

    /* Direct buffers */
    void* (*GetDirectBufferAddress)(JNIEnv* env, jobject buf);
    jlong (*GetDirectBufferCapacity)(JNIEnv* env, jobject buf);

    /* JNI 1.8+ */
    jlong (*GetObjectRefType)(JNIEnv* env, jobject obj);

} JNINativeInterface;

/* JavaVM interface */
struct JNIInvokeInterface_ {
    unsigned int version;
    jint (*DestroyJavaVM)(JavaVM* vm);
    jint (*AttachCurrentThread)(JavaVM* vm, JNIEnv** penv, void* args);
    jint (*DetachCurrentThread)(JavaVM* vm);
    jint (*GetEnv)(JavaVM* vm, void** penv, unsigned int version);
    jint (*AttachCurrentThreadAsDaemon)(JavaVM* vm, JNIEnv** penv, void* args);
};

/* JNI Return values */
#define JNI_OK           0
#define JNI_ERR         -1
#define JNI_EDETACHED   -2
#define JNI_EVERSION    -3
#define JNI_ENOMEM      -4
#define JNI_EEXIST      -5
#define JNI_EINVAL      -6

/* JNI Boolean values */
#define JNI_TRUE  1
#define JNI_FALSE 0

/* JNI Reference release modes */
#define JNI_COMMIT  1
#define JNI_ABORT   2

/* JNI Class modifiers */
#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_VOLATILE     0x0040
#define ACC_TRANSIENT    0x0080
#define ACC_NATIVE       0x0100
#define ACC_INTERFACE    0x0200
#define ACC_ABSTRACT     0x0400
#define ACC_STRICT       0x0800

/* JNI Function table access */
#define JNIEnv (*(const JNINativeInterface_**)env)

/* JNI function macros */
#define GetVersion(env)                 (*env)->GetVersion(env)
#define FindClass(env, name)            (*env)->FindClass(env, name)
#define DefineClass(env, name, loader, buf, len) (*env)->DefineClass(env, name, loader, buf, len)
#define GetSuperclass(env, clazz)       (*env)->GetSuperclass(env, clazz)
#define IsAssignableFrom(env, c1, c2)   (*env)->IsAssignableFrom(env, c1, c2)
#define Throw(env, obj)                 (*env)->Throw(env, obj)
#define ThrowNew(env, clazz, msg)       (*env)->ThrowNew(env, clazz, msg)
#define ExceptionOccurred(env)          (*env)->ExceptionOccurred(env)
#define ExceptionDescribe(env)          (*env)->ExceptionDescribe(env)
#define ExceptionClear(env)             (*env)->ExceptionClear(env)
#define FatalError(env, msg)            (*env)->FatalError(env, msg)
#define NewGlobalRef(env, obj)          (*env)->NewGlobalRef(env, obj)
#define DeleteGlobalRef(env, obj)       (*env)->DeleteGlobalRef(env, obj)
#define NewLocalRef(env, obj)           (*env)->NewLocalRef(env, obj)
#define DeleteLocalRef(env, obj)        (*env)->DeleteLocalRef(env, obj)
#define EnsureLocalCapacity(env, cap)   (*env)->EnsureLocalCapacity(env, cap)
#define AllocObject(env, clazz)         (*env)->AllocObject(env, clazz)
#define NewObject(env, clazz, methodID, ...) (*env)->NewObject(env, clazz, methodID, __VA_ARGS__)
#define GetObjectClass(env, obj)        (*env)->GetObjectClass(env, obj)
#define IsInstanceOf(env, obj, clazz)   (*env)->IsInstanceOf(env, obj, clazz)
#define IsSameObject(env, o1, o2)       (*env)->IsSameObject(env, o1, o2)
#define GetFieldID(env, clazz, name, sig) (*env)->GetFieldID(env, clazz, name, sig)
#define GetObjectField(env, obj, fid)   (*env)->GetObjectField(env, obj, fid)
#define GetIntField(env, obj, fid)      (*env)->GetIntField(env, obj, fid)
#define SetObjectField(env, obj, fid, val) (*env)->SetObjectField(env, obj, fid, val)
#define SetIntField(env, obj, fid, val) (*env)->SetIntField(env, obj, fid, val)
#define GetMethodID(env, clazz, name, sig) (*env)->GetMethodID(env, clazz, name, sig)
#define CallObjectMethod(env, obj, mid, ...) (*env)->CallObjectMethod(env, obj, mid, __VA_ARGS__)
#define CallIntMethod(env, obj, mid, ...) (*env)->CallIntMethod(env, obj, mid, __VA_ARGS__)
#define CallVoidMethod(env, obj, mid, ...) (*env)->CallVoidMethod(env, obj, mid, __VA_ARGS__)
#define GetStaticMethodID(env, clazz, name, sig) (*env)->GetStaticMethodID(env, clazz, name, sig)
#define CallStaticObjectMethod(env, clazz, mid, ...) (*env)->CallStaticObjectMethod(env, clazz, mid, __VA_ARGS__)
#define CallStaticIntMethod(env, clazz, mid, ...) (*env)->CallStaticIntMethod(env, clazz, mid, __VA_ARGS__)
#define CallStaticVoidMethod(env, clazz, mid, ...) (*env)->CallStaticVoidMethod(env, clazz, mid, __VA_ARGS__)
#define GetStaticFieldID(env, clazz, name, sig) (*env)->GetStaticFieldID(env, clazz, name, sig)
#define GetStaticIntField(env, clazz, fid) (*env)->GetStaticIntField(env, clazz, fid)
#define SetStaticIntField(env, clazz, fid, val) (*env)->SetStaticIntField(env, clazz, fid, val)
#define NewStringUTF(env, utf)          (*env)->NewStringUTF(env, utf)
#define GetStringUTFChars(env, str, isCopy) (*env)->GetStringUTFChars(env, str, isCopy)
#define ReleaseStringUTFChars(env, str, chars) (*env)->ReleaseStringUTFChars(env, str, chars)
#define GetArrayLength(env, arr)        (*env)->GetArrayLength(env, arr)
#define NewIntArray(env, len)           (*env)->NewIntArray(env, len)
#define GetIntArrayElements(env, arr, isCopy) (*env)->GetIntArrayElements(env, arr, isCopy)
#define ReleaseIntArrayElements(env, arr, elems, mode) (*env)->ReleaseIntArrayElements(env, arr, elems, mode)
#define GetIntArrayRegion(env, arr, start, len, buf) (*env)->GetIntArrayRegion(env, arr, start, len, buf)
#define SetIntArrayRegion(env, arr, start, len, buf) (*env)->SetIntArrayRegion(env, arr, start, len, buf)
#define GetJavaVM(env, vm)              (*env)->GetJavaVM(env, vm)
#define ExceptionCheck(env)             (*env)->ExceptionCheck(env)
#define NewDirectByteBuffer(env, addr, cap) (*env)->NewDirectByteBuffer(env, addr, cap)
#define GetDirectBufferAddress(env, buf) (*env)->GetDirectBufferAddress(env, buf)
#define GetDirectBufferCapacity(env, buf) (*env)->GetDirectBufferCapacity(env, buf)

/* OrbisJVM-specific JNI extensions */
typedef struct {
    /* Platform-specific JVM operations */
    jint (*orbisjvm_create_jvm)(JavaVM** pvm, void* penv, void* args);
    jint (*orbisjvm_destroy_jvm)(JavaVM* vm);
    jint (*orbisjvm_attach_thread)(JavaVM* vm, JNIEnv** penv, void* args);
    jint (*orbisjvm_detach_thread)(JavaVM* vm);

    /* Memory management */
    void* (*orbisjvm_alloc_jvm_memory)(size_t size, int flags);
    void (*orbisjvm_free_jvm_memory)(void* ptr, size_t size);

    /* Thread management */
    jint (*orbisjvm_create_java_thread)(JavaVM* vm, void* (*start)(void*), void* arg, void** thread_handle);
    void (*orbisjvm_join_java_thread)(void* thread_handle);

    /* Class loading */
    jclass (*orbisjvm_find_class_from_path)(JNIEnv* env, const char* path);
    jint (*orbisjvm_define_class_from_bytes)(JNIEnv* env, const char* name, const jbyte* bytes, jsize len);

    /* Diagnostics */
    void (*orbisjvm_dump_heap)(JNIEnv* env, const char* path);
    void (*orbisjvm_gc)(JNIEnv* env);

} OrbisJVM_JNI_Extensions;

/* Get OrbisJVM JNI extensions */
const OrbisJVM_JNI_Extensions* orbisjvm_get_jni_extensions(void);

/* Initialize JNI compatibility layer */
int orbisjvm_jni_init(void);

/* Shutdown JNI compatibility layer */
void orbisjvm_jni_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_JNI_H