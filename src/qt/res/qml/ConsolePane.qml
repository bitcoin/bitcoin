import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2

Pane {
    id: consolePane

    signal executeCommand(string commandText)

    function showReply(msg) {
        consoleText.text = msg
        consoleFlickable.contentY = consoleFlickable.contentHeight - consoleFlickable.height
    }

    ColumnLayout {
        id: consoleColumn
        anchors.fill: parent

        Flickable {
            id: consoleFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: consoleText.width
            contentHeight: consoleText.height

            clip: true

            TextArea {
                id: consoleText

                width: parent.parent.width

                text: bitcoinTr("RPCConsole", "WARNING: Scammers have been active, telling users to type commands here, stealing their wallet contents. Do not use this console without fully understanding the ramifications of a command.")

                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 10

                color: primaryColor

                wrapMode: Text.Wrap
                readOnly: true
                selectByMouse: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: ">"
                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 12
                font.bold: true
            }

            TextField {
                id: consoleInput
                Layout.fillWidth: true

                focus: true

                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 12
                font.bold: true
                color: primaryColor

                onAccepted: {
                    executeCommand(text);
                    text = ""
                }

            }
        }


        ToolBar {
            Material.elevation: 0
            Material.foreground: primaryColor
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: "‹"
                    onClicked: {
                        stackView.pop()
                    }
                    font: theme.thinFont
                }
            }
        }
    }
}


