#include <orbis/SaveData.h>
#include "orbisPad.h"
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sqlite3.h>

#define SUCCESS 0

#include "saves.h"
#include "util.h"

int sqlite3_memvfs_init(const char* vfsName);
int sqlite3_memvfs_dump(sqlite3 *db, const char *zSchema, const char *zFilename);

int orbis_SaveUmount(const char* mountPath)
{
	OrbisSaveDataMountPoint umount;

	memset(&umount, 0, sizeof(OrbisSaveDataMountPoint));
	strncpy(umount.data, mountPath, sizeof(umount.data));

	int32_t umountErrorCode = sceSaveDataUmount(&umount);
	
	if (umountErrorCode < 0)
	{
		klog("UMOUNT_ERROR (%X)", umountErrorCode);
		notify_popup(NULL, "Warning! Save couldn't be unmounted!");
		disable_unpatch();
	}

	return (umountErrorCode == SUCCESS);
}


int orbis_SaveMount(const save_entry_t *save, uint32_t mount_mode, char* mount_path)
{

	OrbisSaveDataDirName dirName;
	OrbisSaveDataTitleId titleId;
	int32_t saveDataInitializeResult = sceSaveDataInitialize3(0);

	if (saveDataInitializeResult != 0)
	{
		klog("Failed to initialize save data library (%X)", saveDataInitializeResult);
		return 0;
	}

	memset(&dirName, 0, sizeof(OrbisSaveDataDirName));
	memset(&titleId, 0, sizeof(OrbisSaveDataTitleId));
	strlcpy(dirName.data, save->dir_name, sizeof(dirName.data));
	strlcpy(titleId.data, save->title_id, sizeof(titleId.data));

	OrbisSaveDataMount mount;
	memset(&mount, 0, sizeof(OrbisSaveDataMount));

	OrbisSaveDataFingerprint fingerprint;
	memset(&fingerprint, 0, sizeof(OrbisSaveDataFingerprint));
	strlcpy(fingerprint.data, "0000000000000000000000000000000000000000000000000000000000000000", sizeof(fingerprint.data));

	mount.userId = user_id;
	mount.dirName = dirName.data;
	mount.fingerprint = fingerprint.data;
	mount.titleId = titleId.data;
	mount.blocks = save->blocks;
	mount.mountMode = mount_mode | ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;

	OrbisSaveDataMountResult mountResult;
	memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));

	int32_t mountErrorCode = sceSaveDataMount(&mount, &mountResult);
	if (mountErrorCode < 0)
	{
		klog("ERROR (%X): can't mount '%s/%s'", mountErrorCode, save->title_id, save->dir_name);
		return 0;
	}

	//klog("'%s/%s' mountPath (%s)", save->title_id, save->dir_name, mountResult.mountPathName);
	strncpy(mount_path, mountResult.mountPathName, ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE);

	return 1;
}

int orbis_UpdateSaveParams(const char* mountPath, const char* title, const char* subtitle, const char* details, uint32_t userParam)
{
	OrbisSaveDataParam saveParams;
	OrbisSaveDataMountPoint mount;

	memset(&saveParams, 0, sizeof(OrbisSaveDataParam));
	memset(&mount, 0, sizeof(OrbisSaveDataMountPoint));

	strlcpy(mount.data, mountPath, sizeof(mount.data));
	strlcpy(saveParams.title, title, ORBIS_SAVE_DATA_TITLE_MAXSIZE);
	strlcpy(saveParams.subtitle, subtitle, ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE);
	strlcpy(saveParams.details, details, ORBIS_SAVE_DATA_DETAIL_MAXSIZE);
	saveParams.userParam = userParam;
	saveParams.mtime = time(NULL);

	int32_t setParamResult = sceSaveDataSetParam(&mount, ORBIS_SAVE_DATA_PARAM_TYPE_ALL, &saveParams, sizeof(OrbisSaveDataParam));
	if (setParamResult < 0) {
		klog("sceSaveDataSetParam error (%X)", setParamResult);
		return 0;
	}

	return (setParamResult == 1);
}


static sqlite3* open_sqlite_db(const char* db_path)
{
	uint8_t* db_buf;
	size_t db_size;
	sqlite3 *db;

	if (sqlite3_memvfs_init("orbis_rw") != SQLITE_OK_LOAD_PERMANENTLY)
	{
		klog("Error loading extension: %s", "memvfs");
		return NULL;
	}

	if (read_buffer(db_path, &db_buf, &db_size) != SUCCESS)
	{
		klog("Cannot open database file: %s", db_path);
		return NULL;
	}

	// And open that memory with memvfs now that it holds a valid database
	char *memuri = sqlite3_mprintf("file:memdb?ptr=0x%p&sz=%lld&freeonclose=1", db_buf, db_size);
	klog("Opening '%s' (%ld bytes)...", db_path, db_size);

	if (sqlite3_open_v2(memuri, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI, "memvfs") != SQLITE_OK)
	{
		klog("Error open memvfs: %s", sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_free(memuri);

	if (sqlite3_exec(db, "PRAGMA journal_mode = OFF;", NULL, NULL, NULL) != SQLITE_OK)
		klog("Error set pragma: %s", sqlite3_errmsg(db));

	return db;
}