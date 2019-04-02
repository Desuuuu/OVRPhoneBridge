DEFINES += APP_VERSION=\\\"$$VERSION\\\"

FORMS += src/widgets/main_widget.ui \
    src/widgets/tabs/notifications_tab_widget.ui \
    src/widgets/tabs/sms_tab_widget.ui \
    src/widgets/tabs/device_tab_widget.ui \
    src/widgets/tabs/settings_tab_widget.ui

SOURCES += src/main.cpp \
    src/crypto.cpp \
    src/server.cpp \
    src/bridge.cpp \
    src/vr_overlay_controller.cpp \
    src/widgets/fade_widget.cpp \
    src/widgets/focus_line_edit.cpp \
    src/widgets/focus_plain_text_edit.cpp \
    src/widgets/highlight_button.cpp \
    src/widgets/vertical_scroll_area.cpp \
    src/widgets/main_widget.cpp \
    src/widgets/notification_widget.cpp \
    src/widgets/sms_widget.cpp \
    src/widgets/short_sms_widget.cpp \
    src/widgets/tabs/notifications_tab_widget.cpp \
    src/widgets/tabs/sms_tab_widget.cpp \
    src/widgets/tabs/device_tab_widget.cpp \
    src/widgets/tabs/settings_tab_widget.cpp

HEADERS += src/common.h \
    src/crypto.h \
    src/server.h \
    src/bridge.h \
    src/vr_overlay_controller.h \
    src/widgets/fade_widget.h \
    src/widgets/focus_line_edit.h \
    src/widgets/focus_plain_text_edit.h \
    src/widgets/highlight_button.h \
    src/widgets/vertical_scroll_area.h \
    src/widgets/main_widget.h \
    src/widgets/notification_widget.h \
    src/widgets/sms_widget.h \
    src/widgets/short_sms_widget.h \
    src/widgets/tabs/notifications_tab_widget.h \
    src/widgets/tabs/sms_tab_widget.h \
    src/widgets/tabs/device_tab_widget.h \
    src/widgets/tabs/settings_tab_widget.h
