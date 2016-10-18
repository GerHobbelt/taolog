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
    unsigned int cch;      // �ַ���������null��
    wchar_t file[1024];     // �ļ�
    wchar_t func[1024];     // ����
    wchar_t text[ETW_LOGGER_MAX_LOG_SIZE];      // ��־
};
#pragma pack(pop)

// ������Ҫʹ�ø��������
struct LogDataUI : LogData
{
    typedef std::basic_string<TCHAR> string;

    static constexpr int data_cols = 10;

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

    // ��������
    int size() const
    {
        return data_cols;
    }

    inline const wchar_t* operator[](int i)
    {
        // ���� "" ������ null���Է�����
        if (i<0 || i > data_cols - 1)
            return L"";

        const wchar_t* value = L"";
        const LogDataUI* evt = this;

        switch(i)
        {
        case 0: value = evt->id;                            break;
        case 1: value = evt->strTime.c_str();               break;
        case 2: value = evt->strPid.c_str();                break;
        case 3: value = evt->strTid.c_str();                break;
        case 4: value = evt->strProject.c_str();            break;
        case 5: value = evt->file + evt->offset_of_file;    break;
        case 6: value = evt->func;                          break;
        case 7: value = evt->strLine.c_str();               break;
        case 8: value = evt->strLevel->c_str();             break;
        case 9: value = evt->text;                          break;
        }

        return value;
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

    string* strLevel;
};



}
