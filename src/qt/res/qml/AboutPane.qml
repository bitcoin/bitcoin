import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2

Pane {
    id: aboutPane

    ColumnLayout {
        id: aboutColumn
        anchors.fill: parent

        Column {
            Layout.fillWidth: true

            Text {
                id: bitcoinCoreText

                anchors.horizontalCenter: parent.horizontalCenter

                text: "Bitcoin<b>Core</b>"
                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 30
                color: primaryColor
            }


            Text {
                id: versionText

                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter

                text: version

                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 10

                color: primaryColor
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: licenceText.width
            contentHeight: licenceText.height

            clip: true

            Text {
                id: licenceText

                width: parent.parent.width

                text: licenceInfo
                font: theme.thinFont
                color: primaryColor
                wrapMode: Text.Wrap
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

