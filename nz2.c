/*
 * SISTEMSKO PROGRAMIRANJE
 * NAGRADNI ZADATAK 2.
 *
 * 1. Compile file (C -> EXE)
 * 2. Otvoriti direktorij gdje je EXE u terminalu
 * 3. Izvršiti '.\<exe> [putanja]'
 * 4. Putanja mora završavati sa '\'
 * 5. Ako putanja sadrži razmake, nema veze, i dalje funkcionira
 * 6. npr. '.\<exe> C:\Users\leotu\OneDrive\Documents\Emotion Classification\'
 */

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <malloc.h>
#include <tchar.h>

typedef struct Node
{
    SLIST_ENTRY ItemEntry;
    LPTSTR line;
}NODE, *PNODE;

void PrintHead(LPTSTR xml, LPCTSTR print, LPDWORD nFiles, long long* size, LPTSTR tabs);
void EnterDirectory(LPTSTR partialPath, LPCTSTR suffix, DWORD level, LPCTSTR dirName, LPDWORD nFiles, long long* size);
void BaseFunction(LPTSTR path, LPTSTR fullPath, DWORD level, LPCTSTR dirName, LPDWORD nFiles, long long* size);

HANDLE file = NULL;
PSLIST_HEADER pListHead;

DWORD _tmain(DWORD argc, LPTSTR argv[])
{
    if (argc < 2)
    {
        _tprintf(_T("Nedovoljno argumenata. Uporaba skripte: '.\\<exe> [putanja]'\n"));
        return 1;
    }

    _TCHAR path[MAX_PATH];
    _tcscpy(path, argv[1]);
    _tprintf(_T("%s\n"), argv[1]); // Iz nekog razloga ne funkcionira bez ovog

    if (argc > 2)
    {
	    for (DWORD i = 2; i < argc; i++)
	    {
            if ((_tcslen(path) + _tcslen(argv[i]) + 2) > MAX_PATH)
            {
                _tprintf(_T("Predugačka početna putanja.\n"));
                return 2;
            }

            _tcscat(path, _T(" "));
            _tprintf(_T("%s\n"), argv[i]);
		    _tcscat(path, argv[i]);
	    }
    }

    pListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER),
        MEMORY_ALLOCATION_ALIGNMENT);
    if (pListHead == NULL)
    {
        _tprintf(_T("_aligned_malloc failed and returned NULL.\n"));
        return 3;
    }
    InitializeSListHead(pListHead);

    _TCHAR fullPath[MAX_PATH];
    _stprintf(fullPath, _T("%s%s"), path, _T("*"));

    file = CreateFile(_T("dirTree.xml"),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("CreateFile failed and returned INVALID_HANDLE_VALUE.\n"));
        return 4;
    }

    DWORD nFiles = 0;
	long long size = 0;

    BaseFunction(path, fullPath, 0, NULL, &nFiles, &size);

    PSLIST_ENTRY pFirstEntry;
    while ((pFirstEntry = InterlockedPopEntrySList(pListHead)) != NULL)
    {
        PNODE pNode = (PNODE)pFirstEntry;

        DWORD bytes = WideCharToMultiByte(CP_UTF8,
            0,
            pNode->line,
            _tcslen(pNode->line),
            NULL,
            0,
            NULL,
            NULL);

        LPSTR writeStr = (LPSTR)HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, 
            bytes);

        WideCharToMultiByte(CP_UTF8,
            0,
            pNode->line,
            _tcslen(pNode->line),
            writeStr,
            bytes,
            NULL,
            NULL);

        DWORD writtenBytes;
        if (!WriteFile(file, (LPCVOID)writeStr, bytes, &writtenBytes, NULL) || bytes != writtenBytes)
        {
            _tprintf(_T("WriteFile failed.\n"));
            exit(5);
        }

        HeapFree(GetProcessHeap(), 0, writeStr);
        HeapFree(GetProcessHeap(), 0, pNode->line);
        _aligned_free(pFirstEntry);
    }

    InterlockedFlushSList(pListHead);
    _aligned_free(pListHead);
    CloseHandle(file);
    return 0;
}

