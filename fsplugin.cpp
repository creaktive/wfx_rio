#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include "fsplugin.h"
#include "rio.h"

//#include "parsecfg.h"
extern "C" int parsecfg (char *filename, char comment, int process(char *key, char *val));

#define VERSION "0.01a"


HMODULE hRioWfx;
int PluginNumber;
tProgressProc ProgressProc;
tLogProc LogProc;
tRequestProc RequestProc;


typedef struct
{
	CRio *pRio;
	CDirBlock cDirBlock;
	CDirHeader cDirHeader;
	CDirEntry *pDirEntry;
	int CountEntry, index;
} rio_handle;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	hRioWfx = (HMODULE) hModule;
    return TRUE;
}


static int port=0x378, delay_init=20000, delay_tx=100, delay_rx=2;


int proccfg(char *key, char *val)
{
	if (!stricmp(key, "IOPort"))
		sscanf(val, "%x", &port);
	else if (!stricmp(key, "IODelayInit"))
		sscanf(val, "%d", &delay_init);
	else if (!stricmp(key, "IODelayTx"))
		sscanf(val, "%d", &delay_tx);
	else if (!stricmp(key, "IODelayRx"))
		sscanf(val, "%d", &delay_rx);
	else
		MessageBox(NULL, key, "unknown parameter", MB_OK+MB_ICONSTOP);

	return 0;
}


BOOL RioClose(CRio *pRio)
{
	delete pRio;
	pRio = NULL;
	return TRUE;
}


CRio *RioOpen(void)
{
	CRio *pRio = new CRio;
	char cfg[MAX_PATH], *p;
	int len;

	if (!pRio)
	{
		MessageBox(NULL, "not enough memory", NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return NULL;
	}

	memset(cfg, '\0', sizeof(cfg));
	len = GetModuleFileName(hRioWfx, cfg, sizeof(cfg));
	for (p = cfg + len; p >= cfg; p--)
		if (*p == '\\')
		{
			*p = '\0';
			break;
		}
	strncat(cfg, "\\rio.cfg", MAX_PATH);
	parsecfg(cfg, '#', proccfg);

	/*
	_snprintf(cfg, sizeof(cfg), "[%x] [%d] [%d] [%d]", port, delay_init, delay_tx, delay_rx);
	MessageBox(NULL, cfg, NULL, MB_OK);
	*/

	if (!pRio->Set(port))
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return NULL;
	}

	pRio->SetIODelayInit(delay_init);
	pRio->SetIODelayTx(delay_tx);
	pRio->SetIODelayRx(delay_rx);
	pRio->UseExternalFlash(FALSE);

	if (!pRio->CheckPresent())
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return NULL;
	}

	if (!pRio->RxDirectory())
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return NULL;
	}

	return pRio;
}


int __stdcall FsInit(int PluginNr,tProgressProc pProgressProc,tLogProc pLogProc,tRequestProc pRequestProc)
{
	ProgressProc	= pProgressProc;
    LogProc			= pLogProc;
    RequestProc		= pRequestProc;
	PluginNumber	= PluginNr;

	LogProc(PluginNumber, MSGTYPE_CONNECT, "Rio MP3 Player plugin v" VERSION);
	LogProc(PluginNumber, MSGTYPE_DETAILS, "===============================");

	return 0;
}


HANDLE __stdcall FsFindFirst(char* Path,WIN32_FIND_DATA *FindData)
{
	CRio *pRio;
	rio_handle *hRio;
	char msg1[512], msg2[512], *time;

	if ((pRio = RioOpen()) == NULL)
		return INVALID_HANDLE_VALUE;

	if ((hRio = (rio_handle *) malloc(sizeof(rio_handle))) == NULL)
		return INVALID_HANDLE_VALUE;

	hRio->pRio			= pRio;
	hRio->cDirBlock		= pRio->GetDirectoryBlock();
	hRio->cDirHeader	= hRio->cDirBlock.m_cDirHeader;
	hRio->pDirEntry		= hRio->cDirBlock.m_acDirEntry;
	hRio->CountEntry	= hRio->cDirHeader.m_usCountEntry;

	time = asctime(localtime(&hRio->cDirHeader.m_lTimeLastUpdate));
	time[24] = '\0';
	_snprintf(msg1, sizeof(msg1), "flash last updated: %s", time);
	_snprintf(msg2, sizeof(msg2),
		"%ld Kb of %ld Kb free (%hu bad 32 Kb blocks)",
		((long)hRio->cDirHeader.m_usCount32KBlockRemaining * CRIO_SIZE_32KBLOCK) / 1024,
		((long)hRio->cDirHeader.m_usCount32KBlockAvailable * CRIO_SIZE_32KBLOCK) / 1024,
		hRio->cDirHeader.m_usCount32KBlockBad
				);
	LogProc(PluginNumber, MSGTYPE_DETAILS, msg1);
	LogProc(PluginNumber, MSGTYPE_DETAILS, msg2);

	if (hRio->CountEntry == 0)
	{
		SetLastError(ERROR_NO_MORE_FILES);
		FsFindClose(hRio);
		return INVALID_HANDLE_VALUE;
	}
	else if (hRio->CountEntry > CRIO_MAX_DIRENTRY)
		hRio->CountEntry = CRIO_MAX_DIRENTRY;

	hRio->index = 0;
	FsFindNext(hRio, FindData);

	return hRio;
}


