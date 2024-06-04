// Sistemsko programiranje
// Osmi nagradni zadatak

#undef UNICODE
#undef UNICODE_

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
DWORD nameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
HANDLE localSlot, globalSlot, event;
LPCTSTR LocalSlotName = TEXT("\\\\.\\mailslot\\oglasna");
LPCTSTR GlobalSlotName = TEXT("\\\\*\\mailslot\\oglasna");
LPCTSTR EventName = TEXT("Global\\OglasnaEvent");

BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{
    localSlot = CreateMailslot(lpszSlotName,
        0,
        MAILSLOT_WAIT_FOREVER,
        (LPSECURITY_ATTRIBUTES)NULL);

    if (localSlot == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("CreateMailslot failed with %d\n"), GetLastError());
        return FALSE;
    }
    else _tprintf(_T("Mailslot created successfully.\n"));
    return TRUE;
}

BOOL WriteSlot(LPCTSTR lpszMessage)
{
    DWORD bytesToWrite = (DWORD)(lstrlen(lpszMessage) + 1) * sizeof(TCHAR);

    OVERLAPPED ov;
    ov.hEvent = CreateEvent(NULL,
        FALSE,
        FALSE,
        EventName);
    if (ov.hEvent == NULL)
    {
        _tprintf(_T("CreateEvent failed with %d.\n"), GetLastError());
        return FALSE;
    }
    ov.Offset = 0xFFFFFFFF;
    ov.OffsetHigh = 0xFFFFFFFF;

    BOOL fResult = WriteFile(globalSlot,
        lpszMessage,
        bytesToWrite,
        NULL,
        &ov);

    if (!fResult || !GetOverlappedResult(globalSlot, &ov, &bytesToWrite, TRUE))
    {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
        {
	        _tprintf(_T("WriteFile failed with %d.\n"), error);
        	return FALSE;
        }
    }

    _tprintf(_T("Slot written to successfully.\n"));
    CloseHandle(ov.hEvent);
    return TRUE;
}

BOOL ReadSlot()
{
    BOOL messagePrinted = FALSE;
    DWORD cbMessage, cMessage, cbRead, ignoredMessages = 0;
    TCHAR achID[80];
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;

    ov.hEvent = CreateEvent(NULL,
        FALSE,
        FALSE,
        EventName);
    if (ov.hEvent == NULL)
    {
        _tprintf(_T("CreateEvent failed with %d.\n"), GetLastError());
        return FALSE;
    }
    ov.Offset = 0;
    ov.OffsetHigh = 0;

    BOOL fResult = GetMailslotInfo(localSlot,
        (LPDWORD)NULL,
        &cbMessage,
        &cMessage,
        (LPDWORD)NULL);

    if (!fResult)
    {
        _tprintf(_T("GetMailslotInfo failed with %d.\n"), GetLastError());
        return FALSE;
    }

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        _tprintf(_T("Waiting for a message...\n"));
        return TRUE;
    }

    DWORD cAllMessages = cMessage;

    while (cMessage != 0)
    {
        StringCchPrintf((LPTSTR)achID,
            80,
            TEXT("\nMessage #%d of %d\n"),
            cAllMessages - cMessage + 1 - ignoredMessages,
            cAllMessages - ignoredMessages);

        LPTSTR lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,
            lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
        if (lpszBuffer == NULL)
        {
            _tprintf(_T("GlobalAlloc failed with %d.\n"), GetLastError());
            return FALSE;
        }
        lpszBuffer[0] = '\0';

        fResult = ReadFile(localSlot,
            lpszBuffer,
            cbMessage,
            &cbRead,
            &ov);

        if (!fResult)
        {
            _tprintf(_T("ReadFile failed with %d.\n"), GetLastError());
            GlobalFree((HGLOBAL)lpszBuffer);
            return FALSE;
        }

        StringCbCat(lpszBuffer,
            lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage,
            (LPTSTR)achID);

        if (strstr(lpszBuffer, computerName) == NULL)
        {
            _tprintf(_T("%hs\n"), lpszBuffer);
            messagePrinted = TRUE;
        }
        else ignoredMessages++;

        GlobalFree((HGLOBAL)lpszBuffer);

        fResult = GetMailslotInfo(localSlot,
            (LPDWORD)NULL,
            &cbMessage,
            &cMessage,
            (LPDWORD)NULL);

        if (!fResult)
        {
            _tprintf(_T("GetMailslotInfo failed (%d)\n"), GetLastError());
            return FALSE;
        }
    }

    if (!messagePrinted)
    {
        _tprintf(_T("Waiting for a message...\n"));
    }

    CloseHandle(ov.hEvent);
    return TRUE;
}

void help(void)
{
    _tprintf(_T("------------------------------\n"));
    _tprintf(_T("Za provjeru poruka unesite 1\n"));
    _tprintf(_T("Za slanje poruke unesite 2\n"));
    _tprintf(_T("Za izlazak unesite 3\n"));
    _tprintf(_T("'1'+ENTER\n"));
    _tprintf(_T("------------------------------\n"));
}

BOOL _tmain(void)
{
    if (!GetComputerName(computerName, &nameBufferSize))
    {
        _tprintf(_T("GetComputerName failed with %d.\n"), GetLastError());
        return FALSE;
    }

    TCHAR message[MAX_COMPUTERNAME_LENGTH + 1003];
    lstrcpy(message, computerName);
    lstrcat(message, ": ");
    MakeSlot(LocalSlotName);

    help();

    while(TRUE)
    {
        DWORD choice = 0;
        _tprintf(_T("Odabir: "));
        scanf("%ld", &choice);
        switch (choice)
        {
			case 1:
                ReadSlot();
                break;
			case 2:
                _tprintf(_T("Unesite poruku (max 1000 znakova, bez novih redova): \n"));
                getchar();
                scanf("%1000[^\n]", message + nameBufferSize + 2);
                globalSlot = CreateFile(GlobalSlotName,
                    GENERIC_WRITE,
                    FILE_SHARE_WRITE,
                    (LPSECURITY_ATTRIBUTES)NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    (HANDLE)NULL);
                if (globalSlot == INVALID_HANDLE_VALUE)
                {
                    _tprintf(_T("CreateFile failed with %d.\n"), GetLastError());
                    return FALSE;
                }
                WriteSlot(message);
                CloseHandle(globalSlot);
                break;
			case 3:
                CloseHandle(localSlot);
                return TRUE;
			default:
                help();
                break;
        }
    }
}