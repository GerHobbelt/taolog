#include "stdafx.h"

#include "logdata_define.h"

#include "event_container.h"

#include "event_detail.h"

namespace taolog {

void EventDetail::get_metas(WindowMeta * metas)
{
    __super::get_metas(metas);
    metas->style &= ~(WS_MINIMIZEBOX);
}

LPCTSTR EventDetail::get_skin_xml() const
{
    LPCTSTR json = LR"tw(
<Window title="详情" size="512,480">
    <Resource>
        <Font name="default" face="微软雅黑" size="12"/>
        <Font name="1" face="微软雅黑" size="12"/>
        <Font name="consolas" face="Consolas" size="12"/>
    </Resource>
    <Root>
        <Vertical>
            <TextBox name="text" font="consolas" style="multiline,vscroll,hscroll,readonly"/>
        </Vertical>
    </Root>
</Window>
)tw";
    return json;
}

LRESULT EventDetail::handle_message(UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case WM_CREATE:
    {
        auto edit = _root->find<taowin::TextBox>(L"text");
        _hText = edit->hwnd();

        auto logstr = _log->to_string(_gc);

        // 替换 \n 为 \r\n
        std::wregex re(LR"(\r?\n)");
        auto rs = std::regex_replace(logstr, re, L"\r\n");

        edit->set_text(rs.c_str());
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    {
        break; // 暂时不使用，眼花

        HDC hdc = (HDC)wparam;
        HWND hwnd = (HWND)lparam;

        if (hwnd == _hText) {
            ::SetTextColor(hdc, _fg);
            ::SetBkColor(hdc, _bg);
            return LRESULT(_hbrBk);
        }

        break;
    }

    }
    return __super::handle_message(umsg, wparam, lparam);
}

}