BOOL __stdcall FsFindNext(HANDLE Hdl,WIN32_FIND_DATA *FindData)
{
	rio_handle *hRio = (rio_handle *) Hdl;

	if (hRio->index++ >= hRio->CountEntry)
		return FALSE;

	memset(FindData, 0, sizeof(WIN32_FIND_DATA));
	//_snprintf(FindData->cFileName, MAX_PATH, "%02d - %s", hRio->index, hRio->pDirEntry->m_szName);
	strncpy(FindData->cFileName, hRio->pDirEntry->m_szName, MAX_PATH);
	FindData->dwFileAttributes	= FILE_ATTRIBUTE_ARCHIVE;
	FindData->nFileSizeHigh		= 0;
	FindData->nFileSizeLow		= hRio->pDirEntry->m_lSize;

	SYSTEMTIME time_win;
	FILETIME   time_tc;
	struct tm *time_rio;
	memset(&time_win, '\0', sizeof(SYSTEMTIME));
	memset(&time_tc, '\0', sizeof(FILETIME));
	time_rio = gmtime(&hRio->pDirEntry->m_lTimeUpload);
	time_win.wYear		= time_rio->tm_year + 1900;
	time_win.wMonth		= time_rio->tm_mon + 1;
	time_win.wDay		= time_rio->tm_mday;
	time_win.wDayOfWeek	= time_rio->tm_wday;
	time_win.wHour		= time_rio->tm_hour;
	time_win.wMinute	= time_rio->tm_min;
	time_win.wSecond	= time_rio->tm_sec;
	time_win.wMilliseconds=0;
	SystemTimeToFileTime(&time_win, &time_tc);
	FindData->ftLastWriteTime = time_tc;

	hRio->pDirEntry++;
	return TRUE;
}


int __stdcall FsFindClose(HANDLE Hdl)
{
	rio_handle *hRio = (rio_handle *) Hdl;
	RioClose(hRio->pRio);
	free(hRio);
	return 1;
}


/*
BOOL __stdcall FsMkDir(char* Path)
{
}
*/


/*
int __stdcall FsExecuteFile(HWND MainWin,char* RemoteName,char* Verb)
{
}
*/


/*
int __stdcall FsRenMovFile(char* OldName,char* NewName,BOOL Move,BOOL OverWrite,RemoteInfoStruct* ri)
{
}
*/

static char src_file[MAX_PATH], dst_file[MAX_PATH];
static float size;

static BOOL RioProgress(int iPos, int iCount)
{
	float now = (float) iCount - (float) iPos;
	if (size == -1)
		size = now;
	return ProgressProc(PluginNumber, src_file, dst_file, (int)(((size - now) / size) * 100)) ? FALSE : TRUE;
}


int __stdcall FsGetFile(char* RemoteName,char* LocalName,int CopyFlags,RemoteInfoStruct* ri)
{
	CRio *pRio;

	if (GetFileAttributes(LocalName) != 0xFFFFFFFF)
		return FS_FILE_EXISTS;

	if ((pRio = RioOpen()) == NULL)
		return FS_FILE_READERROR;

	strncpy(src_file, RemoteName, MAX_PATH);
	strncpy(dst_file, LocalName, MAX_PATH);
	size = -1;

	if (!pRio->RxFile(RemoteName+1, LocalName, RioProgress))
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		DeleteFile(LocalName);
		return FS_FILE_READERROR;
	}

	return FS_FILE_OK;
}


