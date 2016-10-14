
#include <cassert>

#include <string>
#include <unordered_map>

#include <windows.h>
#include <evntrace.h>

#include "etwlogger.h"

#include <taowin/src/tw_taowin.h>

#include "main_window.h"

static const wchar_t* g_etw_session = L"taoetw-session";

namespace taoetw {

Consumer* g_Consumer;

LPCTSTR MainWindow::get_skin_xml() const
{
    LPCTSTR json = LR"tw(
<window title="ETW Log Viewer" size="820,600">
    <res>
        <font name="default" face="΢���ź�" size="12"/>
        <font name="12" face="΢���ź�" size="12"/>
    </res>
    <root>
        <vertical padding="5,5,5,5">
            <horizontal height="30" padding="0,4,0,4">
                <button name="start-logging" text="��ʼ��¼" width="60" />
                <control width="5" />
                <button name="stop-logging" text="ֹͣ��¼" width="60" />
                <control width="5" />
                <button name="module-manager" text="ģ�����" width="60" />
                <control width="5" />
            </horizontal>
            <listview name="lv" style="singlesel,showselalways,ownerdata" exstyle="clientedge">  </listview>
        </vertical>
    </root>
</window>
)tw";
    return json;
}

LRESULT MainWindow::handle_message(UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case WM_CREATE: return _on_create();
    case kDoLog:    return _on_log((ETWLogger::LogDataUI*)lparam);
    }
    return __super::handle_message(umsg, wparam, lparam);
}

LRESULT MainWindow::on_menu(int id, bool is_accel)
{
    /*
    if (id < (int)_menus.subs.size()) {
        _menus.subs[id].onclick();
    }
    */
    return 0;
}

LRESULT MainWindow::on_notify(HWND hwnd, taowin::control * pc, int code, NMHDR * hdr)
{
    if (!pc) {
        if (hwnd == _listview->get_header()) {
            if (code == NM_RCLICK) {
                return _on_select_column();
            }
            else if (code == HDN_ENDTRACK) {
                return _on_drag_column(hdr);
            }
        }

        return 0;
    }
    else if (pc->name() == _T("lv")) {
        if (code == LVN_GETDISPINFO) {
            return _on_get_dispinfo(hdr);
        }
        else if (code == NM_CUSTOMDRAW) {
            return _on_custom_draw_listview(hdr);
        }
        else if (code == NM_DBLCLK) {
            auto nmlv = reinterpret_cast<NMITEMACTIVATE*>(hdr);
            if (nmlv->iItem != -1) {
                _view_detail(nmlv->iItem);
            }
            return 0;
        }
    }
    else if (pc->name() == L"start-logging") {
        _start();
    }
    else if (pc->name() == L"stop-logging") {
        _stop();
    }
    else if (pc->name() == L"module-manager") {
        (new ModuleManager(_modules))->domodal(this);
    }

    return 0;
}

void MainWindow::_start()
{
    _controller.start(g_etw_session);

    GUID guid = { 0x630514b5, 0x7b96, 0x4b74,{ 0x9d, 0xb6, 0x66, 0xbd, 0x62, 0x1f, 0x93, 0x86 } };
    _controller.enable(guid, true, 0);

    _consumer.init(_hwnd, kDoLog);
    _consumer.start(g_etw_session);
}

void MainWindow::_stop()
{
    _controller.stop();
    _consumer.stop();
}

void MainWindow::_init_listview()
{
    _listview = _root->find<taowin::listview>(L"lv");

    _columns.push_back({ L"ʱ��", true, 160 });
    _columns.push_back({ L"����", true, 50 });
    _columns.push_back({ L"�߳�", true, 50, });
    _columns.push_back({ L"��Ŀ", true, 100, });
    _columns.push_back({ L"�ļ�", true, 200, });
    _columns.push_back({ L"����", true, 100, });
    _columns.push_back({ L"�к�", true, 50, });
    _columns.push_back({ L"��־", true, 300, });

    for (int i = 0; i < (int)_columns.size(); i++) {
        auto& col = _columns[i];
        _listview->insert_column(col.name.c_str(), col.width, i);
    }

    _colors[TRACE_LEVEL_INFORMATION]    = {RGB(  0,   0,   0), RGB(255, 255, 255)};
    _colors[TRACE_LEVEL_WARNING]        = {RGB(255, 128,   0), RGB(255, 255, 255)};
    _colors[TRACE_LEVEL_ERROR]          = {RGB(255,   0,   0), RGB(255, 255, 255)};
    _colors[TRACE_LEVEL_CRITICAL]       = {RGB(255, 255, 255), RGB(255,   0,   0)};
    _colors[TRACE_LEVEL_VERBOSE]        = {RGB(  0,   0,   0), RGB(255, 255, 255)};
}

