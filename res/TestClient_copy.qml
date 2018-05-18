import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Window 2.1

Window {
    id: testClientForm
    objectName: "lalalal"
    width: 1280
    height: 720
    visible: true
    color: "#010101"
    visibility:"FullScreen"

    property int totalGames;
    property int gameTime;
    property bool inGame : false;

    Component.onCompleted: {
        grid.scale = width/(width-width%460);
    }
    signal play(var time, var server, var game)
    signal stop()

    function addNewGame(name, defaultTime, noneditable, price, timeStep, minTime, maxTime, image, hideName) {
        var component = Qt.createComponent("GameCard.qml");
        if (component.status !== Component.Ready) return;
            var inst = component.createObject(grid);
            inst.setTitle(name, hideName);
            if(noneditable){
                inst.disableEditing();
            }
            inst.setTime(defaultTime, price, timeStep, minTime, maxTime);
            if(image !== "") inst.setImage(image);
            totalGames += 1;
            gridScroll.contentHeight = Math.ceil((totalGames*460*grid.scale)/width+1)*215;
    }

    function secondsToFormated(time){
            var message;
            if(time%60 < 10)
                message = "%1:0%2"
            else
                message = "%1:%2"
            var min = time/60;
            var max = time%60;
            return message.arg(Math.floor(min)).arg(Math.floor(max));
    }

    function startGame(time, server, game){
        play(time, server, game);
        timerText.text = secondsToFormated(time);
        gameTime = time;
        inGame = true;
    }

    function stopGame(){
       stop();
       gameTime = 0;
       timerText.text = secondsToFormated(gameTime);
        inGame = false;
    }

    function updateGameTime(){
        if(!inGame || gameTime <= 0) return;
        gameTime -= 1;
        timerText.text = secondsToFormated(gameTime);
    }

    Item {
    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: updateGameTime();
    }
    }

    Button {
        id: button
        x: 517
        y: 505
        anchors.horizontalCenterOffset: 1
        scale: 0.5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -31
        background: Image{
            x: 0
            y: 0
            width: 247
            height: 246
            source: "stop-pressed.png"
        }
        onClicked: testClientForm.stopGame()
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

    Rectangle {
        id: rectangle
        width: testClientForm.width
        height: testClientForm.height
        color: "#000000"
        z: -1
    }
    Flickable {
        id:gridScroll
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 159
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        contentHeight: grid.height
        contentWidth: grid.width
        ScrollBar.vertical: ScrollBar { }
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Grid {
            id: grid
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.top: parent.top
            transformOrigin: Item.TopLeft
            scale: 1
        }
    }

    Image {
        id: image1
        x: 733
        y: 579
        width: 547
        height: 100
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 41
        fillMode: Image.PreserveAspectFit
        source: "logo.jpg"
    }

    Text {
        id: timerText
        y: 593
        color: "#ffffff"
        text: qsTr("00:00")
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 55
        anchors.left: parent.left
        anchors.leftMargin: 28
        styleColor: "#ffffff"
        font.pixelSize: 60
    }
}