int __stdcall FsPutFile(char* LocalName,char* RemoteName,int CopyFlags)
{
	CRio *pRio;

	if ((pRio = RioOpen()) == NULL)
		return FS_FILE_WRITEERROR;

	strncpy(src_file, LocalName, MAX_PATH);
	strncpy(dst_file, RemoteName, MAX_PATH);
	size = -1;

	CDirBlock &cDirBlock = pRio->GetDirectoryBlock();
	CDirHeader &cDirHeader = cDirBlock.m_cDirHeader;

	HANDLE h;
	if ((h = CreateFile(LocalName, GENERIC_READ, FILE_SHARE_READ+FILE_SHARE_WRITE+FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		return FS_FILE_READERROR;
	if (GetFileSize(h, NULL) >= ((unsigned long)cDirHeader.m_usCount32KBlockRemaining * CRIO_SIZE_32KBLOCK))
	{
		MessageBox(NULL, "device full", NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		CloseHandle(h);
		return FS_FILE_WRITEERROR;
	}
	CloseHandle(h);

	if (!pRio->TxFile(RemoteName+1, LocalName, RioProgress))
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return FS_FILE_WRITEERROR;
	}

	if (!pRio->TxDirectory())
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return FS_FILE_WRITEERROR;
	}

	return FS_FILE_OK;
}


BOOL __stdcall FsDeleteFile(char* RemoteName)
{
	CRio *pRio;

	if ((pRio = RioOpen()) == NULL)
		return FALSE;

	if (!pRio->RemoveFile(RemoteName+1))
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return FALSE;
	}

	if (!pRio->TxDirectory())
	{
		MessageBox(NULL, pRio->GetErrorStr(), NULL, MB_OK+MB_ICONSTOP);
		RioClose(pRio);
		return FALSE;
	}

	RioClose(pRio);
	return TRUE;
}


/*
BOOL __stdcall FsRemoveDir(char* RemoteName)
{
}
*/


/*
BOOL __stdcall FsSetAttr(char* RemoteName,int NewAttr)
{
}
*/


/*
BOOL __stdcall FsSetTime(char* RemoteName,FILETIME *CreationTime,
      FILETIME *LastAccessTime,FILETIME *LastWriteTime)
{
}
*/


BOOL __stdcall FsDisconnect(char* DisconnectRoot)
{
	return TRUE;
}


void __stdcall FsStatusInfo(char* RemoteDir,int InfoStartEnd,int InfoOperation)
{
	// This function may be used to initialize variables and to flush buffers
	
/*	char text[MAX_PATH];

	if (InfoStartEnd==FS_STATUS_START)
		strcpy(text,"Start: ");
	else
		strcpy(text,"End: ");
	
	switch (InfoOperation) {
	case FS_STATUS_OP_LIST:
		strcat(text,"Get directory list");
		break;
	case FS_STATUS_OP_GET_SINGLE:
		strcat(text,"Get single file");
		break;
	case FS_STATUS_OP_GET_MULTI:
		strcat(text,"Get multiple files");
		break;
	case FS_STATUS_OP_PUT_SINGLE:
		strcat(text,"Put single file");
		break;
	case FS_STATUS_OP_PUT_MULTI:
		strcat(text,"Put multiple files");
		break;
	case FS_STATUS_OP_RENMOV_SINGLE:
		strcat(text,"Rename/Move/Remote copy single file");
		break;
	case FS_STATUS_OP_RENMOV_MULTI:
		strcat(text,"Rename/Move/Remote copy multiple files");
		break;
	case FS_STATUS_OP_DELETE:
		strcat(text,"Delete multiple files");
		break;
	case FS_STATUS_OP_ATTRIB:
		strcat(text,"Change attributes of multiple files");
		break;
	case FS_STATUS_OP_MKDIR:
		strcat(text,"Create directory");
		break;
	case FS_STATUS_OP_EXEC:
		strcat(text,"Execute file or command line");
		break;
	default:
		strcat(text,"Unknown operation");
	}
	if (InfoOperation != FS_STATUS_OP_LIST)   // avoid recursion due to re-reading!
		MessageBox(0,text,RemoteDir,0);
*/
}


void __stdcall FsGetDefRootName(char* DefRootName,int maxlen)
{
	memset(DefRootName, '\0', maxlen);
	strncpy(DefRootName, "Rio MP3 Player", maxlen);
}
