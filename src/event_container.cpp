#pragma once

#include "event_container.h"

namespace taoetw {

bool EventContainer::add(EVENT * evt)
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
    _reobj = std::wregex(rule, std::regex_constants::icase);
    _filter = [&](const EVENT* evt) {
        // �����Ҳ�֪����ô����base_int���ֶΣ��������⴦��Ϊ��Ч�ʣ�
        const wchar_t* p = nullptr;

        switch (base_int)
        {
        case 0: p = evt->strTime.c_str();       break;
        case 1: p = evt->strPid.c_str();        break;
        case 2: p = evt->strTid.c_str();        break;
        case 3: p = evt->strProject.c_str();    break;
        case 4: p = evt->file;                  break;
        case 5: p = evt->func;                  break;
        case 6: p = evt->strLine.c_str();       break;
        case 7: p = evt->text;                  break;
        }

        return !p || !std::regex_search(p, _reobj);
    };
}

}

