QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/deploy/qgroundcontrol-start.sh $$DESTDIR
QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/deploy/qgroundcontrol.desktop $$DESTDIR
exists($$PWD/create_ABLUO) {
    QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/res/Images/CustomAppIcon_ABLUO.png $$DESTDIR
} else {
    QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/res/Images/CustomAppIcon_UP.png $$DESTDIR
}
