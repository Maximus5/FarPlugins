#ifndef _MODULE_DEF_H_
#define _MODULE_DEF_H_

#define MODULE_EXPORT __stdcall

// Extract progress callbacks
typedef int (CALLBACK *ExtractProgressFunc)(HANDLE, __int64);

struct ExtractProcessCallbacks
{
	HANDLE signalContext;
	ExtractProgressFunc FileProgress;
};

#define ACTUAL_API_VERSION 3
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
	const wchar_t* FilePath;
	const char* Password;
};

struct StorageItemInfo
{
	DWORD Attributes;
	__int64 Size;
	FILETIME CreationTime;
	FILETIME ModificationTime;
	wchar_t Path[1024];
};

struct ExtractOperationParams 
{
	int item;
	int flags;
	const wchar_t* destFilePath;
	ExtractProcessCallbacks callbacks;
};

typedef int (MODULE_EXPORT *OpenStorageFunc)(StorageOpenParams, HANDLE*, StorageGeneralInfo*);
typedef void (MODULE_EXPORT *CloseStorageFunc)(HANDLE);
typedef int (MODULE_EXPORT *GetItemFunc)(HANDLE, int, StorageItemInfo*);
typedef int (MODULE_EXPORT *ExtractFunc)(HANDLE, ExtractOperationParams params);

struct ModuleLoadParameters
{
	//IN
	const wchar_t* Settings;
	//OUT
	DWORD ModuleVersion;
	DWORD ApiVersion;
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetItem;
	ExtractFunc ExtractItem;
};

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

// Extract error reactions
#define EEN_ABORT 1
#define EEN_RETRY 2
#define EEN_SKIP 3
#define EEN_SKIPALL 4
#define EEN_CONTINUE 5
#define EEN_CONTINUESILENT 6

#endif
