import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.9
import QtQuick.Controls.Material 2.2
import QtMultimedia 5.8

Pane {
    id: sendPane

    property int confirmations: 2
    property string confirmationsString: "20 minutes (2 blocks)"
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

        SpinBox {
            Layout.fillWidth: true
            id: amountSpinBox
            editable: true
            from: 0
            to: 100000000
            stepSize: 1000

            textFromValue: function(value, locale) {
                return value + " SAT"
            }
        }

        Row {
            spacing: 15
            Dial {
                id: feeDial
                snapMode: Dial.SnapAlways
                stepSize: 1.0 / 9
                onMoved: {
                    // TODO: Get these strings from C++
                    if (value <= 1.0 / 9) {
                        sendPane.confirmationsString = "20 minutes (2 blocks)"
                        sendPane.confirmations = 2
                    }
                    else if (value > 1.0 / 9 && value <= 1.0 / 9 * 2) {
                        sendPane.confirmationsString = "40 minutes (4 blocks)"
                        sendPane.confirmations = 4
                    }
                    else if (value > 1.0 / 9 * 2 && value <= 1.0 / 9 * 3) {
                        sendPane.confirmationsString = "60 minutes (6 blocks)"
                        sendPane.confirmations = 6
                    }
                    else if (value > 1.0 / 9 * 3 && value <= 1.0 / 9 * 4) {
                        sendPane.confirmationsString = "2 hours (12 blocks)"
                        sendPane.confirmations = 12
                    }
                    else if (value > 1.0 / 9 * 4 && value <= 1.0 / 9 * 5) {
                        sendPane.confirmationsString = "4 hours (24 blocks)"
                        sendPane.confirmations = 24
                    }
                    else if (value > 1.0 / 9 * 5 && value <= 1.0 / 9 * 6) {
                        sendPane.confirmationsString = "8 hours (48 blocks)"
                        sendPane.confirmations = 48
                    }
                    else if (value > 1.0 / 9 * 6 && value <= 1.0 / 9 * 7) {
                        sendPane.confirmationsString = "24 hours (144 blocks)"
                        sendPane.confirmations = 144
                    }
                    else if (value > 1.0 / 9 * 7 && value <= 1.0 / 9 * 8) {
                        sendPane.confirmationsString = "3 days (504 blocks)"
                        sendPane.confirmations = 504
                    }
                    else if (value > 1.0 / 9 * 8) {
                        sendPane.confirmationsString = "7 days (1008 blocks)"
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

        ToolBar {
            Material.elevation: 0
            Material.foreground: primaryColor
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("‹")
                    onClicked: {
                        camera.stop()
                        stackView.pop()
                    }
                    font: theme.thinFont
                }
                ToolButton {
                    text: qsTr("SEND")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        send(addressText.text, amountSpinBox.value, confirmations)
                    }
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
