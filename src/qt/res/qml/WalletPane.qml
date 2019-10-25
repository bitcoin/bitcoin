import QtQuick 2.12
import QtQml 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

Pane {
    function updateBalance(formattedBalance) {
        balanceText.text = formattedBalance
    }

    function showQr(address) {
        receivePane.address = address
        stackView.push(receivePane)
    }

    signal request()

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Text {
            id: balanceText
            horizontalAlignment: Text.AlignHCenter
            font.family: robotoThin.name
            font.styleName: "Thin"
            font.pointSize: 25
            Layout.fillWidth: true
            Layout.topMargin: 10
            color: primaryColor
        }

        ListView {
            id: transactionListView
            objectName: "transactionListView"

            currentIndex: -1

            model: transactionsModel

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 10

            clip: true
            snapMode: ListView.SnapToItem

            delegate: RowLayout {
                width: parent.width
                Image {
                    id: transactionIcon
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40
                    source: model.type === 4 ? "qrc:/icons/tx_input" : "qrc:/icons/tx_output"

                    Rectangle {
                        id: whiteRectangle
                        anchors.fill: transactionIcon
                        visible: false
                        color: "white"
                    }

                    OpacityMask {
                        anchors.fill: transactionIcon
                        source: whiteRectangle
                        maskSource: transactionIcon
                        enabled: darkMode ? true : false
                        visible: darkMode ? true : false
                    }
                }

                Text {
                    text: model.date.toLocaleString(Qt.locale(), "dd.MM.yy. hh:mm")
                    font: theme.thinFont
                    color: primaryColor
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            transactionInfoDialog.transactionInfoString = model.longdescription
                            transactionInfoDialog.open()
                        }
                    }
                }
                Text {
                    text: "<b>" + (model.type === 4 ? "+" : "") + model.formattedamount
                    font: theme.thinFont
                    color: primaryColor
                }
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        Dialog {
            id: transactionInfoDialog

            property string transactionInfoString

            Material.background: secondaryColor

            modal: true
            focus: true
            x: (parent.width - width) / 2
            y: parent.height / 15
            width: Math.min(parent.width, parent.height) / 8 * 7
            contentHeight: transactionInfoText.height

            Text {
                id: transactionInfoText
                width: transactionInfoDialog.availableWidth
                text: transactionInfoDialog.transactionInfoString.slice(0, -18) // Get rid of the last newline
                wrapMode: Text.Wrap
                font: theme.thinFont
                color: primaryColor
            }
        }

        ToolBar {
            Material.elevation: 0
            Material.foreground: primaryColor // TODO: investigate why these are not propagated from main
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: talkcoinTr("TalkcoinGUI","&Send")
                    onClicked: {
                        sendPane.camera.start()
                        stackView.push(sendPane)
                    }
                    font: theme.thinFont
                }
                ToolButton {
                    text: talkcoinTr("TalkcoinGUI","&Receive")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: request()
                    font: theme.thinFont
                }

                ToolButton {
                    text: "⋮"
                    Layout.alignment: Qt.AlignRight

                    onClicked: optionsMenu.open()

                    Menu {
                        Material.foreground: primaryColor
                        Material.background: secondaryColor

                        id: optionsMenu
                        x: parent.width - width
                        transformOrigin: Menu.TopRight

                        MenuItem {
                            text: talkcoinTr("TalkcoinGUI", "&Settings")
                            font: theme.thinFont
                            onTriggered: {
                                stackView.push(settingsPane)
                            }
                        }

                        MenuItem {
                            text: talkcoinTr("TalkcoinGUI", "&About %1")
                            font: theme.thinFont
                            onTriggered: {
                                stackView.push(aboutPane)
                            }
                        }

                        MenuItem {
                            text: talkcoinTr("RPCConsole", "&Console")
                            font: theme.thinFont
                            onTriggered: {
                                stackView.push(consolePane)
                            }
                        }
                    }
                }
            }
        }
    }
}
