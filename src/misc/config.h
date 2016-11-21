#pragma once

#include <cassert>

#include <json11/json11.hpp>
#include <codecvt>

namespace taoetw {


class JsonWrapper
{
public:
    typedef json11::Json Json;

public:
    // �ն���
    JsonWrapper()
    {

    }

    // �����й���
    JsonWrapper(const Json& json)
        : _json(json)
    { }

    // ����
    void attach(const Json& json)
    {
        _json = json;
    }

    // ֱ�ӷ���ԭ���Ķ���
    operator Json()
    {
        return _json;
    }

public:
    // ֱ�ӵ��� Json ����ķ���
    const Json* operator->()
    {
        return &_json;
    }

    // ֱ�ӵ���ԭ���� [int] ����
    const Json& operator[](size_t i) const
    {
        return _json[i];
    }

    // ֱ�ӵ���ԭ���� [const char*] ����
    const Json& operator[](const char* k) const
    {
        return _json[k];
    }

public:
    // ֱ����������ֵ
    Json& operator[](size_t i)
    {
        return as_arr()[i];
    }

    // ֱ�����ö����Աֵ
    Json& operator[](const char* k)
    {
        return as_obj()[k];
    }

public:
    // ��Ϊ�� const ����ʹ�ã���Ҫ�ֶ�ȷ��Ϊ���飩
    Json::array& as_arr()
    {
        assert(_json.is_array());
        return const_cast<Json::array&>(_json.array_items());
    }

    // ��Ϊ�� const ����ʹ�ã���Ҫ�ֶ�ȷ��Ϊ����
    Json::object& as_obj()
    {
        assert(_json.is_object());
        return const_cast<Json::object&>(_json.object_items());
    }

    // ��Ϊ�� const �ַ���ʹ�ã���Ҫ�ֶ�ȷ��Ϊ�ַ�����
    std::string& as_str()
    {
        assert(_json.is_string());
        return const_cast<std::string&>(_json.string_value());
    }

public:
    // �жϵ�ǰ�����Ƿ�����Ϊ k ������
    bool has_arr(const char* k)
    {
        assert(_json.is_object());

        return _json.has_shape({{k,Json::Type::ARRAY}}, _err);
    }

    // �жϵ�ǰ�����Ƿ�����Ϊ k �Ķ���
    bool has_obj(const char* k)
    {
        assert(_json.is_object());

        return _json.has_shape({{k,Json::Type::OBJECT}}, _err);
    }

    // �жϵ�ǰ�����Ƿ�����Ϊ k ���ַ���
    bool has_str(const char* k)
    {
        assert(_json.is_object());

        return _json.has_shape({{k, Json::Type::STRING}}, _err);
    }

    // ��ȡ [k] Ϊ���飨ȷ����
    JsonWrapper arr(const char* k)
    {
        if(!has_arr(k))
            as_obj()[k] = Json::array{};

        return _json[k];
    }

    // ��ȡ [k]  Ϊ����ȷ����
    JsonWrapper obj(const char* k)
    {
        if(!has_obj(k))
            as_obj()[k] = Json::object {};

        return _json[k];
    }

    // ��ȡ [k] Ϊ�ַ�����ȷ����
    JsonWrapper str(const char* k)
    {
        if(!has_str(k))
            as_obj()[k] = "";

        return _json[k];
    }

protected:
    Json _json;
    std::string  _err;
};

class Config
{
public:
    typedef std::wstring_convert<std::codecvt_utf8_utf16<unsigned short>, unsigned short> U8U16Cvt;

public:
    Config()
        : _new (false)
    {}

    bool load(const std::wstring& file);
    bool save();
    bool is_fresh() { return _new; }

    JsonWrapper* operator->() { return &_obj; }
    operator json11::Json() { return _obj; }
    JsonWrapper operator[](size_t i) { return _obj[i]; }
    JsonWrapper operator[](const char* k) { return _obj[k]; }

    // ת�� std::string �� std::wstring
    static std::wstring ws(const std::string& s);
    // ת�� std::wstring �� std::string
    static std::string us(const std::wstring& s);

protected:
    std::wstring _file;     // ��ǰ����·��
    JsonWrapper  _obj;      // �����ļ�������Ԫ��
    bool         _new;      // �Ƿ����´����������ļ�

};

extern Config& g_config;

}
