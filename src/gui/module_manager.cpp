#include "stdafx.h"

#include "misc/utils.h"
#include "misc/config.h"

#include "misc/event_system.hpp"

#include "_module_entry.hpp"

#include "tooltip_window.h"

#include "module_manager.h"
#include "module_editor.h"

namespace taolog {

void ModuleManager::get_metas(WindowMeta * metas)
{
    __super::get_metas(metas);
    metas->style &= ~(WS_MINIMIZEBOX);
}

LPCTSTR ModuleManager::get_skin_xml() const
{
    LPCTSTR json = LR"tw(
<Window title="模块管理" size="280,250">
    <Resource>
        <Font name="default" face="微软雅黑" size="12"/>
        <Font name="1" face="微软雅黑" size="12"/>
        <Font name="consolas" face="Consolas" size="12"/>
    </Resource>
    <Root>
        <Horizontal padding="5,5,5,5">
            <ListView name="list" style="showselalways,ownerdata,tabstop" exstyle="clientedge" />
            <Vertical padding="5,5,5,5" width="50">
                <Button name="enable" text="启用" style="disabled,tabstop" height="24" />
                <Control height="20" />
                <Button name="add" text="添加" height="24" style="tabstop" />
                <Control height="5" />
                <Button name="modify" text="修改" style="disabled,tabstop" height="24"/>
                <Control height="5" />
                <Button name="delete" text="删除" style="disabled,tabstop" height="24"/>
                <Control height="20" />
                <Button name="copy" text="复制" style="disabled,tabstop" height="24"/>
                <Control height="5" />
                <Button name="paste" text="粘贴" style="disabled,tabstop" height="24"/>
                <Control height="5" />
            </Vertical>
        </Horizontal>
    </Root>
</Window>
)tw";
    return json;
}

LRESULT ModuleManager::handle_message(UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
    case WM_CREATE:
    {
        _tipwnd->create(this);
        _tipwnd->set_font(_mgr.get_font(L"default"));

        _listview   = _root->find<taowin::ListView>(L"list");
        _btn_add    = _root->find<taowin::Button>(L"add");
        _btn_enable = _root->find<taowin::Button>(L"enable");
        _btn_modify = _root->find<taowin::Button>(L"modify");
        _btn_delete = _root->find<taowin::Button>(L"delete");
        _btn_copy   = _root->find<taowin::Button>(L"copy");
        _btn_paste  = _root->find<taowin::Button>(L"paste");

        _listview->insert_column(L"名字", 150, 0);
        _listview->insert_column(L"状态", 50, 1);

        _listview->set_item_count((int)_modules.size(), 0);

        subclass_control(_listview);

        _update_pastebtn_status();

        _data_source.SetModules(&_modules);
        _listview->set_source(&_data_source);

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

LRESULT ModuleManager::control_message(taowin::SystemControl* ctl, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    // TODO static!!
    static bool mi = false;

    if(umsg == WM_MOUSEMOVE) {
        if(!mi) {
            taowin::set_track_mouse(ctl);
            mi = true;
        }
    }
    else if(umsg == WM_MOUSELEAVE) {
        mi = false;
    }
    else if(umsg == WM_MOUSEHOVER) {
        mi = false;

        POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

        if(ctl == _listview) {
            LVHITTESTINFO hti;;
            hti.pt = pt;
            if(_listview->subitem_hittest(&hti) != -1) {
                _tipwnd->format(_modules[hti.iItem]->to_tip());
            }
            return 0;
        }
    }

    return __super::control_message(ctl, umsg, wparam, lparam);
}

LRESULT ModuleManager::on_notify(HWND hwnd, taowin::Control * pc, int code, NMHDR * hdr)
{
    if (!pc) return 0;

    if (pc == _listview) {
        if (code == NM_CUSTOMDRAW) {
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

                // 说明选中了多个不同状态的项
                if(state == -1) {
                    int choice = msgbox(L"【是】启用这些模块；\n【否】禁用这些模块。", MB_ICONQUESTION | MB_YESNOCANCEL);
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

            // 在全部为禁用状态的时候进行提示（不全部为禁用时后边会提示哪些处于启用状态，此提示就不必要了）
            const wchar_t* title = items.size() == 1 ? _modules[items[0]]->name.c_str() : L"确认";
            int state = _get_enable_state_for_items(items);
            if((!_get_is_open() || state == 0) && msgbox((L"确定要删除选中的 " + std::to_wstring(items.size()) + L" 项？").c_str(), MB_OKCANCEL | MB_ICONQUESTION, title) != IDOK)
                return 0;

            if(_get_is_open()) {
                // 计算哪些模块已经启用，已经启用的模块不能被删除
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

                    msg += L"以下模块由于已经启用而不能删除：\n\n";
                    msg += modules_enabled;
                    msg += L"\n是否仅删除被禁用的模块？";

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

// state == -1 仅对items为1个时有效
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
            *err = L"此 GUID 已经存在。";
            return false;
        }

        return true;
    };

    (new ModuleEntryEditor(mod, _levels ,onok, onguid))->domodal(this);
}

void ModuleManager::_delete_items(const std::vector<int>& items)
{
    g_evtsys.trigger(L"project:del", &items);

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
        _listview->ensure_visible(index);   // 确保可见
        _listview->set_item_state(-1, LVIS_SELECTED, 0); //取消选中其它的
        _listview->set_item_state(index, LVIS_SELECTED, LVIS_SELECTED); //选中当前新增的
        async_call([&]() {_listview->focus(); }); // 之前有窗口处于正在关闭状态，所以异步调
        g_evtsys.trigger(L"project:new", entry);
    };

    auto onguid = [&](const GUID& guid, std::wstring* err) {
        if (_has_guid(guid)) {
            *err = L"此 GUID 已经存在。";
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
        _btn_enable->set_text(state != -1 ? (state == 1 ? L"禁用" : L"启用") : L"启/禁");
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
    str += L'\n'; // 强迫症

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
            msgbox(L"无效 JSON 字符串。");
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
            // 默认都是不开启的
            m->enable = false;
            _modules.push_back(m);
            g_evtsys.trigger(L"project:new", m);
        }
        else {
            delete m;
        }
    }

    if(!test && count_added) {
        _listview->set_item_count((int)_modules.size(), LVSICF_NOINVALIDATEALL);
        msgbox(L"粘贴了 " + std::to_wstring(count_added) + L" 个。");
    }

    return !!count_added;
}

void ModuleManager::_update_pastebtn_status()
{
    _btn_paste->set_enabled(_paste_items(true));
}

}
