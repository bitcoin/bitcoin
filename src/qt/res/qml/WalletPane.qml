import QtQuick 2.9
import QtQml 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.9

Pane {
    function updateBalance(formattedBalance) {
        balanceText.text = formattedBalance
    }

    function showQr(address) {
        receivePane.qrSource = "image://qr/bitcoin:" + address
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
        }

        ListView {
            id: transactionListView
            objectName: "transactionListView"

            currentIndex: -1

            model: transactionsModel

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 10

            delegate: RowLayout {
                width: parent.width
                Image {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40
                    source: model.type === 4 ? "qrc:/icons/tx_input" : "qrc:/icons/tx_output"
                }
                Text {
                    text: model.date.toLocaleString(Qt.locale(), "dd.MM.yy. hh:mm")
                    font: theme.thinFont
                }
                Text {
                    text: "<b>" + (model.type === 4 ? "+" : "") + model.formattedamount
                    font: theme.thinFont
                }
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        ToolBar {
            Material.elevation: 0
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("SEND")
                    onClicked: {
                        stackView.push(sendPane)
                    }
                    font: theme.thinFont
                }
                ToolButton {
                    text: qsTr("RECEIVE")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: request()
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
