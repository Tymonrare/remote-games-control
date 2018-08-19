import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Window 2.1

Item {
    id: gameCard
    property string title
    property int dialStep;
    property int minTime;
    property int maxTime;
    property int gameTime;
    width: 460
    height: 215

    function setTitle(name, hide){
        title = name;
        nameLabel.text = name;
        if(hide)
            nameLabel.opacity = 0;
    }

    function setTime(time, price, step, min, max){
        dialStep = step;
        minTime = min;
        maxTime = max;

        dial.from = min;
        dial.to = max;
        dial.stepSize = step;
        dial.value = time;
        gameTime = time;

        var message;
        if(time%60 < 10)
        message = "%1:0%2"
        else
        message = "%1:%2"
        var mint = time/60;
        var maxt = time%60;
        label.text = message.arg(Math.floor(mint)).arg(Math.floor(maxt));
        if(dial.enabled)
            priceLabel.text = "" + price + "р/мин"
        else
            priceLabel.text = "" + price + "р/" + time/60 + "мин"
    }

    function setImage(path){
        image.source = "file:" + path;
    }

    function disableEditing(){
        dial.enabled = false;
    }

    Dial {
        id: dial
        width: 218
        height: 212
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.verticalCenterOffset: 1
        scale: 1
        anchors.verticalCenter: parent.verticalCenter
        stepSize: 30
        to: 599
        from: 60
        value: 60
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
            text: qsTr("6:00")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.topMargin: 45
            anchors.top: parent.top
            textFormat: Text.PlainText
            font.family: "Times New Roman"
            font.bold: true
            font.pointSize: 44
            color: "#FFFFFF"
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        onAngleChanged: {
            var angle = (dial.angle+140)/280*(maxTime-minTime);
            angle += minTime - angle%dialStep;
            label.text = testClientForm.secondsToFormated(angle);
            gameTime = angle;
        }
        onValueChanged: {
            //var angle = (dial.angle+140)/280*(maxTime-minTime);
            //angle += - angle%dialStep;
            //dial.value = angle;
        }
    }
    Button {
        id: control
        x: 139
        y: 29
        width: 399
        height: 158
        anchors.verticalCenterOffset: 0
        anchors.right: parent.right
        anchors.rightMargin: -78
        anchors.verticalCenter: parent.verticalCenter
        scale: 0.5


        background: Image{
            x: 0
            y: 0
            width: 399
            height: 158
            source: "play-pressed.png"
        }
        onClicked: testClientForm.startGame(gameTime, 0, title)
    }

    Image {
        id: image
        anchors.fill: parent
        sourceSize.width: -1
        fillMode: Image.PreserveAspectCrop
        source: "gamepad.jpg"
        opacity: 1
        state: "RELEASED"

        states: [
            State {
                name: "PRESSED"
                PropertyChanges { target: image; opacity: 0.0}
            },
            State {
                name: "RELEASED"
                PropertyChanges { target: image; opacity: 1.0}
            }
        ]

        transitions: [
        Transition {
            from: "PRESSED"
            to: "RELEASED"
            NumberAnimation {property: "opacity"; target: image; duration: 300}
        },
        Transition {
            from: "RELEASED"
            to: "PRESSED"
            NumberAnimation {property: "opacity"; target: image; duration: 300}
        }
        ]
        Text {
            id: nameLabel
            x: 0
            y: 68
            width: 292
            height: 82
            text: qsTr("game Name one")
            anchors.verticalCenterOffset: -39
            anchors.horizontalCenterOffset: 0
            font.family: "Arial"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.NoWrap
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pointSize: 80
            style: Text.Outline
            styleColor: "#000000"
            color: "#FFFFFF"
        }
        MouseArea {
            id: mouseArea1
            anchors.fill: parent
            hoverEnabled: true
            onEntered: image.state = "PRESSED"
            onExited: image.state = "RELEASED"

            propagateComposedEvents: true

            onClicked: mouse.accepted = false;
            onPressed: mouse.accepted = false;
            onReleased: mouse.accepted = false;
            onDoubleClicked: mouse.accepted = false;
            onPositionChanged: mouse.accepted = false;
            onPressAndHold: mouse.accepted = false;
        }

        Text {
            id: priceLabel
            x: 1
            y: 75
            width: 292
            height: 82
            color: "#ffffff"
            text: qsTr("100р/мин")
            anchors.horizontalCenter: parent.horizontalCenter
            styleColor: "#000000"
            anchors.verticalCenterOffset: 66
            font.family: "Arial"
            anchors.horizontalCenterOffset: 0
            anchors.verticalCenter: parent.verticalCenter
            wrapMode: Text.NoWrap
            verticalAlignment: Text.AlignVCenter
            style: Text.Outline
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.FixedSize
            font.pointSize: 20
        }
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
}
