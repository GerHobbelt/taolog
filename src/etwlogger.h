#pragma once 

/*
 *    etw��������־��ϵͳ����С����̬�����Ƿ������־������Ŀ��ģ����Բ�����productrelease�汾

   1. ֧��TCHAR�ַ����̰߳�ȫ
   2. ÿ��ģ��(dll��̬exe)��Ҫ����ETWLogger���󣬲��ڹ��캯����ģ��Ķ���providerguid
   3. ���ڵײ�ľ�̬���ӿ⣬����Ҫ����ETWLogger����(�ڵ���ģ�鲽��2���涨��)��ֱ�ӵ��ýӿ�

 * 
*/

#ifdef ETW_LOGGER
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>
#include <wmistr.h>
#include <Evntrace.h>


#define MAX_LOGGER_BUF (1024)
#define MAX_LOGGER_HEAP_BUF (60 * 1024)

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WFUNCTION__ WIDEN(__FUNCTION__)

#ifdef _UNICODE
#define __TFILE__ __WFILE__
#define __TFUNCTION__ __WFUNCTION__
#else
#define __TFILE__ __FILE__
#define __TFUNCTION__ __FUNCTION__
#endif

enum LOGTYPE
{
	LOGSTACK = 20,
	LOGHEAP
};

// ���� buffer ����С�� MAX_LOGGER_BUF ����ջ�ڴ�ռ�
struct LogStack
{	
	TCHAR data[MAX_LOGGER_BUF];
};

// ���� buffer ���ȳ��� MAX_LOGGER_BUF ���ö�̬���ٵ��ڴ�ռ�
// ������ EVENT_TRACE_HEADER ���buffer size ���ܳ��� 64k
struct LogHeap
{	
	USHORT len;
	TCHAR data;
};

struct Log
{
	GUID provider;
	SYSTEMTIME st;
	union
	{
		LogStack logS;
		LogHeap logH;
	} u;	
};

typedef struct _logevent
{
	EVENT_TRACE_HEADER Header;
	Log log;	
} LOGEVENT_DATA, *PLOGEVENT_DATA;

class ETWLogger
{
public:
	ETWLogger(const GUID & providerGuid)
		:m_providerGuid(providerGuid)
		,m_registrationHandle(NULL)
		,m_sessionHandle(NULL)
		,m_traceOn(FALSE)
		,m_enableLevel(0)
		,m_reg(FALSE)
	{
		::InitializeCriticalSection(&m_cs);

		// This GUID defines the event trace class. 	
		// {6E5E5CBC-8ACF-4fa6-98E4-0C63A075323B}
		const GUID clsGuid = 
		{ 0x6e5e5cbc, 0x8acf, 0x4fa6, { 0x98, 0xe4, 0xc, 0x63, 0xa0, 0x75, 0x32, 0x3b } };

		m_clsGuid = clsGuid;

		RegisterProvider();
	}

	~ETWLogger()
	{
		::DeleteCriticalSection(&m_cs);

		UnRegisterProvider();
	}

	void lock()
	{
		::EnterCriticalSection(&m_cs);
	}

	void unlock()
	{
		::LeaveCriticalSection(&m_cs);
	}

	static ULONG WINAPI ControlCallback(WMIDPREQUESTCODE requestCode, PVOID context, ULONG* reserved, PVOID buffer)
	{
		ULONG ret = ERROR_SUCCESS;	

		UNREFERENCED_PARAMETER(reserved);// ����������ȼ�4���棬û���κ�����

		ETWLogger * logger = (ETWLogger *)context;
		if(logger != NULL && buffer != NULL)
		{
			logger->lock();

			switch (requestCode)
			{
			case WMI_ENABLE_EVENTS:  //Enable Provider.
				{
					logger->m_sessionHandle = GetTraceLoggerHandle(buffer);
					if (INVALID_HANDLE_VALUE == (HANDLE)logger->m_sessionHandle)
					{
						wprintf(L"GetTraceLoggerHandle failed with, %d.\n", ret);
						break;
					}

					SetLastError(ERROR_SUCCESS);
					logger->m_enableLevel = GetTraceEnableLevel(logger->m_sessionHandle); 
					if (!logger->m_enableLevel)
					{
						DWORD id = GetLastError();
						if (id != ERROR_SUCCESS)
						{
							wprintf(L"GetTraceEnableLevel failed with, %d.\n", ret);
							break;
						} 
						else  // Decide what a zero enable level means to your provider
						{
							logger->m_enableLevel = TRACE_LEVEL_WARNING; 
						}
					}	

					wprintf(L"Tracing enabled for Level %u\n", logger->m_enableLevel);

					logger->m_traceOn = TRUE;
					break;
				}

			case WMI_DISABLE_EVENTS:  // Disable Provider.
				{
					logger->m_traceOn = FALSE;
					logger->m_sessionHandle = NULL;
					break;
				}

			default:
				{
					ret = ERROR_INVALID_PARAMETER;
					break;
				}
			}

			logger->unlock();
		}			

		return ret;
	}

