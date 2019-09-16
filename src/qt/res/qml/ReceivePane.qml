import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2


Pane {
    id: receivePane

    property alias qrSource: qrImage.source

    ColumnLayout {
        id: qrColumn
        anchors.fill: parent

        Column {
            Layout.fillWidth: true
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                id: qrImage
                Layout.preferredWidth: parent.parent.width / 6 * 5
                Layout.preferredHeight: parent.parent.width / 6 * 5
            }

            Text {
                anchors.horizontalCenter: qrImage.horizontalCenter
                font: theme.thinFont
                text: qsTr("Tap to copy to clipboard")
                horizontalAlignment: Text.AlignHCenter

            }
        }

        ToolBar {
            Material.elevation: 0
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
