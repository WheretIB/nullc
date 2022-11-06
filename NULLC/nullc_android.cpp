#include <jni.h>

#include <android/log.h>

#include <assert.h>

#include "nullc.h"
#include "nullc_debug.h"
#include "nullc_internal.h"

#ifdef BUILD_TESTS
#include "../tests/UnitTests.h"
#endif

namespace NULLC
{
	extern const char*	nullcLastError;
}

namespace
{
	JNIEnv *env = 0;

	jclass classBool, classChar, classShort, classInt, classLong, classFloat, classDouble;
	jclass classNullcArray, classNullcRef, classNullcFuncPtr, classNullcAutoArray;

	jclass classFileHandler = 0;
	jobject fileHandler = 0;
}

const char* nullcAndroidFileLoadFunc(const char* name, unsigned int* size)
{
	//__android_log_print(ANDROID_LOG_INFO, "nullc", "nullcAndroidfileLoadFunc %s\n", name);

	assert(name);
	assert(size);

	if(!env || !fileHandler)
	{
		*size = 0;
		return NULL;
	}

	jmethodID mid = env->GetMethodID(classFileHandler, "Read", "(Ljava/lang/String;)[B");

	jstring javaName = env->NewStringUTF(name);
	jbyteArray arr = (jbyteArray)env->CallObjectMethod(fileHandler, mid, javaName);
	env->DeleteLocalRef(javaName);

    if(!arr)
    {
        *size = 0;
        return NULL;
    }

	jsize arrSize = env->GetArrayLength(arr);
	jbyte *arrElements = env->GetByteArrayElements(arr, NULL);

	char *contents = (char*)NULLC::alloc(arrSize + 1);
	memcpy(contents, arrElements, arrSize);
	contents[arrSize] = 0;

	env->ReleaseByteArrayElements(arr, arrElements, 0);

	*size = arrSize;
	return contents;
}

