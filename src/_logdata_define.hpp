#pragma once

#include <tchar.h>

#include <string>

#include <Windows.h>
#include <guiddef.h>

namespace taoetw {

#define ETW_LOGGER_MAX_LOG_SIZE (60*1024)

// һ��Ҫ�� etwlogger.h �еĶ���һ��
#pragma pack(push,1)
struct LogData
{
    GUID guid;              // ������ GUID
    SYSTEMTIME time;        // ʱ���
    unsigned int line;      // �к�
    unsigned int size;      // �ַ���������null��
    wchar_t file[1024];     // �ļ�
    wchar_t func[1024];     // ����
    wchar_t text[ETW_LOGGER_MAX_LOG_SIZE];      // ��־
};
#pragma pack(pop)

// ������Ҫʹ�ø��������
struct LogDataUI : LogData
{
    typedef std::basic_string<TCHAR> string;

    string to_string() const
    {
        TCHAR tmp[128];
        string str = text;

        str += L"\r\n\r\n" + string(80, L'-') + L"\r\n\r\n";
        _swprintf(tmp, L"��ţ�%s\r\n���̣�%d\r\n�̣߳�%d\r\n", id, pid, tid);
        str += tmp;

        auto& t = time;
        _swprintf(tmp, L"ʱ�䣺%d-%02d-%02d %02d:%02d:%02d:%03d\r\n",
            t.wYear, t.wMonth, t.wDay,
            t.wHour, t.wMinute, t.wSecond, t.wMilliseconds
        );
        str += tmp;

        str += L"��Ŀ��" + strProject + L"\r\n";
        str += string(L"�ļ���") + file + L"\r\n";
        str += string(L"������") + func + L"\r\n";
        str += L"�кţ�" + strLine + L"\r\n";
        str += L"�ȼ���" + std::to_wstring(level) + L"\r\n";

        return std::move(str);
    }

    unsigned int pid;       // ���̱�ʶ
    unsigned int tid;       // �̱߳�ʶ
    unsigned char level;    // ��־�ȼ�

    TCHAR id[22];
    string strTime;
    string strLine;
    string strPid;
    string strTid;

    string strProject;

    int offset_of_file;

    void* strLevel;
};



}
