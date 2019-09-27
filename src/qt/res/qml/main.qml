import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0

StackView {
    id: stackView

    property bool darkMode: false
    property string primaryColor: darkMode ? "white" : "black"
    property string secondaryColor: darkMode ? "black" : "white"

    anchors.fill: parent

    Material.accent: primaryColor
    Material.foreground: primaryColor
    Material.background: secondaryColor
    Material.primary: secondaryColor

    // Use this function to reuse translations from QtWidget contexts
    // For mobile-specific strings use the standard qsTr()
    function bitcoinTr(context, sourceText) {
        var translated = qsTranslate(context, sourceText)
        return translated
               //.replace(translated.match("\((.*?)\)"), "") // Remove text in brackets
               .replace("&", "") // Remove the underscores
    }

    function showInitMessage(msg) {
        initMessage.text = msg
    }

    function hideSplash() {
        stackView.replace(walletPane)
    }

    signal copyToClipboard(string clipboardText)
    signal changeUnit(int unit)

    FontLoader
    {
        id: robotoThin
        source: "qrc:/fonts/Roboto-Thin.ttf"
    }

    QtObject {
        id: theme
        property font thinFont: Qt.font({
                                            family: robotoThin.name,
                                            styleName: "Thin",
                                            pointSize: 12,
                                        })
    }

    ReceivePane {
        id: receivePane
        objectName: "receivePane"
        visible: false
    }

    SendPane {
        id: sendPane
        objectName: "sendPane"
        visible: false
    }

    AboutPane {
        id: aboutPane
        objectName: "aboutPane"
        visible: false
    }

    SettingsPane {
        id: settingsPane
        objectName: "settingsPane"
        visible: false
    }

    ConsolePane {
        id: consolePane
        objectName: "consolePane"
        visible: false
    }

    WalletPane {
        id: walletPane
        objectName: "walletPane"
    }

    initialItem: Pane {
        id: pane

        Text {
            id: bitcoinCoreText
            text: "Bitcoin<b>Core</b>"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: 20
            font.family: robotoThin.name
            font.styleName: "Thin"
            font.pointSize: 30
            color: primaryColor
        }

        Image {
            id: bitcoinLogo
            width: 100
            height: 100
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            source: "image://icons/window"
        }

        Colorize {
            anchors.fill: bitcoinLogo
            source: bitcoinLogo
            saturation: 0.0
        }

        BusyIndicator {
            id: busyIndication
            width: 140
            height: 140
            anchors.centerIn: parent
        }

        Text {
            id: initMessage
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: 10
            font: theme.thinFont
            color: primaryColor
        }
    }
}

