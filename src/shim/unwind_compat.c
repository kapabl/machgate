#define _GNU_SOURCE

#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <unwind.h>

static pthread_once_t unwind_library_once = PTHREAD_ONCE_INIT;
static void* unwind_library;

static void open_unwind_library(void)
{
	unwind_library = dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_LOCAL);
}

static void* resolve_unwind_operation(const char* name, const void* adapter)
{
	pthread_once(&unwind_library_once, open_unwind_library);

	void* operation = NULL;
	if (unwind_library)
		operation = dlsym(unwind_library, name);
	if (!operation || operation == adapter)
		operation = dlsym(RTLD_NEXT, name);
	if (operation == adapter)
		return NULL;
	return operation;
}

_Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn callback, void* context)
{
	typedef _Unwind_Reason_Code (*backtrace_operation)(
		_Unwind_Trace_Fn, void*);
	backtrace_operation operation = (backtrace_operation)
		resolve_unwind_operation("_Unwind_Backtrace", _Unwind_Backtrace);
	if (!operation)
		return _URC_FATAL_PHASE1_ERROR;
	return operation(callback, context);
}

void _Unwind_DeleteException(struct _Unwind_Exception* exception)
{
	typedef void (*delete_operation)(struct _Unwind_Exception*);
	delete_operation operation = (delete_operation)resolve_unwind_operation(
		"_Unwind_DeleteException", _Unwind_DeleteException);
	if (operation)
		operation(exception);
}

_Unwind_Word _Unwind_GetCFA(struct _Unwind_Context* context)
{
	typedef _Unwind_Word (*get_cfa_operation)(struct _Unwind_Context*);
	get_cfa_operation operation = (get_cfa_operation)resolve_unwind_operation(
		"_Unwind_GetCFA", _Unwind_GetCFA);
	return operation ? operation(context) : 0;
}

_Unwind_Ptr _Unwind_GetDataRelBase(struct _Unwind_Context* context)
{
	typedef _Unwind_Ptr (*get_pointer_operation)(struct _Unwind_Context*);
	get_pointer_operation operation = (get_pointer_operation)
		resolve_unwind_operation("_Unwind_GetDataRelBase",
		                         _Unwind_GetDataRelBase);
	return operation ? operation(context) : 0;
}

_Unwind_Ptr _Unwind_GetIP(struct _Unwind_Context* context)
{
	typedef _Unwind_Ptr (*get_pointer_operation)(struct _Unwind_Context*);
	get_pointer_operation operation = (get_pointer_operation)
		resolve_unwind_operation("_Unwind_GetIP", _Unwind_GetIP);
	return operation ? operation(context) : 0;
}

_Unwind_Ptr _Unwind_GetIPInfo(struct _Unwind_Context* context,
	                      int* before_instruction)
{
	typedef _Unwind_Ptr (*get_ip_info_operation)(struct _Unwind_Context*, int*);
	get_ip_info_operation operation = (get_ip_info_operation)
		resolve_unwind_operation("_Unwind_GetIPInfo", _Unwind_GetIPInfo);
	if (!operation) {
		if (before_instruction)
			*before_instruction = 0;
		return 0;
	}
	return operation(context, before_instruction);
}

void* _Unwind_GetLanguageSpecificData(struct _Unwind_Context* context)
{
	typedef void* (*get_data_operation)(struct _Unwind_Context*);
	get_data_operation operation = (get_data_operation)resolve_unwind_operation(
		"_Unwind_GetLanguageSpecificData", _Unwind_GetLanguageSpecificData);
	return operation ? operation(context) : NULL;
}

_Unwind_Ptr _Unwind_GetRegionStart(struct _Unwind_Context* context)
{
	typedef _Unwind_Ptr (*get_pointer_operation)(struct _Unwind_Context*);
	get_pointer_operation operation = (get_pointer_operation)
		resolve_unwind_operation("_Unwind_GetRegionStart",
		                         _Unwind_GetRegionStart);
	return operation ? operation(context) : 0;
}

_Unwind_Ptr _Unwind_GetTextRelBase(struct _Unwind_Context* context)
{
	typedef _Unwind_Ptr (*get_pointer_operation)(struct _Unwind_Context*);
	get_pointer_operation operation = (get_pointer_operation)
		resolve_unwind_operation("_Unwind_GetTextRelBase",
		                         _Unwind_GetTextRelBase);
	return operation ? operation(context) : 0;
}

_Unwind_Reason_Code _Unwind_RaiseException(
	struct _Unwind_Exception* exception)
{
	typedef _Unwind_Reason_Code (*raise_operation)(
		struct _Unwind_Exception*);
	raise_operation operation = (raise_operation)resolve_unwind_operation(
		"_Unwind_RaiseException", _Unwind_RaiseException);
	return operation ? operation(exception) : _URC_FATAL_PHASE1_ERROR;
}

void _Unwind_Resume(struct _Unwind_Exception* exception)
{
	typedef void (*resume_operation)(struct _Unwind_Exception*);
	resume_operation operation = (resume_operation)
		resolve_unwind_operation("_Unwind_Resume", _Unwind_Resume);
	if (operation)
		operation(exception);
	abort();
}

void _Unwind_SetGR(struct _Unwind_Context* context, int index,
	           _Unwind_Word value)
{
	typedef void (*set_register_operation)(
		struct _Unwind_Context*, int, _Unwind_Word);
	set_register_operation operation = (set_register_operation)
		resolve_unwind_operation("_Unwind_SetGR", _Unwind_SetGR);
	if (operation)
		operation(context, index, value);
}

void _Unwind_SetIP(struct _Unwind_Context* context, _Unwind_Ptr value)
{
	typedef void (*set_pointer_operation)(struct _Unwind_Context*, _Unwind_Ptr);
	set_pointer_operation operation = (set_pointer_operation)
		resolve_unwind_operation("_Unwind_SetIP", _Unwind_SetIP);
	if (operation)
		operation(context, value);
}
