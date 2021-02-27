import QtQuick 2.15
import "../Constants"

Rectangle {
    property alias orientation: gradient.orientation

    property alias start_pos: g_start.position
    property alias end_pos: g_end.position

    property color start_color: Style.colorGradient1
    property color end_color: Style.colorGradient2
    Behavior on start_color { ColorAnimation { duration: Style.animationDuration } }
    Behavior on end_color { ColorAnimation { duration: Style.animationDuration } }

    gradient: Gradient {
        id: gradient
        orientation: Qt.Horizontal

        GradientStop {
            id: g_start
            position: 0.0
            color: start_color
        }

        GradientStop {
            id: g_end
            position: 1.0
            color: end_color
        }
    }
}
