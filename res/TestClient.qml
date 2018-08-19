import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Window 2.1

Window {
    id: testClientForm
    width: 1280
    height: 720
    visible: true
    color: "#010101"
    visibility:"FullScreen"

    signal play(var time)
    signal stop()

    Button {
        id: control
        x: 825
        y: 281
        width: 399
        height: 158
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 56


        background: Image{
            x: 0
            y: 0
            width: 399
            height: 158
            source: "play-pressed.png"
        }
        onClicked: testClientForm.play(dial.value)
    }

    Button {
        id: button
        x: 517
        y: 436
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 38
        background: Image{
            x: 0
            y: 0
            width: 247
            height: 246
            source: "stop-pressed.png"
        }
         onClicked: testClientForm.stop()
    }

    Connections {
        target: control
        onPressed: {
            control.background.source = "play-unpressed.png"
            control.background.scale = 0.9;
        }
    }

    Connections {
        target: control
        onReleased:  {
            control.background.source = "play-pressed.png"
            control.background.scale = 1;
        }
    }

    Connections {
        target: button
        onPressed: {
            button.background.source = "stop-unpressed.png"
            button.background.scale = 0.9;
        }
    }

    Connections {
        target: button
        onReleased:  {
            button.background.source = "stop-pressed.png"
            button.background.scale = 1;
        }
    }

    Dial {
        id: dial
        y: 245
        width: 230
        height: 230
        anchors.verticalCenter: parent.verticalCenter
        stepSize: 30
        to: 599
        from: 60
        value: 60
        anchors.left: parent.left
        anchors.leftMargin: 75
        background: Rectangle {
                x: dial.width / 2 - width / 2
                y: dial.height / 2 - height / 2
                width: Math.max(64, Math.min(dial.width, dial.height))
                height: width
                color: "#111111"
                radius: width / 2
                border.color: "#ffffff"
                border.width: 5
                opacity: dial.enabled ? 1 : 0.3
            }

            handle: Rectangle {
                id: handleItem
                x: dial.background.x + dial.background.width / 2 - width / 2
                y: dial.background.y + dial.background.height / 2 - height / 2
                width: 16
                height: 16
                color: "#a4b5e0"
                radius: 8
                border.color: "#000000"
                antialiasing: true
                opacity: dial.enabled ? 1 : 0.3
                transform: [
                    Translate {
                        y: -Math.min(dial.background.width, dial.background.height) * 0.4 + handleItem.height / 2
                    },
                    Rotation {
                        angle: dial.angle
                        origin.x: handleItem.width / 2
                        origin.y: handleItem.height / 2
                    }
                ]
            }

            Label {
                id: label
                x: 39
                y: 67
                text: qsTr("6:00")
                textFormat: Text.PlainText
                font.family: "Times New Roman"
                font.bold: true
                font.pointSize: 64
                color: "#FFFFFF"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            onAngleChanged: {
                var angle = (140 + dial.angle)/280*(dial.to-dial.from)+60;
                angle -= angle%30;
                var message;
                if(angle%60 < 10)
                    message = "%1:0%2"
                else
                    message = "%1:%2"
                var min = angle/60;
                var max = angle%60;
                label.text = message.arg(Math.floor(min)).arg(Math.floor(max));
            }
            onValueChanged: {
                var angle = (140 + dial.angle)/280*(dial.to-dial.from)+60;
                angle -= angle%30;
                dial.value = angle;
                var min = angle/60;
                var max = angle%60;
                label.text = message.arg(Math.floor(min)).arg(Math.floor(max));
            }
    }

    Rectangle {
        id: rectangle
        width: testClientForm.width
        height: testClientForm.height
        color: "#000000"
        z: -1
    }

    Image {
        id: image
        x: 179
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 19
        source: "title.png"
    }
}
