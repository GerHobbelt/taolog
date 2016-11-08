#include "stdafx.h"

#include "utils.h"

#include "config.h"

#include "_module_entry.hpp"

#include "module_manager.h"
#include "module_editor.h"

namespace taoetw {

LPCTSTR ModuleManager::get_skin_xml() const
{
    LPCTSTR json = LR"tw(
<window title="ģ�����" size="280,300">
    <res>
        <font name="default" face="΢���ź�" size="12"/>
        <font name="1" face="΢���ź�" size="12"/>
        <font name="consolas" face="Consolas" size="12"/>
    </res>
    <root>
        <horizontal padding="5,5,5,5">
            <listview name="list" style="showselalways,ownerdata,tabstop" exstyle="clientedge" />
            <vertical padding="5,5,5,5" width="50">
                <button name="enable" text="����" style="disabled,tabstop" height="24" />
                <control height="20" />
                <button name="add" text="���" height="24" style="tabstop" />
                <control height="5" />
                <button name="modify" text="�޸�" style="disabled,tabstop" height="24"/>
                <control height="5" />
                <button name="delete" text="ɾ��" style="disabled,tabstop" height="24"/>
                <control height="20" />
                <button name="copy" text="����" style="disabled,tabstop" height="24"/>
                <control height="5" />
                <button name="paste" text="ճ��" style="disabled,tabstop" height="24"/>
                <control height="5" />
            </vertical>
        </horizontal>
    </root>
</window>
)tw";
    return json;
}

LRESULT ModuleManager::handle_message(UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
    case WM_CREATE:
    {
        _listview   = _root->find<taowin::listview>(L"list");
        _btn_add    = _root->find<taowin::button>(L"add");
        _btn_enable = _root->find<taowin::button>(L"enable");
        _btn_modify = _root->find<taowin::button>(L"modify");
        _btn_delete = _root->find<taowin::button>(L"delete");
        _btn_copy   = _root->find<taowin::button>(L"copy");
        _btn_paste  = _root->find<taowin::button>(L"paste");

        _listview->insert_column(L"����", 150, 0);
        _listview->insert_column(L"״̬", 50, 1);

        _listview->set_item_count((int)_modules.size(), 0);

        _update_pastebtn_status();

        return 0;
    }

    case WM_CLOSE:
    {
        _save_modules();
        break;
    }

    case WM_ACTIVATE:
    {
        if(!_window_created)
            break;

        int state = (int)LOWORD(wparam);
        if(state == WA_ACTIVE || state == WA_CLICKACTIVE) {
            _update_pastebtn_status();
        }
        break;
    }

    }
    return __super::handle_message(umsg, wparam, lparam);
}

