import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2
import QtMultimedia 5.8

Pane {
    id: sendPane

    property int confirmations: 2
    property string confirmationsString: confirmationTargets[confirmations.toString()]
    property alias camera: camera

    signal send(string address, int amount, int confirmations)

    ColumnLayout {
        id: sendColumn
        spacing: 10

        anchors.fill: parent

        Text {
            id: scannerText
            Layout.fillWidth: true

            font: theme.thinFont
            color: primaryColor
            text: qsTr("Scan QR Code")
            horizontalAlignment: Text.AlignHCenter

        }

        VideoOutput {
            id: viewfinderOutput

            source: camera
            autoOrientation: true
            Layout.fillWidth: true
            Layout.preferredHeight: width
            Layout.alignment: Qt.AlignHCenter

            fillMode: VideoOutput.PreserveAspectCrop

            Camera {
                id: camera
                cameraState: Camera.UnloadedState
            }

            Rectangle {
                id: captureZone
                color: "red"
                opacity: 0.2
                width: parent.width * 0.8
                height: width
                anchors.centerIn: parent
                radius: 20
            }
        }

        TextArea {
            id: addressText
            Layout.fillWidth: true
            placeholderText: qsTr("...or enter the address here")
            font: theme.thinFont
            color: primaryColor
        }

        TextField {
            id: amountField
            Layout.fillWidth: true
            placeholderText: qsTr("Amount in ") // + display unit
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: 0
                decimals: 8
            }
            font: theme.thinFont
            color: primaryColor
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
                        camera.stop()
                        stackView.pop()
                    }
                    font: theme.thinFont
                }
                ToolButton {
                    text: bitcoinTr("BitcoinGUI", "&Send")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        send(addressText.text, amountSpinBox.value, confirmations)
                    }
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

                        contentItem: Row {
                            id: feeRow
                            spacing: 15
                            Dial {
                                id: feeDial
                                Material.accent: primaryColor
                                snapMode: Dial.SnapAlways
                                stepSize: 1.0 / 8
                                onMoved: {
                                    if (value === 0) {
                                        sendPane.confirmations = 2
                                    }
                                    else if (value === stepSize) {
                                        sendPane.confirmations = 4
                                    }
                                    else if (value === stepSize * 2) {
                                        sendPane.confirmations = 6
                                    }
                                    else if (value === stepSize * 3) {
                                        sendPane.confirmations = 12
                                    }
                                    else if (value === stepSize * 4) {
                                        sendPane.confirmations = 24
                                    }
                                    else if (value === stepSize * 5) {
                                        sendPane.confirmations = 48
                                    }
                                    else if (value === stepSize * 6) {
                                        sendPane.confirmations = 144
                                    }
                                    else if (value === stepSize * 7) {
                                        sendPane.confirmations = 504
                                    }
                                    else if (value === 1) {
                                        sendPane.confirmations = 1008
                                    }
                                }
                            }

                            Text {
                                id: feeExplainer
                                font: theme.thinFont
                                color: primaryColor
                                text: qsTr("Transaction should confirm in ") + sendPane.confirmationsString
                                width: 180
                                wrapMode: Text.WordWrap
                                anchors.verticalCenter: feeDial.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
