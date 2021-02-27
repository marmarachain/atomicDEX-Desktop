import QtQuick 2.15
import "../Constants"

AnimatedRectangle {
    property string type
    radius: 20

    height: type_tag.font.pixelSize * 1.5
    width: type_tag.width + 8

    color: Style.getCoinTypeColor(model.type)

    DefaultText {
        id: type_tag
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        text: type
        font.pixelSize: Style.textSizeSmall1
        color: Style.colorWhite1
    }
}