	void RegisterProvider()
	{
		TRACE_GUID_REGISTRATION eventClassGuids[] = { (LPGUID)&m_clsGuid, NULL};

		ULONG status = ::RegisterTraceGuids( (WMIDPREQUEST)ControlCallback, this, (LPGUID)&m_providerGuid, 
			sizeof(eventClassGuids)/sizeof(TRACE_GUID_REGISTRATION), eventClassGuids, NULL, NULL, &m_registrationHandle);
		if (ERROR_SUCCESS == status)
		{
			m_reg = TRUE;
		}
	}

	void UnRegisterProvider()
	{
		if(m_reg && m_registrationHandle != NULL)
		{
			::UnregisterTraceGuids(m_registrationHandle);
			m_registrationHandle = NULL;
			m_reg = FALSE;
		}		
	}

	// �ж��Ƿ��ӡ��־
	BOOL IsLog(unsigned char level)
	{
		BOOL ret = FALSE;

		lock();

		if (m_traceOn && level <= m_enableLevel)
		{
			ret = TRUE;
		}

		unlock();

		return ret;
	}

	void WriteEvent(unsigned char level, const TCHAR * data)
	{
		lock();	

		if(data != NULL)
		{
			LOGEVENT_DATA logEvent;

			ZeroMemory(&logEvent, sizeof(logEvent) );
			logEvent.Header.Size = USHORT( (char *)logEvent.log.u.logS.data - (char *)&logEvent) + (USHORT)(_tcslen(data) + 1) * sizeof(TCHAR);
			logEvent.Header.Flags = WNODE_FLAG_TRACED_GUID /*| WNODE_FLAG_USE_MOF_PTR*/;
			logEvent.Header.Guid = m_clsGuid;
			logEvent.Header.Class.Type = LOGSTACK;
			logEvent.Header.Class.Level = level;			

			_tcscpy_s(logEvent.log.u.logS.data, data);			
			logEvent.log.provider = m_providerGuid;

			SYSTEMTIME st = {0};													
			GetSystemTime(&st);														
			SystemTimeToTzSpecificLocalTime(NULL, &st, &logEvent.log.st);				

			ULONG status = ::TraceEvent(m_sessionHandle, &(logEvent.Header) );
			if (ERROR_SUCCESS != status)
			{
				//Decide how to handle failures. Typically, you do not
				//want to terminate the application because you failed to
				//log an event. If the error is a memory failure, you may
				//may want to log a message to the system event log or turn
				//off logging.
				wprintf(L"TraceEvent() event failed, %d\n", status);
				if (ERROR_INVALID_HANDLE == status)
				{
					m_traceOn = FALSE;
				}
			}
		}	

		unlock();
	}

	void WriteHugeEvent(unsigned char level, const TCHAR * data)
	{
		lock();	

		if(data != NULL)
		{
			USHORT len = (USHORT)(_tcslen(data) + 1) * sizeof(TCHAR);

			LOGEVENT_DATA * logEvent = (LOGEVENT_DATA * )malloc(sizeof(LOGEVENT_DATA) + len);
			if(logEvent != NULL)
			{
				ZeroMemory(logEvent, sizeof(LOGEVENT_DATA) + len);			

				logEvent->Header.Size = USHORT( (char *)&logEvent->log.u.logH.data - (char *)logEvent) + len;
				logEvent->Header.Flags = WNODE_FLAG_TRACED_GUID /*| WNODE_FLAG_USE_MOF_PTR*/;
				logEvent->Header.Guid = m_clsGuid;
				logEvent->Header.Class.Type = LOGHEAP;
				logEvent->Header.Class.Level = level;


				logEvent->log.u.logH.len = len;
				memcpy(&logEvent->log.u.logH.data, data, len);
				logEvent->log.provider = m_providerGuid;

				SYSTEMTIME st = {0};													
				GetSystemTime(&st);														
				SystemTimeToTzSpecificLocalTime(NULL, &st, &logEvent->log.st);				

				ULONG status = ::TraceEvent(m_sessionHandle, &(logEvent->Header) );
				if (ERROR_SUCCESS != status)
				{					
					wprintf(L"TraceEvent() event failed, %d\n", status);
					if (ERROR_INVALID_HANDLE == status)
					{
						m_traceOn = FALSE;
					}
				}

				free(logEvent);
				logEvent = NULL;
			}			
		}	

		unlock();
	}

private:
	
	UCHAR m_enableLevel; 
	BOOL m_traceOn; 
	BOOL m_reg;
	GUID m_providerGuid;
	GUID m_clsGuid;
	TRACEHANDLE m_registrationHandle;
	TRACEHANDLE m_sessionHandle;

