#pragma once

#include <functional>

#include <Windows.h>

namespace taoetw {

class DebugView
{
    static constexpr int buffer_size = 4096; // fixed, don't change

    struct DbWinBuffer
    {
        DWORD pid;
        char str[buffer_size - sizeof(DWORD)];
    };

    typedef std::function<void(DWORD pid, const char* str)> OnNotify;

public:
    DebugView()
        : _hThread(nullptr)
        , _hMutex(nullptr)
        , _hFile(nullptr)
        , _hEvtBufReady(nullptr)
        , _hEvtDataReady(nullptr)
        , _pLog(nullptr)
    { }

public:
    bool init(OnNotify notify); // notify �����ڷ� init ʱ���߳�
    void uninit();

protected:
    static unsigned int __stdcall __ThreadProc(void* ud);
    unsigned int ThreadProc();

protected:
    OnNotify        _notify;

protected:
    HANDLE          _hThread;       // �����߳�
    HANDLE          _hMutex;        // OutputDebugString ������
    HANDLE          _hFile;         // �����ڴ�
    HANDLE          _hEvtBufReady;  // �����ڴ�����
    HANDLE          _hEvtDataReady; // ���ݱ���
    HANDLE          _hEvtExit;      // �˳��¼�
    DbWinBuffer*    _pLog;          // �����ڴ�ӳ��
};

}
