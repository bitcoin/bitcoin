import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

Pane {
    id: settingsPane

    ColumnLayout {
        id: settingsColumn
        anchors.fill: parent

        GroupBox {
            Layout.fillWidth: true
            Material.foreground: primaryColor
            Material.primary: primaryColor
            title: bitcointalkcoinTr("OptionsDialog", "&Display")
            ColumnLayout {
                anchors.fill: parent

                RowLayout {
                    spacing: 10

                    Label {
                        text: bitcointalkcoinTr("OptionsDialog", "&Unit to show amounts in:")
                        font: theme.thinFont
                    }

                    ComboBox {
                        Material.foreground: primaryColor
                        model: availableUnits

                        onActivated: {
                            changeUnit(index)
                        }

                        font: theme.thinFont
                    }
                }

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
