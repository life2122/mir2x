#include "colorf.hpp"
#include "sdldevice.hpp"
#include "menuboard.hpp"

extern SDLDevice *g_sdlDevice;

MenuBoard::MenuBoard(dir8_t argDir,
        int argX,
        int argY,

        std::optional<int> argWOpt,

        int argItemSpace,
        int argSeperatorSpace,

        std::initializer_list<std::pair<Widget *, bool>> argMenuItemList,
        std::function<void(Widget *)> argOnClickMenu,

        std::array<int, 4> argMargin,

        Widget *argParent,
        bool    argAutoDelete)

    : Widget
      {
          argDir,
          argX,
          argY,
          0,
          0,
          {},
          argParent,
          argAutoDelete,
      }

    , m_wOpt(argWOpt)
    , m_itemSpace(argItemSpace)
    , m_seperatorSpace(argSeperatorSpace)
    , m_onClickMenu(std::move(argOnClickMenu))
    , m_margin(argMargin)
{
    if(m_wOpt.has_value()){
        fflassert(m_wOpt.value() > 0, m_wOpt);
    }

    setSize(m_wOpt.value_or(0) + m_margin[2] + m_margin[3], m_margin[0] + m_margin[1]);

    for(auto p: argMenuItemList){
        addChild(p.first, p.second);
    }
}

void MenuBoard::addChild(Widget *widget, bool autoDelete)
{
    if(widget){
        const bool firstChild = m_childList.empty();
        Widget::addChild(widget, autoDelete);

        if(firstChild){
            widget->moveAt(DIR_UPLEFT, m_margin[2],       m_margin[0] + m_itemSpace / 2);
        }
        else{
            widget->moveAt(DIR_UPLEFT, m_margin[2], h() - m_margin[1] + m_itemSpace / 2);
        }

        if(!m_wOpt.has_value()){
            m_w = std::max<int>(w(), widget->w() + m_margin[2] + m_margin[3]);
        }

        m_h = widget->dy() + widget->h() + (m_itemSpace - m_itemSpace / 2) + m_margin[1];
    }
}

void MenuBoard::drawEx(int dstX, int dstY, int srcX, int srcY, int srcW, int srcH) const
{
    g_sdlDevice->fillRectangle(colorf::BLACK + colorf::A_SHF(255), dstX, dstY, srcW, srcH);
    Widget::drawEx(dstX, dstY, srcX, srcY, srcW, srcH);

    if(auto p = focusedChild()){
        auto drawSrcX = srcX;
        auto drawSrcY = srcY;
        auto drawSrcW = srcW;
        auto drawSrcH = srcH;
        auto drawDstX = dstX;
        auto drawDstY = dstY;

        if(mathf::cropChildROI(
                    &drawSrcX, &drawSrcY,
                    &drawSrcW, &drawSrcH,
                    &drawDstX, &drawDstY,

                    w(),
                    h(),

                    p->dx(),
                    p->dy() - m_itemSpace / 2,
                    w() - m_margin[1] - m_margin[3],
                    p->h() + m_itemSpace)){
            g_sdlDevice->fillRectangle(colorf::WHITE + colorf::A_SHF(128), drawDstX, drawDstY, drawSrcW, drawSrcH);
        }
    }
    g_sdlDevice->drawRectangle(colorf::WHITE + colorf::A_SHF(128), dstX, dstY, srcW, srcH);
}

bool MenuBoard::processEvent(const SDL_Event &event, bool valid)
{
    if(!valid){
        return consumeFocus(false);
    }

    if(!show()){
        return consumeFocus(false);
    }

    if(Widget::processEvent(event, valid)){
        if(event.type == SDL_MOUSEBUTTONDOWN){
            if(auto p = focusedChild()){
                if(m_onClickMenu){
                    m_onClickMenu(p);
                }

                setShow(false);
                setFocus(false);
            }
        }
        return true;
    }

    switch(event.type){
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            {
                const auto [eventX, eventY] = SDLDeviceHelper::getEventPLoc(event).value();
                if(!in(eventX, eventY)){
                    setFocus(false);
                    return event.type == SDL_MOUSEMOTION;
                }

                // event drops into margin
                // we should drop focus but still consume the event

                if(!mathf::pointInRectangle<int>(
                            eventX,
                            eventY,

                            x() + m_margin[2],
                            y() + m_margin[0],

                            w() - m_margin[2] - m_margin[3],
                            h() - m_margin[0] - m_margin[1])){
                    return !consumeFocus(false);
                }

                for(auto &p: m_childList){
                    if(mathf::pointInRectangle(eventX, eventY, p.widget->x(), p.widget->y() - m_itemSpace / 2, w() - m_margin[2] - m_margin[3], p.widget->h() + m_itemSpace)){
                        if(event.type == SDL_MOUSEBUTTONDOWN){
                            if(m_onClickMenu){
                                m_onClickMenu(p.widget);
                            }

                            setShow(false);
                            setFocus(false);
                        }
                        else{
                            consumeFocus(true, p.widget);
                        }
                        return true;
                    }
                }
                return false;
            }
        default:
            {
                return false;
            }
    }
}

Widget *MenuBoard::getSeparator()
{
    static Widget separator
    {
        DIR_UPLEFT,
        0,
        0,
    };
    return &separator;
}
