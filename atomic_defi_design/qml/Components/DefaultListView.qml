import QtQuick 2.15
import QtQuick.Controls 2.15

import "../Constants"

ListView {
    id: root

    property bool scrollbar_visible: contentItem.childrenRect.height > height
    readonly property double scrollbar_margin: scrollbar_visible ? 8 : 0

    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: DefaultScrollBar { }

    implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentItem.childrenRect.height

    clip: true

    // Opacity animation
    opacity: root.count === 0 ? 0 : enabled ? 1 : 0.2
    Behavior on opacity { SmoothedAnimation { duration: Style.animationDuration * 0.5; velocity: -1 } }
}