void nullcAndroidFileFreeFunc(const char* data)
{
	if(!data)
		return;

	NULLC::dealloc((void*)data);
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcInit(JNIEnv *env, jclass cls)
{
	::env = env;

	nullcInit();

	nullcSetFileReadHandler(nullcAndroidFileLoadFunc, nullcAndroidFileFreeFunc);

	classBool = env->FindClass("java/lang/Boolean");
	classBool = (jclass)env->NewGlobalRef(classBool);

	classChar = env->FindClass("java/lang/Character");
	classChar = (jclass)env->NewGlobalRef(classChar);

	classShort = env->FindClass("java/lang/Short");
	classShort = (jclass)env->NewGlobalRef(classShort);

	classInt = env->FindClass("java/lang/Integer");
	classInt = (jclass)env->NewGlobalRef(classInt);

	classLong = env->FindClass("java/lang/Long");
	classLong = (jclass)env->NewGlobalRef(classLong);

	classFloat = env->FindClass("java/lang/Float");
	classFloat = (jclass)env->NewGlobalRef(classFloat);

	classDouble = env->FindClass("java/lang/Double");
	classDouble = (jclass)env->NewGlobalRef(classDouble);

	classNullcArray = env->FindClass("org/nullc/Nullc$NULLCArray");
	classNullcArray = (jclass)env->NewGlobalRef(classNullcArray);

	classNullcRef = env->FindClass("org/nullc/Nullc$NULLCRef");
	classNullcRef = (jclass)env->NewGlobalRef(classNullcRef);

	classNullcFuncPtr = env->FindClass("org/nullc/Nullc$NULLCFuncPtr");
	classNullcFuncPtr = (jclass)env->NewGlobalRef(classNullcFuncPtr);

	classNullcAutoArray = env->FindClass("org/nullc/Nullc$NULLCAutoArray");
	classNullcAutoArray = (jclass)env->NewGlobalRef(classNullcAutoArray);

	return 1;
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcClearImportPath(JNIEnv *env, jclass cls)
{
	nullcClearImportPaths();
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcAddImportPath(JNIEnv *env, jclass cls, jstring importPath)
{
	const char *nativeString = env->GetStringUTFChars(importPath, 0);

	nullcAddImportPath(nativeString);

	env->ReleaseStringUTFChars(importPath, nativeString);
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcSetFileReadHandler(JNIEnv *env, jclass cls, jobject handler)
{
	::env = env;

	if(classFileHandler)
		env->DeleteGlobalRef(classFileHandler);
	classFileHandler = 0;

	if(handler)
	{
		classFileHandler = env->FindClass("org/nullc/Nullc$FileReadHandler");
		classFileHandler = (jclass)env->NewGlobalRef(classFileHandler);

		//__android_log_print(ANDROID_LOG_INFO, "nullc", "nullcSetFileReadHandler 1 %p\n", classFileHandler);
	}

	if(fileHandler)
		env->DeleteGlobalRef(fileHandler);

	fileHandler = handler;

	if(handler)
	{
		fileHandler = env->NewGlobalRef(fileHandler);

		//__android_log_print(ANDROID_LOG_INFO, "nullc", "nullcSetFileReadHandler 2 %p\n", fileHandler);
	}
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcSetGlobalMemoryLimit(JNIEnv *env, jclass cls, jint limit)
{
	nullcSetGlobalMemoryLimit(limit);
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcSetEnableLogFiles(JNIEnv *env, jclass cls, jint enable)
{
	nullcSetEnableLogFiles(enable, 0, 0, 0);
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcSetOptimizationLevel(JNIEnv *env, jclass cls, jint level)
{
	nullcSetOptimizationLevel(level);
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcTerminate(JNIEnv *env, jclass cls)
{
	env->DeleteGlobalRef(classBool);
	classBool = 0;
	env->DeleteGlobalRef(classChar);
	classChar = 0;
	env->DeleteGlobalRef(classShort);
	classShort = 0;
	env->DeleteGlobalRef(classInt);
	classInt = 0;
	env->DeleteGlobalRef(classLong);
	classLong = 0;
	env->DeleteGlobalRef(classFloat);
	classFloat = 0;
	env->DeleteGlobalRef(classDouble);
	classDouble = 0;
	env->DeleteGlobalRef(classNullcArray);
	classNullcArray = 0;
	env->DeleteGlobalRef(classNullcRef);
	classNullcRef = 0;
	env->DeleteGlobalRef(classNullcFuncPtr);
	classNullcFuncPtr = 0;
	env->DeleteGlobalRef(classNullcAutoArray);
	classNullcAutoArray = 0;

	nullcTerminate();
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcSetExecutor(JNIEnv *env, jclass cls, jobject id)
{
	// TODO
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcLoadModuleBySource(JNIEnv *env, jclass cls, jstring module, jstring code)
{
	const char *moduleStr = env->GetStringUTFChars(module, 0);
	const char *codeStr = env->GetStringUTFChars(code, 0);

	nullres res = nullcLoadModuleBySource(moduleStr, codeStr);

	env->ReleaseStringUTFChars(module, moduleStr);
	env->ReleaseStringUTFChars(code, codeStr);

	return !!res;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcLoadModuleByBinary(JNIEnv *env, jclass cls, jstring module, jbyteArray code)
{
	const char *moduleStr = env->GetStringUTFChars(module, 0);
	jbyte *codeArr = env->GetByteArrayElements(code, 0);

	nullres res = nullcLoadModuleByBinary(moduleStr, (const char*)codeArr);

	env->ReleaseStringUTFChars(module, moduleStr);
	env->ReleaseByteArrayElements(code, codeArr, JNI_ABORT);

	return !!res;
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcRemoveModule(JNIEnv *env, jclass cls, jstring module)
{
	const char *moduleStr = env->GetStringUTFChars(module, 0);

	nullcRemoveModule(moduleStr);

	env->ReleaseStringUTFChars(module, moduleStr);
}

JNIEXPORT jstring JNICALL Java_org_nullc_Nullc_nullcEnumerateModules(JNIEnv *env, jclass cls, jint id)
{
	return env->NewStringUTF(nullcEnumerateModules(id));
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcBuild(JNIEnv *env, jclass cls, jstring code)
{
	const char *codeStr = env->GetStringUTFChars(code, 0);

	nullres res = nullcBuild(codeStr);

	env->ReleaseStringUTFChars(code, codeStr);

	return !!res;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcRun(JNIEnv *env, jclass cls)
{
	return !!nullcRun();
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcRunFunction(JNIEnv *env, jclass cls, jstring funcName, jobjectArray arguments)
{
	if(!funcName)
		return !!nullcRun();

	const char *funcNameStr = env->GetStringUTFChars(funcName, 0);

	NULLCFuncPtr func;
	if(!nullcGetFunction(funcNameStr, &func))
	{
		env->ReleaseStringUTFChars(funcName, funcNameStr);
		return false;
	}

	unsigned functionCount = 0;
	ExternFuncInfo *functions = nullcDebugFunctionInfo(&functionCount);

	unsigned typeCount = 0;
	ExternTypeInfo *types = nullcDebugTypeInfo(&typeCount);

	unsigned localCount = 0;
	ExternLocalInfo *locals = nullcDebugLocalInfo(&localCount);

	ExternFuncInfo &function = functions[func.id];

	char *argBuf = (char*)NULLC::alloc(function.argumentSize + sizeof(void*));

	jsize argCount = env->GetArrayLength(arguments);

	static char errorText[512];

	if(argCount != function.paramCount)
	{
		NULLC::SafeSprintf(errorText, 512, "ERROR: function '%s' expects %d arguments, but %d were supplied", funcNameStr, function.paramCount, argCount);
		NULLC::nullcLastError = errorText;
		return false;
	}

	env->ReleaseStringUTFChars(funcName, funcNameStr);

	char *argPos = argBuf;
	for(unsigned int i = 0; i < function.paramCount; i++)
	{
		ExternLocalInfo &lInfo = locals[function.offsetToFirstLocal + i];
		ExternTypeInfo &tInfo = types[lInfo.type];

		jobject obj = env->GetObjectArrayElement(arguments, i);
		jclass objClass = env->GetObjectClass(obj);

		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_CHAR:
			if(objClass == classBool)
			{
				*(int*)argPos = (int)env->CallBooleanMethod(obj, env->GetMethodID(classBool, "booleanValue", "()Z"));
			}else if(objClass == classChar){
				*(int*)argPos = (int)env->CallCharMethod(obj, env->GetMethodID(classChar, "charValue", "()C"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_SHORT:
			if(objClass == classShort)
			{
				*(int*)argPos = (int)env->CallShortMethod(obj, env->GetMethodID(classShort, "shortValue", "()S"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_INT:
			if(objClass == classInt)
			{
				*(int*)argPos = env->CallIntMethod(obj, env->GetMethodID(classInt, "intValue", "()I"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_LONG:
			if(objClass == classLong)
			{
				*(long long*)argPos = env->CallLongMethod(obj, env->GetMethodID(classLong, "longValue", "()J"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 8;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			if(objClass == classFloat)
			{
				*(float*)argPos = env->CallFloatMethod(obj, env->GetMethodID(classFloat, "floatValue", "()F"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			if(objClass == classDouble)
			{
				*(double*)argPos = env->CallDoubleMethod(obj, env->GetMethodID(classDouble, "doubleValue", "()D"));
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			argPos += 8;
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
			if(objClass == classNullcArray)
			{
				jfieldID ptrField = env->GetFieldID(classNullcArray, "ptr", "J");
				jfieldID lengthField = env->GetFieldID(classNullcArray, "length", "I");

				*(int*)argPos = (int)env->GetLongField(obj, ptrField);
				argPos += 4;

				*(int*)argPos = env->GetIntField(obj, lengthField);
				argPos += 4;
			}else if(objClass == classNullcRef){
				jfieldID typeIdField = env->GetFieldID(classNullcArray, "typeID", "I");
				jfieldID ptrField = env->GetFieldID(classNullcArray, "ptr", "J");

				*(int*)argPos = env->GetIntField(obj, typeIdField);
				argPos += 4;

				*(int*)argPos = (int)env->GetLongField(obj, ptrField);
				argPos += 4;
			}else if(objClass == classNullcFuncPtr){
				jfieldID contextField = env->GetFieldID(classNullcArray, "context", "J");
				jfieldID idField = env->GetFieldID(classNullcArray, "id", "I");

				*(int*)argPos = (int)env->GetLongField(obj, contextField);
				argPos += 4;

				*(int*)argPos = env->GetIntField(obj, idField);
				argPos += 4;
			}else if(objClass == classNullcAutoArray){
				jfieldID typeIdField = env->GetFieldID(classNullcArray, "typeID", "I");
				jfieldID ptrField = env->GetFieldID(classNullcArray, "ptr", "J");
				jfieldID lengthField = env->GetFieldID(classNullcArray, "length", "I");

				*(int*)argPos = env->GetIntField(obj, typeIdField);
				argPos += 4;

				*(int*)argPos = (int)env->GetLongField(obj, ptrField);
				argPos += 4;

				*(int*)argPos = env->GetIntField(obj, lengthField);
				argPos += 4;
			}else{
				NULLC::SafeSprintf(errorText, 512, "ERROR: function argument %d is of type %s", i, nullcDebugSource() + tInfo.offsetToName);
				NULLC::nullcLastError = errorText;
				return false;
			}
			break;
		case ExternTypeInfo::TYPE_VOID:
			break;
		}
	}
	*(uintptr_t*)argPos = 0;
	argPos += sizeof(uintptr_t);

	nullres res = nullcRunFunctionInternal(func.id, argBuf);

	NULLC::dealloc(argBuf);

	return !!res;
}

JNIEXPORT jstring JNICALL Java_org_nullc_Nullc_nullcGetResult(JNIEnv *env, jclass cls)
{
	return env->NewStringUTF(nullcGetResult());
}

JNIEXPORT jint JNICALL Java_org_nullc_Nullc_nullcGetResultInt(JNIEnv *env, jclass cls)
{
	return nullcGetResultInt();
}

JNIEXPORT jdouble JNICALL Java_org_nullc_Nullc_nullcGetResultDouble(JNIEnv *env, jclass cls)
{
	return nullcGetResultDouble();
}

JNIEXPORT jlong JNICALL Java_org_nullc_Nullc_nullcGetResultLong(JNIEnv *env, jclass cls)
{
	return nullcGetResultLong();
}

JNIEXPORT jstring JNICALL Java_org_nullc_Nullc_nullcGetLastError(JNIEnv *env, jclass cls)
{
	return env->NewStringUTF(nullcGetLastError());
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcFinalize(JNIEnv *env, jclass cls)
{
	return !!nullcFinalize();
}

JNIEXPORT jlong JNICALL Java_org_nullc_Nullc_nullcAllocate(JNIEnv *env, jclass cls, jint size)
{
	return (intptr_t)nullcAllocate(size);
}

JNIEXPORT jlong JNICALL Java_org_nullc_Nullc_nullcAllocateTyped(JNIEnv *env, jclass cls, jint typeID)
{
	return (intptr_t)nullcAllocateTyped(typeID);
}

JNIEXPORT jobject JNICALL Java_org_nullc_Nullc_nullcAllocateArrayTyped(JNIEnv *env, jclass cls, jint typeID, jint size)
{
	jobject array = env->AllocObject(classNullcArray);

	jfieldID ptrField = env->GetFieldID(classNullcArray, "ptr", "J");
	jfieldID lengthField = env->GetFieldID(classNullcArray, "length", "I");

	NULLCArray arr = nullcAllocateArrayTyped(typeID, size);

	env->SetLongField(array, ptrField, (long long)arr.ptr);
	env->SetIntField(array, lengthField, arr.len);

	return array;
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcThrowError(JNIEnv *env, jclass cls, jstring error)
{
	const char *errorStr = env->GetStringUTFChars(error, 0);

	nullcThrowError("%s", errorStr);

	env->ReleaseStringUTFChars(error, errorStr);
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcCallFunction(JNIEnv *env, jclass cls, jobject ptr, jobjectArray arguments)
{
	// TODO
	return false;
}

JNIEXPORT jobject JNICALL Java_org_nullc_Nullc_nullcGetGlobal(JNIEnv *env, jclass cls, jstring name)
{
	// TODO
	return 0;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcSetGlobal(JNIEnv *env, jclass cls, jstring name, jobject data)
{
	// TODO
	return false;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcGetFunction(JNIEnv *env, jclass cls, jstring name, jobject func)
{
	// TODO
	return false;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcSetFunction(JNIEnv *env, jclass cls, jstring name, jobject func)
{
	// TODO
	return false;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcIsStackPointer(JNIEnv *env, jclass cls, jlong ptr)
{
	return !!nullcIsStackPointer((void*)(intptr_t)ptr);
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcIsManagedPointer(JNIEnv *env, jclass cls, jlong ptr)
{
	return !!nullcIsManagedPointer((void*)(intptr_t)ptr);
}

JNIEXPORT jint JNICALL Java_org_nullc_Nullc_nullcInitTypeinfoModule(JNIEnv *env, jclass cls)
{
	return nullcInitTypeinfoModule();
}

JNIEXPORT jint JNICALL Java_org_nullc_Nullc_nullcInitDynamicModule(JNIEnv *env, jclass cls)
{
	return nullcInitDynamicModule();
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcAnalyze(JNIEnv *env, jclass cls, jstring code)
{
	const char *codeStr = env->GetStringUTFChars(code, 0);

	nullres res = nullcAnalyze(codeStr);

	env->ReleaseStringUTFChars(code, codeStr);

	return !!res;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcCompile(JNIEnv *env, jclass cls, jstring code)
{
	const char *codeStr = env->GetStringUTFChars(code, 0);

	nullres res = nullcCompile(codeStr);

	env->ReleaseStringUTFChars(code, codeStr);

	return !!res;
}

JNIEXPORT jbyteArray JNICALL Java_org_nullc_Nullc_nullcGetBytecode(JNIEnv *env, jclass cls)
{
	char *bytecode = 0;
	unsigned size = nullcGetBytecode(&bytecode);

	if(!bytecode)
		return 0;

	jbyteArray arr = env->NewByteArray(size);
	jbyte *arrElements = env->GetByteArrayElements(arr, NULL);

	memcpy(arrElements, bytecode, size);
	delete[] bytecode;

	env->ReleaseByteArrayElements(arr, arrElements, 0);

	return arr;
}

JNIEXPORT jbyteArray JNICALL Java_org_nullc_Nullc_nullcGetBytecodeNoCache(JNIEnv *env, jclass cls)
{
	char *bytecode = 0;
	unsigned size = nullcGetBytecodeNoCache(&bytecode);

	if(!bytecode)
		return 0;

	jbyteArray arr = env->NewByteArray(size);
	jbyte *arrElements = env->GetByteArrayElements(arr, NULL);

	memcpy(arrElements, bytecode, size);
	delete[] bytecode;

	env->ReleaseByteArrayElements(arr, arrElements, 0);

	return arr;
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcSaveListing(JNIEnv *env, jclass cls, jstring fileName)
{
	const char *fileNameStr = env->GetStringUTFChars(fileName, 0);

	nullres res = nullcSaveListing(fileNameStr);

	env->ReleaseStringUTFChars(fileName, fileNameStr);

	return !!res;
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcTranslateToC(JNIEnv *env, jclass cls, jstring fileName, jstring mainName)
{
	const char *fileNameStr = env->GetStringUTFChars(fileName, 0);
	const char *mainNameStr = env->GetStringUTFChars(mainName, 0);

	nullcTranslateToC(fileNameStr, mainNameStr, 0);

	env->ReleaseStringUTFChars(fileName, fileNameStr);
	env->ReleaseStringUTFChars(mainName, mainNameStr);
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcClean(JNIEnv *env, jclass cls)
{
	nullcClean();
}

JNIEXPORT jboolean JNICALL Java_org_nullc_Nullc_nullcLinkCode(JNIEnv *env, jclass cls, jbyteArray arr)
{
	jsize size = env->GetArrayLength(arr);
	jbyte *arrElements = env->GetByteArrayElements(arr, NULL);

	char *bytecode = (char*)NULLC::alloc(size);
	memcpy(bytecode, arrElements, size);

	env->ReleaseByteArrayElements(arr, arrElements, 0);

	nullres res = nullcLinkCode(bytecode);

	NULLC::dealloc(bytecode);

	return !!res;
}

JNIEXPORT void JNICALL Java_org_nullc_Nullc_nullcRunTests(JNIEnv *env, jclass cls, jboolean verbose)
{
#ifdef BUILD_TESTS
	RunTests(verbose, nullcAndroidFileLoadFunc, nullcAndroidFileFreeFunc, false, false, false, false, false);
#endif
}

}