void PrintHead(LPTSTR xml, LPCTSTR print, LPDWORD nFiles, long long* size, LPTSTR tabs)
{
    LPTSTR spaceCheck = wcsstr(print, _T(" \0"));

    if (spaceCheck != NULL)
    {
        _TCHAR printPath[1000];

        _tcscpy(printPath, print);
        for (unsigned long long i = 0; i < _tcslen(printPath); i++)
            if (print[i] == _T(' '))
                printPath[i] = _T('_');

        if (tabs == NULL)
			_stprintf(xml, _T("<directory name=\"%s\" nFiles=\"%lu\" size=\"%lld\">\n"), printPath, *nFiles, *size);
        else
            _stprintf(xml, _T("%s<directory name=\"%s\" nFiles=\"%lu\" size=\"%lld\">\n"), tabs, printPath, *nFiles, *size);
    }

    else
    {
	    if (tabs == NULL)
            _stprintf(xml, _T("<directory name=\"%s\" nFiles=\"%lu\" size=\"%lld\">\n"), print, *nFiles, *size);
        else
            _stprintf(xml, _T("%s<directory name=\"%s\" nFiles=\"%lu\" size=\"%lld\">\n"), tabs, print, *nFiles, *size);
    }
}

void EnterDirectory(LPTSTR partialPath, LPCTSTR suffix, DWORD level, LPCTSTR dirName, LPDWORD nFiles, long long* size)
{
    // If directory name is too long, skip.
    if ((_tcslen(partialPath) + _tcslen(suffix) + 2) > MAX_PATH)
        return;

    _TCHAR path[MAX_PATH];
    _stprintf(path, _T("%s%s%s"), partialPath, suffix, _T("\\"));

	_TCHAR fullPath[MAX_PATH];
    _stprintf(fullPath, _T("%s%s"), path, _T("*"));

    DWORD dirnFiles = 0;
	long long dirsize = 0;
    BaseFunction(path, fullPath, level, dirName, &dirnFiles, &dirsize);
    *nFiles += dirnFiles;
    *size += dirsize;
}

void BaseFunction(LPTSTR path, LPTSTR fullPath, DWORD level, LPCTSTR dirName, LPDWORD nFiles, long long* size)
{
    WIN32_FIND_DATA data;
    const HANDLE handle = FindFirstFile(fullPath, &data);

    LPTSTR xml = (LPTSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        1000 * sizeof(_TCHAR));

    if (xml == NULL)
    {
        _tprintf(_T("HeapAlloc failed and returned NULL.\n"));
        exit(6);
    }

    _TCHAR tabs[100];

    _tcscpy(tabs, _T(""));
    for (DWORD i = 0; i < level; i++)
        _tcscat(tabs, _T("\t"));

    PNODE temp = (PNODE)_aligned_malloc(sizeof(NODE),
        MEMORY_ALLOCATION_ALIGNMENT);

    if (temp == NULL)
    {
        _tprintf(_T("_aligned_malloc failed and returned NULL.\n"));
        exit(7);
    }
    
    _stprintf(xml, _T("%s</directory>\n"), tabs);
    temp->line = xml;
    InterlockedPushEntrySList(pListHead, &(temp->ItemEntry));

    // Skip . and ..
    FindNextFile(handle, &data);

    while (FindNextFile(handle, &data))
    {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (_tcscmp(data.cAlternateFileName, _T("")) != 0)
                EnterDirectory(path, data.cAlternateFileName, level + 1, data.cFileName, nFiles, size);
            else 
                EnterDirectory(path, data.cFileName, level + 1, data.cFileName, nFiles, size);
        }

        else
        {
            (*nFiles)++;
	        *size += ((long long)data.nFileSizeHigh * ((long long)MAXDWORD + 1)) + (long long)data.nFileSizeLow;
        }
    }

    xml = (LPTSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        1000 * sizeof(_TCHAR));

    if (xml == NULL)
    {
        _tprintf(_T("HeapAlloc failed and returned NULL.\n"));
        exit(8);
    }

    temp = (PNODE)_aligned_malloc(sizeof(NODE),
        MEMORY_ALLOCATION_ALIGNMENT);

    if (temp == NULL)
    {
        _tprintf(_T("HeapAlloc failed and returned NULL.\n"));
        exit(9);
    }

    if (dirName == NULL)
    {
        PrintHead(xml, path, nFiles, size, NULL);
    }
    else
    {
        PrintHead(xml, dirName, nFiles, size, tabs);
    }

    temp->line = xml;
    InterlockedPushEntrySList(pListHead, &(temp->ItemEntry));
}