#include <cstdlib>
#include "engine/except/runtime-error.hpp"
#include "engine/platform/logger.hpp"
#include "xcb-window.hpp"

vtest::WSIWindowEvent vtest::XCBWindow::wsiWindowEvent_(xcb_key_press_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};
    wsi_event.kind = WSIWindowEvent::KEY_PRESS;
    wsi_event.rootWindow = xcb_event->root;
    wsi_event.eventWindow = xcb_event->event;
    wsi_event.childWindow = xcb_event->child;
    wsi_event.eventDetail = xcb_event->detail;

    return wsi_event;
}

vtest::WSIWindowEvent vtest::XCBWindow::wsiWindowEvent_(xcb_button_press_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};
    wsi_event.kind = vtest::WSIWindowEvent::BUTTON_PRESS;
    wsi_event.rootWindow = xcb_event->root;
    wsi_event.eventWindow = xcb_event->event;
    wsi_event.childWindow = xcb_event->child;

    return wsi_event;
}

vtest::WSIWindowEvent vtest::XCBWindow::wsiWindowEvent_(xcb_client_message_event_t * xcb_event) {
    WSIWindowEvent wsi_event {};

    switch (xcb_event->type) {
        case 256:
            wsi_event.kind = WSIWindowEvent::CLOSE_BUTTON_PRESS;
            break;

        default:
            break;
    }

    wsi_event.eventWindow = xcb_event->window;

    return wsi_event;
}

vtest::XCBWindow::XCBWindow() {
    m_WindowIds = new std::list<uint32_t>();
    m_connection = xcb_connect(nullptr, nullptr);

    int xcb_error = xcb_connection_has_error(m_connection);

    if(m_connection == nullptr || xcb_error >= 1) {
        throw vtrs::RuntimeError("Unable to initialise XCB connection.", vtrs::RuntimeError::E_TYPE_XCB_CLIENT, xcb_error);
    }

    /* Obtain setup info and access the screen */
    m_setup = xcb_get_setup(m_connection);
    m_screen = xcb_setup_roots_iterator(m_setup).data;

    if (m_screen == nullptr) {
        throw vtrs::RuntimeError("Unable to access XCB screen.", vtrs::RuntimeError::E_TYPE_XCB_CLIENT);
    }

    xcb_intern_atom_cookie_t protocol_cookie = xcb_intern_atom(m_connection, 1, 12,"WM_PROTOCOLS");
    m_protocolReply  = xcb_intern_atom_reply(m_connection, protocol_cookie, nullptr);

    xcb_intern_atom_cookie_t window_cookie = xcb_intern_atom(m_connection, 0, 16,"WM_DELETE_WINDOW");
    m_windowReply  = xcb_intern_atom_reply(m_connection, window_cookie, nullptr);
}

void vtest::XCBWindow::createWindow(int width, int height) {
    xcb_window_t window_id = xcb_generate_id(m_connection);
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t value_list[2];
    value_list[0] = m_screen->white_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS;

    xcb_create_window(m_connection,
                      m_screen->root_depth,
                      window_id,
                      m_screen->root, 0, 0,
                      width, height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      m_screen->root_visual, value_mask, value_list);

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, window_id, m_protocolReply->atom, 4, 32, 1, &m_windowReply->atom);

    xcb_map_window(m_connection, window_id);
    xcb_flush(m_connection);

    m_WindowIds->push_back(window_id);
}

vtest::XCBWindow::~XCBWindow() {
    for(uint32_t& m_WindowId : *m_WindowIds) {
        xcb_destroy_window(m_connection, m_WindowId);
    }

    xcb_disconnect(m_connection);
}

vtest::WSIWindowEvent vtest::XCBWindow::pollEvents() {
    xcb_generic_event_t* xcb_event = xcb_poll_for_event(m_connection);
    WSIWindowEvent wsi_event;

    if (xcb_event == nullptr) {
        return wsi_event;
    }

    switch (xcb_event->response_type & ~0x80) {
        case XCB_KEY_PRESS:
            wsi_event = wsiWindowEvent_(reinterpret_cast<xcb_key_press_event_t*>(xcb_event));
            break;

        case XCB_BUTTON_PRESS:
            wsi_event = wsiWindowEvent_(reinterpret_cast<xcb_button_press_event_t*>(xcb_event));
            break;

        case XCB_CLIENT_MESSAGE:
            wsi_event = wsiWindowEvent_(reinterpret_cast<xcb_client_message_event_t*>(xcb_event));
            break;

        default:
            break;
    }

    free(xcb_event);

    if (wsi_event.kind == WSIWindowEvent::CLOSE_BUTTON_PRESS) {
        xcb_destroy_window(m_connection, wsi_event.eventWindow);
        m_WindowIds->remove(wsi_event.eventWindow);
    }

    return wsi_event;
}