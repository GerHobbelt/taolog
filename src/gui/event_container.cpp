#include "stdafx.h"

#include "misc/config.h"
#include "logdata_define.h"

#include "event_container.h"

namespace taolog {

json11::Json EventContainer::to_json() const
{
    return json11::Json::object {
        {"name",        g_config.us(name)       },
        {"field_index", field_index             },
        {"field_name",  g_config.us(field_name) },
        {"value_index", value_index             },
        {"value_name",  g_config.us(value_name) },
        {"value_input", g_config.us(value_input)},
    };
}

std::wstring EventContainer::to_tip() const
{
    std::wstring str;

    str.reserve(2048);

    str += L"���֣�" + name + L"\bn";
    str += L"�ֶΣ�" + field_name + L"\bn";
    str += L"�ı���" + (!value_input.empty() ? value_input : value_name) + L"\bn";
    str += L"��ʱ��" + std::wstring(is_tmp ? L"��" : L"��") + L"\bn";

    return std::move(str);
}

EventContainer* EventContainer::from_json(const json11::Json& obj)
{
    EventContainer* p = nullptr;

    auto name           = obj["name"];
    auto field_index    = obj["field_index"];
    auto field_name     = obj["field_name"];
    auto value_index    = obj["value_index"];
    auto value_name     = obj["value_name"];
    auto value_input    = obj["value_input"];

    if(name.is_string()
        && field_index.is_number()
        && field_name.is_string()
        && value_index.is_number()
        && value_name.is_string()
        && value_input.is_string()
        )
    {
        p = new EventContainer(
            g_config.ws(name.string_value()),
            field_index.int_value(),
            g_config.ws(field_name.string_value()),
            value_index.int_value(),
            g_config.ws(value_name.string_value()),
            g_config.ws(value_input.string_value())
        );
    }

    return p;
}

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

void EventContainer::enable_filter(bool b)
{
    if(!b) {
        _filter = nullptr;
        return;
    }
    else {
        auto tolower = [](std::wstring s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };

        _value_input_lower = tolower(value_input);

        if(value_input[0] == L'~') {
            try {
                auto s = value_input.substr(1);
                _regex_input = std::wregex(s, std::regex_constants::icase);
            }
            catch(...) {
                throw std::wstring(L"�����������ʽ��");
            }
        }

        _filter = [&](const EVENT& evt) {
            const auto p = (*evt)[field_index];

            auto search_value_in_p = [&] {
                if(value_input[0] != L'~') {
                    auto haystack = tolower(std::wstring(p));
                    auto& needle = _value_input_lower;

                    return !!::wcsstr(haystack.c_str(), needle.c_str());
                }
                else {
                    return std::regex_search(p, _regex_input);
                }
            };

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
                return !search_value_in_p();
            }
            // ģ��
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

// ---------------------------------------------------------------------------------

void EventSearcher::reset(EventContainer* haystack, const std::vector<int>& cols, const std::wstring& s) throw(...)
{
    // ���������¼�����
    _haystack = haystack;

    // ����������
    // EventContainer ���в�֧�� -1
    // ��Ҫ�������л�
    _cols = cols;

    // �����������ֵ���ֶ�
    try {
        _needle.value_index = std::stoi(s);
    }
    catch(...) {
        _needle.value_index = -1;
    }

    // ����ǱȽ� value_name
    _needle.value_name = s;

    // ����ǱȽ� value_input
    _needle.value_input = s;

    // ���ÿ�ʼ����
    _last = -1;
    _needle.clear();

    // throws
    try {
        _needle.enable_filter(true);
        _valid = true;
    }
    catch(...) {
        _valid = false;
        throw;
    }
}

int EventSearcher::next(bool forward)
{
    if(!_valid) {
        return last(-1);
    }

    // ������ƥ�������
    for(int i = 0; i < _countof(_match_cols); i++)
        _match_cols[i] = false;

    // �õ���һ������
    auto la_next_line = [&] {return forward ? last() + 1 : last() - 1; };

    int next_line;
    bool valid = false;

    for(next_line = la_next_line()
        ; next_line >= 0 && next_line < (int)_haystack->size()
        ; next_line = la_next_line()
        )
    {
        auto log = (*_haystack)[next_line];

        for(const auto& index : _cols) {
            _needle.field_index = index;
            if(_needle.add(log)) {
                _match_cols[index] = true;
                valid = true;
            }
        }

        last(next_line);

        if(valid) break;
    }

    return valid ? last(next_line) : -1;
}

int EventSearcher::last() const
{
    return _last;
}

int EventSearcher::last(int i)
{
    _last = i;
    return _last;
}

void EventSearcher::invalid()
{
    _valid = false;
    _last = -1;
    _needle.clear();
    _needle.enable_filter(false);
}

const std::wstring& EventSearcher::s() const
{
    return _needle.value_name;
}

const bool (&EventSearcher::match_cols() const)[LogDataUI::cols()]
{
    return _match_cols;
}

} // namespace taolog
