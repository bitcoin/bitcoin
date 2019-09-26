import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2


Pane {
    id: receivePane

    property string address

    ColumnLayout {
        id: qrColumn
        anchors.fill: parent

        Column {
            Layout.fillWidth: true
            Image {
                id: qrImage

                source: "image://qr/bitcoin:" + address

                anchors.horizontalCenter: parent.horizontalCenter
                Layout.preferredWidth: parent.parent.width / 6 * 5
                Layout.preferredHeight: parent.parent.width / 6 * 5

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        textAnimation.running = true
                        clipboardText.text = qsTranslate("","Address copied to clipboard")
                        copyToClipboard(address)
                    }
                }
            }

            Text {
                id: clipboardText
                anchors.horizontalCenter: qrImage.horizontalCenter
                font: theme.thinFont
                color: primaryColor
                text: qsTranslate("","Tap to copy to clipboard")
                horizontalAlignment: Text.AlignHCenter

                NumberAnimation {
                    id: textAnimation
                    target: clipboardText
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 1000
                    easing.type: Easing.bezierCurve
                }
            }
        }

        ToolBar {
            Material.elevation: 0
            Material.foreground: primaryColor

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom

            RowLayout {
                anchors.fill: parent
                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("‹")
                    onClicked: {
                        stackView.pop()
                    }
                    font: theme.thinFont
                }
                ToolButton {
                    text: qsTr("SHARE")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {}
                    font: theme.thinFont
                }
                ToolButton {
                    text: qsTr("⋮")
                    Layout.alignment: Qt.AlignRight
                }
            }
        }
    }
}
