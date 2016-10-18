#pragma once

#include <tchar.h>

#include <string>

#include <Windows.h>
#include <guiddef.h>

namespace taoetw {

#define ETW_LOGGER_MAX_LOG_SIZE (60*1024)

// һ��Ҫ�� etwlogger.h �еĶ���һ��
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

// ������Ҫʹ�ø��������
struct LogDataUI : LogData
{
    typedef std::basic_string<TCHAR> string;

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