	CRITICAL_SECTION m_cs;
};

extern ETWLogger g_etwLogger;


// �󲿷�Ӧ�ó�������ջ�ռ��ӡ��־���������Ƚϴ����־��Ϣʱ��ʹ�öѿռ��ӡ��־
inline void LogMessage(unsigned char level, const TCHAR * file, const TCHAR * function, unsigned int line, const TCHAR * format, ...)
{
	assert(file != NULL && function != NULL && format != NULL);

	if(file != NULL && function != NULL && format != NULL && g_etwLogger.IsLog(level) )
	{		
		va_list args;
		va_start(args, format);			

		ULONG textLen = _vsctprintf(format, args) + 1;
		textLen += _tcslen(file);
		textLen += _tcslen(function);
		textLen += _tcslen(_T("[%s - %s - %u] %s") );
		textLen += sizeof(line);

		if(textLen < MAX_LOGGER_BUF) // С�ڸó���ʹ��ջ�ڴ棬���ܻ�ȽϺã�����������ڴ���Ƭ�Ƚ���
		{
			TCHAR content[MAX_LOGGER_BUF] = {0};
			TCHAR text[MAX_LOGGER_BUF] = {0};

			_vstprintf_s(content, sizeof(content)/sizeof(TCHAR), format, args);

			_stprintf_s(text, sizeof(text)/sizeof(TCHAR), _T("[%s - %s - %u] %s"), file, function, line, content);

			g_etwLogger.WriteEvent(level, text);
		}
		else if(textLen * sizeof(TCHAR) < MAX_LOGGER_HEAP_BUF) // ����ܳ���60k
		{
			TCHAR * content = new TCHAR[textLen];
			TCHAR * text = new TCHAR[textLen];

			if(content != NULL && text != NULL)
			{
				ZeroMemory(content, textLen * sizeof(TCHAR) );
				ZeroMemory(text, textLen * sizeof(TCHAR) );

				_vstprintf_s(content, textLen, format, args);

				_stprintf_s(text, textLen, _T("[%s - %s - %u] %s"), file, function, line, content);

				g_etwLogger.WriteHugeEvent(level, text);

				delete []content;
				content = NULL;
				delete []text;
				text = NULL;
			}
			
		}
		else if(textLen * sizeof(TCHAR) >= MAX_LOGGER_HEAP_BUF)
		{			
			TCHAR text[MAX_LOGGER_BUF] = {0};			

			_stprintf_s(text, sizeof(text)/sizeof(TCHAR), _T("[%s - %s - %u] etw logger buffer is bigger than 60k so it is over flow"), file, function, line);

			g_etwLogger.WriteEvent(level, text);
		}

		va_end(args);
	}	
}

// Abnormal exit or termination events
#define ETW_LEVEL_CRITICAL(x, ...) \
{ \
 LogMessage(TRACE_LEVEL_CRITICAL, __TFILE__, __TFUNCTION__, __LINE__, x, __VA_ARGS__); \
}

// Severe error events
#define ETW_LEVEL_ERROR(x, ...) \
{ \
LogMessage(TRACE_LEVEL_ERROR, __TFILE__, __TFUNCTION__, __LINE__, x, __VA_ARGS__); \
}

// Warning events such as allocation failures
#define ETW_LEVEL_WARNING(x, ...) \
{ \
LogMessage(TRACE_LEVEL_WARNING, __TFILE__, __TFUNCTION__, __LINE__, x, __VA_ARGS__); \
}

// Non-error events such as entry or exit events
#define ETW_LEVEL_INFORMATION(x, ...) \
{ \
	LogMessage(TRACE_LEVEL_INFORMATION, __TFILE__, __TFUNCTION__, __LINE__, x, __VA_ARGS__); \
}

// Detailed trace events
#define ETW_LEVEL_VERBOSE(x, ...) \
{ \
	LogMessage(TRACE_LEVEL_VERBOSE, __TFILE__, __TFUNCTION__, __LINE__, x, __VA_ARGS__); \
}

// winapi last error, level TRACE_LEVEL_WARNING
#define ETW_LAST_ERROR() \
{  																		    \
	DWORD errid = ::GetLastError();										    \
	TCHAR erridBuf[MAX_PATH] = {0};											\
	_stprintf_s(erridBuf, sizeof(erridBuf)/sizeof(TCHAR), _T("errorid=%u"), errid);	\
	ETW_LEVEL_WARNING(erridBuf); 													\
}


#else 

#define ETW_LEVEL_CRITICAL(x, ...) 
#define ETW_LEVEL_ERROR(x, ...)
#define ETW_LEVEL_WARNING(x, ...)
#define ETW_LEVEL_INFORMATION(x, ...)
#define ETW_LEVEL_VERBOSE(x, ...)

#define ETW_LAST_ERROR()

#endif