#ifndef _MODULE_DEF_H_
#define _MODULE_DEF_H_

#define MODULE_EXPORT __stdcall

// Extract progress callbacks
typedef int (CALLBACK *ExtractProgressFunc)(HANDLE, __int64);

#pragma pack(push, 1)

struct ExtractProcessCallbacks
{
	HANDLE signalContext;
	ExtractProgressFunc FileProgress;
};

#define ACTUAL_API_VERSION 6
#define STORAGE_FORMAT_NAME_MAX_LEN 32
#define STORAGE_PARAM_MAX_LEN 64

struct StorageGeneralInfo
{
	wchar_t Format[STORAGE_FORMAT_NAME_MAX_LEN];
	wchar_t Compression[STORAGE_PARAM_MAX_LEN];
	wchar_t Comment[STORAGE_PARAM_MAX_LEN];
	FILETIME Created;
};

struct StorageOpenParams
{
	size_t StructSize;
	const wchar_t* FilePath;
	const char* Password;
	const void* Data;
	size_t DataSize;
};

struct StorageItemInfo
{
	__int64 Size;
	__int64 PackedSize;
	DWORD Attributes;
	FILETIME CreationTime;
	FILETIME ModificationTime;
	WORD NumHardlinks;
	wchar_t Owner[64];
	wchar_t Path[1024];
};

struct ExtractOperationParams 
{
	int ItemIndex;
	int Flags;
	const wchar_t* DestPath;
	const char* Password;
	ExtractProcessCallbacks Callbacks;
};

typedef int (MODULE_EXPORT *OpenStorageFunc)(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo *info);
typedef int (MODULE_EXPORT *PrepareFilesFunc)(HANDLE storage);
typedef void (MODULE_EXPORT *CloseStorageFunc)(HANDLE storage);
typedef int (MODULE_EXPORT *GetItemFunc)(HANDLE storage, int item_index, StorageItemInfo* item_info);
typedef int (MODULE_EXPORT *ExtractFunc)(HANDLE storage, ExtractOperationParams params);

struct module_cbs
{
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetItem;
	ExtractFunc ExtractItem;
	PrepareFilesFunc PrepareFiles;
};

struct ModuleLoadParameters
{
	//IN
	size_t StructSize;
	const wchar_t* Settings;
	//OUT
	GUID ModuleId;
	DWORD ModuleVersion;
	DWORD ApiVersion;
	module_cbs ApiFuncs;
};

#pragma pack(pop)

// Function that should be exported from modules
typedef int (MODULE_EXPORT *LoadSubModuleFunc)(ModuleLoadParameters*);
typedef void (MODULE_EXPORT *UnloadSubModuleFunc)(void);

#define MAKEMODULEVERSION(mj,mn) ((mj << 16) | mn)
#define STRBUF_SIZE(x) ( sizeof(x) / sizeof(x[0]) )

// Open storage return results
#define SOR_INVALID_FILE 0
#define SOR_SUCCESS 1
#define SOR_PASSWORD_REQUIRED 2

// Item retrieval result
#define GET_ITEM_ERROR 0
#define GET_ITEM_OK 1
#define GET_ITEM_NOMOREITEMS 2

// Extract result
#define SER_SUCCESS 0
#define SER_ERROR_WRITE 1
#define SER_ERROR_READ 2
#define SER_ERROR_SYSTEM 3
#define SER_USERABORT 4
#define SER_PASSWORD_REQUIRED 5

#endif
