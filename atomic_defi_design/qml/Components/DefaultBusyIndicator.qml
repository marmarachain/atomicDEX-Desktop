import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import "../Constants"

BusyIndicator {
    id: control

    Universal.theme: Style.dark_theme ? Universal.Dark : Universal.Light
    Universal.accent: Style.colorQtThemeAccent
    Universal.foreground: Style.colorQtThemeForeground
    Universal.background: Style.colorQtThemeBackground

    implicitWidth: 48
    implicitHeight: 48
}
