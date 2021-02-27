import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import "../Constants"
import "../Components"

BasicModal {
    id: root

    property string message

    function onFatalNotification(msg) {
        console.debug("Fatal error: " + msg)
        if (API.app.wallet_mgr.initial_loading_status !== "None") {
            API.app.disconnect()
            app.current_page = app.idx_login
            message = msg
            open()
        }
    }

    width: 900

    ModalContent {
        title: qsTr("Fatal Error")

        DefaultText {
            text: message === "connection dropped" ? qsTr("Connection has been lost. You have been disconnected.") :
                                                     qsTr("An error occurred. You have been disconnected.")
        }

        footer: [
            DefaultButton {
                Layout.fillWidth: true
                text: qsTr("Close")
                onClicked: root.close()
            }
        ]
    }

    Component.onCompleted: {
        API.app.notification_mgr.fatalNotification.connect(onFatalNotification)
        console.debug("FatalErrorModal Constructed")
    }

    Component.onDestruction: {
        API.app.notification_mgr.fatalNotification.disconnect(onFatalNotification)
        console.debug("FatalErrorModal Destructed")
    }
}
