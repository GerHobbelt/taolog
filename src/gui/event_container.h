#pragma once

namespace taoetw {

class EventContainer
{
protected:
    typedef LogDataUIPtr EVENT;
    typedef std::function<bool(const EVENT event)> FILTER;
    typedef std::vector<EVENT> EVENTS;

public:
    EventContainer()
    {
        is_tmp = false;
        enable_filter(false);
    }

    EventContainer(
        const std::wstring& name,
        int field_index, const std::wstring& field_name,
        int value_index, const std::wstring& value_name,
        const std::wstring& value_input
        )
        : name(name)
        , field_index(field_index)
        , field_name(field_name)
        , value_index(value_index)
        , value_name(value_name)
        , value_input(value_input)
    {
        enable_filter(true);
    }

public:
    json11::Json to_json() const;
    std::wstring to_tip() const;
    static EventContainer* from_json(const json11::Json& obj);

public:
    bool add(EVENT& evt);
    bool filter_results(EventContainer* container);
    EVENT operator[](int index) { return _events[index]; }
    size_t size() const { return _events.size(); }
    void clear() { return _events.clear(); }
    EVENTS& events() { return _events; }
    void enable_filter(bool b);

public:
    std::wstring name;              // ����������
    std::wstring field_name;        // �����ĸ��ֶ�
    int          field_index;       // �ֶ�����
    std::wstring value_name;        // ��ѡ��ֵ������
    int          value_index;       // ��ѡ��ֵ������
    std::wstring value_input;       // �Զ�������

    bool         is_tmp;            // ��ʱʹ�õ�

protected:

    EVENTS          _events;
    FILTER          _filter;

    std::wstring _value_input_lower;
};

typedef std::vector<EventContainer*> EventContainerS;

typedef std::pair<EventContainer, EventContainerS> EventPair;
}