LRESULT ModuleManager::on_notify(HWND hwnd, taowin::control * pc, int code, NMHDR * hdr)
{
    if (!pc) return 0;

    if (pc == _listview) {
        if (code == LVN_GETDISPINFO) {
            auto pdi = reinterpret_cast<NMLVDISPINFO*>(hdr);
            auto& mod = _modules[pdi->item.iItem];
            auto lit = &pdi->item;

            const TCHAR* value = _T("");

            switch (lit->iSubItem) {
            case 0: value = mod->name.c_str();                   break;
            case 1: value = mod->enable ? L"������" : L"�ѽ���";  break;
            }

            lit->pszText = const_cast<TCHAR*>(value);

            return 0;
        }
        else if (code == NM_CUSTOMDRAW) {
            LRESULT lr = CDRF_DODEFAULT;
            ModuleEntry* mod;

            auto lvcd = (LPNMLVCUSTOMDRAW)hdr;

            switch (lvcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                lr = CDRF_NOTIFYITEMDRAW;
                break;

            case CDDS_ITEMPREPAINT:
                mod = _modules[lvcd->nmcd.dwItemSpec];

                if (mod->enable) {
                    lvcd->clrTextBk = RGB(255, 255, 255);
                    lvcd->clrText = RGB(0xff, 0x60, 0xd8);
                    lr = CDRF_NEWFONT;
                }
                break;
            }

            return lr;
        }
        else if (code == NM_DBLCLK) {
            auto nmlv = reinterpret_cast<NMITEMACTIVATE*>(hdr);

            if (nmlv->iItem != -1) {
                std::vector<int> items{ nmlv->iItem };
                _enable_items(items, -1);
            }

            return 0;
        }
        else if (code == LVN_ITEMCHANGED) {
            _on_item_state_change();
            return 0;
        }
    }
    else if (pc == _btn_enable) {
        if(code == BN_CLICKED) {
            std::vector<int> items;

            if(_listview->get_selected_items(&items)) {
                int state = _get_enable_state_for_items(items);

                // ˵��ѡ���˶����ͬ״̬����
                if(state == -1) {
                    int choice = msgbox(L"���ǡ�������Щģ�飻\n���񡿽�����Щģ�顣", MB_ICONQUESTION | MB_YESNOCANCEL);
                    if(choice == IDCANCEL) return 0;
                    state = choice == IDYES;
                }
                else {
                    state = state == 1 ? 0 : 1;
                }

                _enable_items(items, state);
            }

            return 0;
        }
    }
    else if (pc == _btn_add) {
        if(code == BN_CLICKED) {
            _add_item();
            return 0;
        }
    }
    else if (pc == _btn_modify) {
        if(code == BN_CLICKED) {
            int index = _listview->get_next_item(-1, LVNI_SELECTED);
            if(index != -1)
                _modify_item(index);
            return 0;
        }
    }
    else if (pc == _btn_delete) {
        if(code == BN_CLICKED) {
            std::vector<int> items;

            if(!_listview->get_selected_items(&items))
                return 0;

            // ��ȫ��Ϊ����״̬��ʱ�������ʾ����ȫ��Ϊ����ʱ��߻���ʾ��Щ��������״̬������ʾ�Ͳ���Ҫ�ˣ�
            const wchar_t* title = items.size() == 1 ? _modules[items[0]]->name.c_str() : L"ȷ��";
            int state = _get_enable_state_for_items(items);
            if((!_get_is_open() || state == 0) && msgbox((L"ȷ��Ҫɾ��ѡ�е� " + std::to_wstring(items.size()) + L" �").c_str(), MB_OKCANCEL | MB_ICONQUESTION, title) != IDOK)
                return 0;

            if(_get_is_open()) {
                // ������Щģ���Ѿ����ã��Ѿ����õ�ģ�鲻�ܱ�ɾ��
                std::wstring modules_enabled;

                for(auto i = items.begin(); i != items.end();) {
                    auto mod = _modules[*i];

                    if(mod->enable) {
                        wchar_t guid[128];

                        if(::StringFromGUID2(mod->guid, &guid[0], _countof(guid))) {
                            modules_enabled += L"    " + mod->name + L"\n";
                        }
                        i = items.erase(i);
                    }
                    else {
                        ++i;
                    }
                }

                if(!modules_enabled.empty()) {
                    std::wstring msg;

                    msg += L"����ģ�������Ѿ����ö�����ɾ����\n\n";
                    msg += modules_enabled;
                    msg += L"\n�Ƿ��ɾ�������õ�ģ�飿";

                    if(msgbox(msg, MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
                        return 0;
                }
            }

            _delete_items(items);

            return 0;
        }
    }
    else if(pc == _btn_copy) {
        if(code == BN_CLICKED) {
            _copy_items();
            return 0;
        }
    }
    else if(pc == _btn_paste) {
        if(code == BN_CLICKED) {
            _paste_items(false);
            return 0;
        }
    }

    return 0;
}

// state == -1 ����itemsΪ1��ʱ��Ч
void ModuleManager::_enable_items(const std::vector<int>& items, int state)
{
    bool enable = state == -1 ? !_modules[items[0]]->enable : state == 1;

    for (auto i : items) {
        auto& mod = _modules[i];

        std::wstring err;

        if (!_on_toggle(mod, enable, &err)) {
            msgbox(err, MB_ICONERROR, mod->name);
            continue;
        }

        mod->enable = enable;

        _listview->redraw_items(i, i);
    }

    _on_item_state_change();
}

void ModuleManager::_modify_item(int i)
{
    auto& mod = _modules[i];

    if (mod->enable && _get_is_open()) return;

    auto onok = [&](ModuleEntry* entry) {
        _listview->redraw_items(i, i);
    };

    auto onguid = [&](const GUID& guid, std::wstring* err) {
        if (_has_guid(guid) && !::IsEqualGUID(guid, mod->guid)) {
            *err = L"�� GUID �Ѿ����ڡ�";
            return false;
        }

        return true;
    };

    (new ModuleEntryEditor(mod, _levels ,onok, onguid))->domodal(this);
}

void ModuleManager::_delete_items(const std::vector<int>& items)
{
    // �Ӻ���ǰɾ
    for (auto it = items.crbegin(); it != items.crend(); it++) {
        _modules.erase(_modules.begin() + *it);
    }

    // just in case
    _listview->set_item_count((int)_modules.size(), 0);

    _listview->redraw_items(0, _listview->get_item_count());

    _on_item_state_change();
}

void ModuleManager::_add_item()
{
    auto onok = [&](ModuleEntry* entry) {
        _modules.push_back(entry);
        _listview->set_item_count((int)_modules.size(), LVSICF_NOINVALIDATEALL);

        int index = _listview->get_item_count() - 1;
        _listview->ensure_visible(index);   // ȷ���ɼ�
        _listview->set_item_state(-1, LVIS_SELECTED, 0); //ȡ��ѡ��������
        _listview->set_item_state(index, LVIS_SELECTED, LVIS_SELECTED); //ѡ�е�ǰ������
        async_call([&]() {_listview->focus(); }); // ֮ǰ�д��ڴ��ڴ������ڹر�״̬�������첽��
    };

    auto onguid = [&](const GUID& guid, std::wstring* err) {
        if (_has_guid(guid)) {
            *err = L"�� GUID �Ѿ����ڡ�";
            return false;
        }

        return true;
    };

    (new ModuleEntryEditor(nullptr, _levels, onok, onguid))->domodal(this);
}

bool ModuleManager::_has_guid(const GUID & guid)
{
    for (auto& mod : _modules) {
        if (::IsEqualGUID(guid, mod->guid)) {
            return true;
        }
    }

    return false;
}

int ModuleManager::_get_enable_state_for_items(const std::vector<int>& items)
{
    bool state = _modules[items[0]]->enable;
    bool same = true;

    for (auto i : items) {
        if (_modules[i]->enable != state) {
            same = false;
            break;
        }
    }

    return same ? (state ? 1 : 0) : -1;
}

void ModuleManager::_on_item_state_change()
{
    std::vector<int> items;
    _listview->get_selected_items(&items);

    bool opened = _get_is_open();

    _btn_enable->set_enabled(!items.empty());
    _btn_modify->set_enabled(items.size()==1 && (!opened || (items[0] != -1 && !_modules[items[0]]->enable)));
    _btn_delete->set_enabled(items.size() > 1 && (!opened || _get_enable_state_for_items(items) != 1) || (!items.empty() && (!opened || !_modules[items[0]]->enable)));
    _btn_copy->set_enabled(!items.empty());

    if (!items.empty()) {
        int state = _get_enable_state_for_items(items);
        _btn_enable->set_text(state != -1 ? (state == 1 ? L"����" : L"����") : L"��/��");
    }
}

void ModuleManager::_save_modules()
{
    auto& modules = g_config->arr("modules").as_arr();

    modules.clear();

    for(auto& m : _modules) {
        modules.push_back(*m);
    }
}

void ModuleManager::_copy_items()
{
    std::vector<int> indices;
    if(!_listview->get_selected_items(&indices))
        return;

    json11::Json::array objs;

    for(const auto& i : indices) {
        const auto& m = *_modules[i];
        objs.emplace_back(m);
    }

    auto str = g_config.ws(JsonWrapper(objs)->dump(true, 4));
    str += L'\n'; // ǿ��֢

    utils::set_clipboard_text(str);
}

bool ModuleManager::_paste_items(bool test)
{
    auto str = utils::get_clipboard_text();
    if(str.empty())
        return false;

    auto wstr = g_config.us(str);
    std::string err;

    auto json = json11::Json::parse(wstr.c_str(), err);
    if(!err.empty() || !json.is_array()) {
        if(!test) {
            msgbox(L"��Ч JSON �ַ�����");
        }
        return false;
    }

    auto has_module = [&](const GUID& guid)
    {
        bool has = false;

        for(auto& m : _modules) {
            if(guid == m->guid) {
                has = true;
                break;
           }
        }

        return has;
    };

    int count_added = 0;

    for(const auto& jm : json.array_items()) {
        auto m = ModuleEntry::from_json(jm);
        if(!m) continue;

        if(!has_module(m->guid))
            count_added++;

        if(!test) {
            // Ĭ�϶��ǲ�������
            m->enable = false;
            _modules.push_back(m);
        }
        else {
            delete m;
        }
    }

    if(!test && count_added) {
        _listview->set_item_count((int)_modules.size(), LVSICF_NOINVALIDATEALL);
        msgbox(L"ճ���� " + std::to_wstring(count_added) + L" ����");
    }

    return !!count_added;
}

void ModuleManager::_update_pastebtn_status()
{
    _btn_paste->set_enabled(_paste_items(true));
}

}