void MainWindow::_init_menu()
{
    /*
    MenuEntry menu;

    menu.enable = true;
    menu.name = L"��ʼ��¼";
    menu.onclick = [&]() {
        _start();

    };
    _menus[L"start_logging"] = menu;

    menu.enable = true;
    menu.name = L"ֹͣ��¼";
    menu.onclick = [&]() {
        _stop();
    };
    _menus[L"stop_logging"] = menu;

    menu.enable = true;
    menu.name = L"ģ�����";
    menu.onclick = [&]() {
        (new ModuleManager(_modules))->domodal(this);
    };
    _menus[L"module_manager"] = menu;

    HMENU hMenu = ::CreateMenu();
    _menus

    int i = 0;
    for (auto it = _menus.begin(), end = _menus.end(); it != end; ++it, ++i) {
        auto& menu = it->second;
        menu.id = i;
        ::AppendMenu(hMenu, MF_STRING, i, _menus[i].name.c_str());
        ::EnableMenuItem(hMenu, i, MF_BYCOMMAND | (_menus[i].enable ? MF_ENABLED : MF_GRAYED | MF_DISABLED));
    }

    ::SetMenu(_hwnd, hMenu);
    */
}

void MainWindow::_view_detail(int i)
{
    auto evt = _events[i];
    auto& cr = _colors[evt->level];
    auto detail_window = new EventDetail(evt, cr.fg, cr.bg);
    detail_window->create();
    detail_window->show();
}

LRESULT MainWindow::_on_create()
{
    _init_listview();
    _init_menu();

    taoetw::g_Consumer = &_consumer;

    return 0;
}

LRESULT MainWindow::_on_log(ETWLogger::LogDataUI* item)
{
    const std::wstring* root = nullptr;

    _module_from_guid(item->guid, &item->strProject, &root);

    // ���·��
    if (*item->file && root) {
        if (::wcsnicmp(item->file, root->c_str(), root->size()) == 0) {
            item->offset_of_file = (int)root->size();
        }
    }
    else {
        item->offset_of_file = 0;
    }

    _events.push_back(item);
    _listview->set_item_count(_events.size(), LVSICF_NOINVALIDATEALL);

    return 0;
}

LRESULT MainWindow::_on_custom_draw_listview(NMHDR * hdr)
{
    LRESULT lr = CDRF_DODEFAULT;
    ETWLogger::LogDataUI* log;

    auto lvcd = (LPNMLVCUSTOMDRAW)hdr;

    switch (lvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        lr = CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_ITEMPREPAINT:
        log = _events[lvcd->nmcd.dwItemSpec];
        lvcd->clrText = _colors[log->level].fg;
        lvcd->clrTextBk = _colors[log->level].bg;
        lr = CDRF_NEWFONT;
        break;
    }

    return lr;
}

LRESULT MainWindow::_on_get_dispinfo(NMHDR * hdr)
{
    auto pdi = reinterpret_cast<NMLVDISPINFO*>(hdr);
    auto evt = _events[pdi->item.iItem];
    auto lit = &pdi->item;

    const TCHAR* value = _T("");

    switch (lit->iSubItem)
    {
    case 0: value = evt->strTime.c_str();               break;
    case 1: value = evt->strPid.c_str();                break;
    case 2: value = evt->strTid.c_str();                break;
    case 3: value = evt->strProject.c_str();            break;
    case 4: value = evt->file + evt->offset_of_file;    break;
    case 5: value = evt->func;                          break;
    case 6: value = evt->strLine.c_str();               break;
    case 7: value = evt->text;                          break;
    }

    lit->pszText = const_cast<TCHAR*>(value);

    return 0;
}

LRESULT MainWindow::_on_select_column()
{
    auto colsel = new ColumnSelection(_columns);

    colsel->OnToggle([&](int i) {
        auto& col = _columns[i];
        _listview->set_column_width(i, col.show ? col.width : 0);
    });

    colsel->domodal(this);

    return 0;
}

LRESULT MainWindow::_on_drag_column(NMHDR* hdr)
{
    auto nmhdr = (NMHEADER*)hdr;
    auto& item = nmhdr->pitem;
    auto& col = _columns[nmhdr->iItem];

    col.show = item->cxy != 0;
    if (item->cxy) col.width = item->cxy;

    return 0;
}

void MainWindow::_module_from_guid(const GUID & guid, std::wstring * name, const std::wstring ** root)
{
    if (!_guid2mod.count(guid)) {
        for (auto& item : _modules) {
            if (::IsEqualGUID(item->guid, guid)) {
                _guid2mod[guid] = item;
                break;
            }
        }
    }

    auto it = _guid2mod.find(guid);

    if (it != _guid2mod.cend()) {
        *name = it->second->name;
        *root = &it->second->root;
    }
    else {
        *name = L"<unknown>";
        *root = nullptr;
    }
}

}
