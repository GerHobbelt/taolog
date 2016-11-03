#include "stdafx.h"

#include "_logdata_define.hpp"

#include "event_container.h"

namespace taoetw {

bool EventContainer::add(EVENT& evt)
{
    if (!_filter || !_filter(evt)) {
        _events.push_back(evt);
        return true;
    }
    else {
        return false;
    }
}

bool EventContainer::filter_results(EventContainer* container)
{
    assert(container != this);

    for (auto& evt : _events)
        container->add(evt);

    return !container->_events.empty();
}

void EventContainer::_init()
{
    _filter = [&](const EVENT& evt) {
        const auto p = (*evt)[field_index];

        switch(field_index)
        {
        // ��ţ�ʱ�䣬���̣��̣߳��к�
        // ֱ��ִ������ԱȽϣ����ִ�Сд��
        case 0: case 1: case 2: case 3: case 7:
        {
            return p != value_input;
        }
        // �ļ�����������־
        // ִ�в����ִ�Сд������
        case 5: case 6: case 9:
        {
            auto tolower = [](std::wstring& s) {
                std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                return s;
            };

            auto haystack = tolower(std::wstring(p));
            auto needle = tolower(value_input);

            return !::wcsstr(haystack.c_str(), needle.c_str());
        }
        // ��Ŀ
        case 4:
        {
            return value_name != p;
        }
        // �ȼ�
        case 8:
        {
            return value_index != evt->level;
        }
        default:
            assert(0 && L"invalid index");
            return true;
        }
    };
}

}
