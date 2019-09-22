import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2

Pane {
    id: settingsPane

    ColumnLayout {
        id: settingsColumn
        anchors.fill: parent

        GroupBox {
            Layout.fillWidth: true
            Material.foreground: primaryColor
            Material.primary: primaryColor
            title: qsTr("Display")
            ColumnLayout {
                anchors.fill: parent
                Switch {
                    text: qsTr("Dark Mode")
                    font: theme.thinFont
                    onToggled: {
                        if (checked) {
                            darkMode = true
                        }
                        else {
                            darkMode = false
                        }
                    }
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
            }
        }
    }
}
