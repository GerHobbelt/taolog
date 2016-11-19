#include "stdafx.h"

#include "config.h"

#include "mem_pool.hpp"

#include "_logdata_define.hpp"
#include "_module_entry.hpp"

#include "event_container.h"
#include "listview_color.h"
#include "column_selection.h"
#include "result_filter.h"

#include "debug_view.h"

#include "mini_view.h"

namespace taoetw {

RECT MiniView::winpos = {-1,-1};

// TODO ��δ���Ҳ��ֱ�Ӵ� main �а������
void MiniView::update()
{
    // Ĭ���Ƿ��Զ����������һ�е�
    // �������ǰ�����������һ�У����Զ�����
    int count = (int)_events.size();
    int sic_flag = LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL;
    bool is_last_focused = count > 1 && (_listview->get_item_state(count - 2, LVIS_FOCUSED) & LVIS_FOCUSED)
        || _listview->get_next_item(-1, LVIS_FOCUSED) == -1;

    if(is_last_focused) {
        sic_flag &= ~LVSICF_NOSCROLL;
    }

    _listview->set_item_count(count, sic_flag);

    if(is_last_focused) {
        _listview->set_item_state(-1, LVIS_FOCUSED | LVIS_SELECTED, 0);
        _listview->ensure_visible(count - 1);
        _listview->set_item_state(count - 1, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
    }
}

LRESULT MiniView::_on_logcmd(LogMessage::Value cmd, LPARAM lParam)
{
    if(cmd == LogMessage::Alloc) {
        return (LRESULT)_logpool.alloc();
    }
    else if(cmd == LogMessage::Dealloc) {
        auto p = reinterpret_cast<LogDataUI*>(lParam);
        p->~LogDataUI();
        _logpool.destroy(p);
        return 0;
    }
    else if(cmd == LogMessage::Log) {
        LogDataUIPtr item((LogDataUI*)lParam, [&](LogDataUI* p) {
            _on_logcmd(LogMessage::Dealloc, LPARAM(p));
        });

        // ��־���
        _snwprintf(item->id, _countof(item->id), L"%llu", (unsigned long long)_events.size() + 1);

        item->offset_of_file = 0;

        // �ַ�����ʽ����־�ȼ�
        item->strLevel = nullptr;

        // ȫ���¼�����
        _events.add(item);

        // �ж�һ�µ�ǰ�������Ƿ�����˴��¼�
        // ���û����ӣ��Ͳ���Ҫˢ���б�ؼ���
        bool added_to_current = _curflt == &_events;

        // �����˵��¼�������ָ�븴�ã�
        if(!_filters.empty()) {
            for(auto& f : _filters) {
                bool added = f->add(item);
                if(f == _curflt && added && !added_to_current)
                    added_to_current = true;
            }
        }

        if(added_to_current) {
            // Ĭ���Ƿ��Զ����������һ�е�
            // �������ǰ�����������һ�У����Զ�����
            int count = (int)_curflt->size();
            int sic_flag = LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL;
            bool is_last_focused = count > 1 && (_listview->get_item_state(count - 2, LVIS_FOCUSED) & LVIS_FOCUSED)
                || _listview->get_next_item(-1, LVIS_FOCUSED) == -1;

            if(is_last_focused) {
                sic_flag &= ~LVSICF_NOSCROLL;
            }

            _listview->set_item_count(count, sic_flag);

            if(is_last_focused) {
                _listview->set_item_state(-1, LVIS_FOCUSED | LVIS_SELECTED, 0);
                _listview->ensure_visible(count - 1);
                _listview->set_item_state(count - 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            }
        }

        return 0;
    }

    return 0;
}

LPCTSTR MiniView::get_skin_xml() const
{
    LPCTSTR json = LR"tw(
<window title="DebugView" size="600,400">
    <res>
        <font name="default" face="΢���ź�" size="12"/>
    </res>
    <root>
        <vertical padding="3,3,3,3">
            <listview name="lv" style="showselalways,ownerdata" exstyle="clientedge,doublebuffer"/>
        </vertical>
    </root>
</window>
)tw";
    return json;
}

LRESULT MiniView::handle_message(UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case kLogCmd:
        return _on_logcmd(LogMessage::Value(wparam), lparam);

    case WM_CREATE:
    {
        _curflt = &_events;

        _listview = _root->find<taowin::listview>(L"lv");

        // Ĭ���ö�
        ::SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // �ָ�����λ��
        if(winpos.left != -1 && winpos.top != -1) {
            taowin::Rect rc = winpos;
            ::SetWindowPos(_hwnd, nullptr, rc.left, rc.top, rc.width(), rc.height(), SWP_NOZORDER);
        }

        _columns.emplace_back(L"���", true,   50, "id"   ,0);
        _columns.emplace_back(L"ʱ��", true,   86, "time" ,1);
        _columns.emplace_back(L"����", true ,  50, "pid"  ,2);
        _columns.emplace_back(L"��־", true,  300, "log"  ,3);

        for (int i = 0; i < (int)_columns.size(); i++) {
            auto& col = _columns[i];
            _listview->insert_column(col.name.c_str(), col.show ? col.width : 0, i);
        }

        async_call([&] {
            auto fnGetFields = [&](std::vector<std::wstring>* fields, int* def) {
                for(auto& c : _columns) {
                    fields->emplace_back(c.name);
                }

                *def = (int)_columns.size() - 1;
            };

            auto fnGetValues = [&](int idx, std::unordered_map<int, const wchar_t*>* values) {
                values->clear();
            };

            AddNewFilter dlg(fnGetFields, fnGetValues);
            if(dlg.domodal(this) == IDOK) {
                _events.name        = dlg.name;
                _events.field_index = dlg.field_index;
                _events.field_name  = dlg.field_name;
                _events.value_index = dlg.value_index;
                _events.value_name  = dlg.value_name;
                _events.value_input = dlg.value_input;

                _events.enable_filter(true);
            }

            if(!_dbgview.init([&](DWORD pid, const char* str) {
                auto p = (LogDataUI*)::SendMessage(_hwnd, kLogCmd, LogMessage::Alloc, 0);
                p = LogDataUI::from_dbgview(pid, str, p);
                ::PostMessage(_hwnd, kLogCmd, LogMessage::Log, LPARAM(p));
            }))
            {
                async_call([&]() {
                    msgbox(L"�޷����� DebugView ��־����ǰ������������ DebugView ��־�鿴���������С�", MB_ICONINFORMATION);
                    close();
                });
            }
        });

        return 0;
    }
    case WM_SIZE:
    {
        break;
    }
    case WM_CLOSE:
    {
        ::GetWindowRect(_hwnd, &winpos);

        _dbgview.uninit();

        DestroyWindow(_hwnd);

        return 0;
    }
    }
    return __super::handle_message(umsg, wparam, lparam);
}

LRESULT MiniView::on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr)
{
    if(pc == _listview) {
        if(code == LVN_GETDISPINFO) {
            auto pdi = reinterpret_cast<NMLVDISPINFO*>(hdr);
            auto& evt = _events[pdi->item.iItem];
            auto lit = &pdi->item;
            auto field = (*evt)[pdi->item.iSubItem];

            lit->pszText = const_cast<LPWSTR>(field);

            return 0;
        }
        else if(code == NM_CUSTOMDRAW) {
            return _on_custom_draw_listview(hdr);
        }
    }

    return __super::on_notify(hwnd, pc, code, hdr);
}

LRESULT MiniView::_on_custom_draw_listview(NMHDR* hdr)
{
    LRESULT lr = CDRF_DODEFAULT;

    auto lvcd = (LPNMLVCUSTOMDRAW)hdr;

    switch (lvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        lr = CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_ITEM | CDDS_PREPAINT:
        break;
        auto& log = *_events[lvcd->nmcd.dwItemSpec];
        // lvcd->clrText = _colors[log.level].fg;
        // lvcd->clrTextBk = _colors[log.level].bg;
        lr = CDRF_NEWFONT;
        break;
    }

    return lr;
}


}
