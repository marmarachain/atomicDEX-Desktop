import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Constants"

DefaultModal {
    id: root

    padding: 10

    width: 900
    height: column_layout.height + verticalPadding * 2

    property alias currentIndex: stack_layout.currentIndex
    property int targetPageIndex: currentIndex
    property alias count: stack_layout.count
    default property alias pages: stack_layout.data

    function nextPage() {
        if(currentIndex === count - 1) root.close()
        else {
            targetPageIndex = currentIndex + 1
            page_change_animation.start()
        }
    }

    function previousPage() {
        if(currentIndex === 0) root.close()
        else {
            targetPageIndex = currentIndex - 1
            page_change_animation.start()
        }
    }

    onOpened: {
        stack_layout.opacity = 1
    }

    SequentialAnimation {
        id: page_change_animation
        NumberAnimation { id: fade_out; target: root; property: "opacity"; to: 0; duration: Style.animationDuration }
        PropertyAction { target: root; property: "currentIndex"; value: targetPageIndex }
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: fade_out.duration }
    }

    Column {
        id: column_layout
        spacing: Style.rowSpacing
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter

        Row {
            id: page_indicator
            visible: root.count > 1
            anchors.horizontalCenter: parent.horizontalCenter

            layoutDirection: Qt.RightToLeft

            Repeater {
                model: root.count
                delegate: Item {
                    id: bundle
                    property color color: root.currentIndex >= (root.count-1 - index) ? Style.colorGreen : Style.colorTheme5
                    width: (index === root.count-1 ? 0 : circle.width*2) + circle.width*0.5
                    height: circle.height * 1.5
                    InnerBackground {
                        id: rectangle
                        height: circle.height/4
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: -circle.width*0.5
                        anchors.right: circle.horizontalCenter
                        color: bundle.color
                    }

                    FloatingBackground {
                        id: circle
                        width: 24
                        height: width
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        color: bundle.color
                    }
                }
            }
        }

        // Inside modal
        StackLayout {
            id: stack_layout
            width: parent.width
            height: stack_layout.children[stack_layout.currentIndex].height
        }
    }
}
