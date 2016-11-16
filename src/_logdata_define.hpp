#pragma once

namespace taoetw {

// #define ETW_LOGGER_MAX_LOG_SIZE (50*1024)

enum class ETW_LOGGER_FLAG {
    ETW_LOGGER_FLAG_UNICODE = 1,
};


// һ��Ҫ�� etwlogger.h �еĶ�������һ��
// ����
//      û�����һ��Ԫ��
//      �ַ������ܲ�һ��
#pragma pack(push,1)
struct LogData
{
    UCHAR           version;        // ��־�汾
    int             pid;            // ���̱��
    int             tid;            // �̱߳��
    UCHAR           level;          // ��־�ȼ�
    UINT            flags;          // ��־��صı��λ
    GUID            guid;           // ������ GUID
    SYSTEMTIME      time;           // ʱ���
    unsigned int    line;           // �к�
    unsigned int    cch;            // �ַ���������null��
    wchar_t         file[260];      // �ļ�
    wchar_t         func[260];      // ����
};
#pragma pack(pop)

// ������Ҫʹ�ø��������
struct LogDataUI : LogData
{
    typedef std::basic_string<TCHAR> string;
    typedef std::basic_stringstream<TCHAR> stringstream;
    typedef std::function<const wchar_t*(int i)> fnGetColumnName;

    static constexpr int data_cols = 10;

    static constexpr bool should_escape[data_cols] = {false,false,false,false,true,true,false,false,false,true};

    string to_string(fnGetColumnName get_column_name) const
    {
        TCHAR tmp[128];
        stringstream ss;

        auto gc = get_column_name;

        ss << strText;
        ss << L"\r\n\r\n" << string(50, L'-') << L"\r\n\r\n";
        ss << gc(0) << L"��" << id << L"\r\n";
        ss << gc(2) << L"��" << pid << L"\r\n";
        ss << gc(3) << L"��" << tid << L"\r\n";

        auto& t = time;
        _snwprintf(tmp, _countof(tmp), L"%s��%d-%02d-%02d %02d:%02d:%02d:%03d\r\n",
            gc(1),
            t.wYear, t.wMonth, t.wDay,
            t.wHour, t.wMinute, t.wSecond, t.wMilliseconds
        );

        ss << tmp;

        ss << gc(4) << L"��" << strProject   << L"\r\n";
        ss << gc(5) << L"��" << file         << L"\r\n";
        ss << gc(6) << L"��" << func         << L"\r\n";
        ss << gc(7) << L"��" << strLine      << L"\r\n";
        ss << gc(8) << L"��" << *strLevel    << L"\r\n";

        return std::move(ss.str());
    }

    inline const wchar_t* operator[](int i)
    {
        // ���� "" ������ null���Է�����
        if (i<0 || i > data_cols - 1)
            return L"";

        const wchar_t* value = L"";

        switch(i)
        {
        case 0: value = id;                            break;
        case 1: value = strTime.c_str();               break;
        case 2: value = strPid.c_str();                break;
        case 3: value = strTid.c_str();                break;
        case 4: value = strProject.c_str();            break;
        case 5: value = file + offset_of_file;         break;
        case 6: value = func;                          break;
        case 7: value = strLine.c_str();               break;
        case 8: value = strLevel->c_str();             break;
        case 9: value = strText.c_str();               break;
        }

        return value;
    }

    static void a2u(const char* a, wchar_t* u, int c) {
        ::MultiByteToWideChar(CP_ACP, 0, a, -1, u, c);
    }

    static LogDataUI* from_logdata(LogData* log, LogDataUI* place = nullptr)
    {
        const auto& log_data = *log;
        auto log_ui = place ? new (place) LogDataUI : new LogDataUI;

        // ��־����ǰ��Ĳ��ֶ���һ����
        // ������־��������ȹ̶������շ����Ȳ��̶�
        ::memcpy(log_ui, &log_data, sizeof(LogData));

        bool bIsUnicode = log_ui->flags & (int)ETW_LOGGER_FLAG::ETW_LOGGER_FLAG_UNICODE;

        // ����Ƿ� Unicode ����Ҫת��
        // ���� file, func, text
        if(!bIsUnicode) {
            char filebuf[_countof(log_ui->file)];
            ::strcpy(filebuf, (char*)log_ui->file);
            a2u(filebuf, log_ui->file, _countof(log_ui->file));

            char funcbuf[_countof(log_ui->func)];
            ::strcpy(funcbuf, (char*)log_ui->func);
            a2u(funcbuf, log_ui->func, _countof(log_ui->func));
        }

        // ������־���ģ�cch���� '\0'�����ʼ�մ�����
        if(bIsUnicode) {
            const wchar_t* pText = (const wchar_t*)((char*)&log_data + sizeof(LogData));
            assert(pText[log_ui->cch - 1] == 0);
            log_ui->strText.assign(pText, log_ui->cch - 1);
        }
        else {
            const char* pText = (const char*)((char*)&log_data + sizeof(LogData));
            assert(pText[log_ui->cch - 1] == 0);

            if(log_ui->cch > 4096) {
                auto p = std::make_unique<char[]>(log_ui->cch);
                memcpy(p.get(), pText, log_ui->cch);
                ::MultiByteToWideChar(CP_ACP, 0, p.get(), -1, (wchar_t*)pText, log_ui->cch);
                log_ui->strText.assign((wchar_t*)pText);
            }
            else {
                char buf[4096];
                memcpy(buf, pText, log_ui->cch);
                ::MultiByteToWideChar(CP_ACP, 0, buf, -1, (wchar_t*)pText, log_ui->cch);
                log_ui->strText.assign((wchar_t*)pText);
            }
        }

        log_ui->strPid  = std::to_wstring(log_ui->pid);
        log_ui->strTid  = std::to_wstring(log_ui->tid);
        log_ui->strLine = std::to_wstring(log_ui->line);

        {
            TCHAR buf[1024];
            auto& t = log_ui->time;

            _sntprintf(&buf[0], _countof(buf),
                _T("%02d:%02d:%02d:%03d"),
                t.wHour, t.wMinute, t.wSecond, t.wMilliseconds
            );

            log_ui->strTime = buf;
        }

        return log_ui;
    }

    string          strText;    // ��־������Ƚ����⣬��ԭ�ṹ�岢��ͬ

    TCHAR id[22];
    string strTime;
    string strLine;
    string strPid;
    string strTid;

    string strProject;

    int offset_of_file;

    string* strLevel;
};

typedef std::shared_ptr<LogDataUI> LogDataUIPtr;

}
