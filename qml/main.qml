import QtQuick 2.7
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtCharts 2.0
import me.caizhenbin.px4 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1000
    height: 620
    minimumWidth: 400
    minimumHeight: 300
    title: qsTr("PX4 Log Viewer")

    property ListModel paramsModel: ListModel {}
    property ListModel itemsModel: ListModel {}
    property ListModel infoModel: ListModel {}
    property QtObject chart
    property QtObject axisX
    property QtObject axisY

    property var lines: []
    property var scatters: []
    property var minX: []
    property var maxX: []
    property var minY: []
    property var maxY: []

    function arrayMin(arr) {
        var len = arr.length, min = Infinity;
        while (len--) {
            if (arr[len] !== null && arr[len] < min) {
                min = arr[len];
            }
        }
        return (min == null || min == Infinity) ? -1 : min;
    }

    function arrayMax(arr) {
        var len = arr.length, max = -Infinity;
        while (len--) {
            if (arr[len] !== null && arr[len] > max) {
                max = arr[len];
            }
        }
        return (max == null || max == -Infinity) ? 1 : max;
    }

    RowLayout {
        anchors.top: pathBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 6

        TabView {
            id: tabs
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: 360
            Layout.maximumWidth: 500
            //Layout.minimumHeight: 150

            Tab {
                title: qsTr("Items")
                TableView {
                    id: items
                    anchors.fill: parent
                    model: itemsModel
                    TableViewColumn {
                        role: "show"
                        title: qsTr("Type")
                        width: 20
                        delegate: CheckBox {
                            //anchors.centerIn: parent
                            checked: styleData.value
                            onCheckedChanged: {
                                var row = styleData.row;
                                var ser = lines[row];
                                var item = itemsModel.get(row);
                                if(!item) {
                                    return;
                                }
                                item.show = checked; //TypeError: Type error???
                                if(checked) {
                                    if(!ser) {
                                        ser = chart.createSeries(ChartView.SeriesTypeLine,
                                                                 item.field, axisX, axisY);
                                        //ser.useOpenGL = true;
                                        ser.color = item.color;
                                    }
                                    logParser.update(ser, item.field);
                                    lines[row] = ser;
                                    minX[row] = logParser.getMinX(item.field);
                                    maxX[row] = logParser.getMaxX(item.field);
                                    minY[row] = logParser.getMinY(item.field);
                                    maxY[row] = logParser.getMaxY(item.field);

                                } else {
                                    chart.removeSeries(ser);
                                    lines[row] = null;
                                    minX[row] = null;
                                    maxX[row] = null;
                                    minY[row] = null;
                                    maxY[row] = null;
                                }

                                //console.debug(minY, maxY)
                                var min, max;
                                min = arrayMin(minX);
                                max = arrayMin(maxX);
                                max += (max === min) ? 1 : 0;
                                axisX.min = logParser.ms2date(min);
                                axisX.max = logParser.ms2date(max);

                                min = arrayMin(minY);
                                max = arrayMax(maxY);
                                var dv = (max - min == 0) ? 0.5 : (max - min)*0.05;
                                axisY.min = min - dv;
                                axisY.max = max + dv;
                            }
                        }
                    }
                    TableViewColumn {
                        role: "field"
                        title: qsTr("Field")
                        width: 130
                    }
                    TableViewColumn {
                        role: "type"
                        title: qsTr("Type")
                        width: 60
                    }
                    TableViewColumn {
                        role: "color"
                        title: qsTr("Color")
                        width: 50
                        delegate: Rectangle {
                            anchors.fill: parent
                            anchors.margins: 2
                            color: itemsModel.get(styleData.row) !== undefined ? itemsModel.get(styleData.row).color : "white"
                            //Qt.hsla(styleData.row/itemsModel.count, 0.9 + Math.random() / 10, 0.5 + Math.random() / 10, 1)
                            border.width: 1
                            border.color: "gray"
                        }
                    }
                    TableViewColumn {
                        role: "length"
                        title: qsTr("Length")
                        width: 60
                    }
                }
            }

            Tab {
                title: qsTr("Parameters")
                TableView {
                    id: params
                    anchors.fill: parent
                    model: paramsModel
                    TableViewColumn {
                        role: "name"
                        title: qsTr("Name")
                        width: 180
                    }
                    TableViewColumn {
                        role: "value"
                        title: qsTr("Value")
                        width: 150
                    }
                }
            }

            Tab {
                title: qsTr("Log Info")
                TableView {
                    id: infos
                    anchors.fill: parent
                    model: infoModel
                    TableViewColumn {
                        role: "name"
                        title: qsTr("Property")
                        width: 180
                    }
                    TableViewColumn {
                        role: "value"
                        title: qsTr("Value")
                        width: 150
                    }
                }
            }
        }

        TabView {
            id: contentTab
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 600

            Tab {
                title: qsTr("Chart")

                ChartView {
                    id: chart
                    title: logParser.arch
                    antialiasing: true
                    legend.alignment: Qt.AlignBottom
                    anchors.fill: parent

                    Component.onCompleted: {
                        root.chart = chart;
                    }

                    DateTimeAxis {
                        id: axisX
                        titleText: "Time"
                        tickCount: 11
                        format: "mm''ss"
                        Component.onCompleted: {
                            root.axisX = axisX;
                        }
                    }

                    ValueAxis {
                        id: axisY
                        titleText: "Value"
                        tickCount: 11
                        Component.onCompleted: {
                            root.axisY = axisY;
                        }
                    }

                    Rectangle {
                        id: rubberBand
                        width: 0
                        height: 0
                        visible: false
                        border.color: "gray"
                        border.width: 1
                        color: "steelblue"
                        opacity: 0.3
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.AllButtons
                        scrollGestureEnabled: false

                        Item {
                            id: plotAreaItem
                            x: chart.plotArea.x
                            y: chart.plotArea.y
                            width: chart.plotArea.width
                            height: chart.plotArea.height
                        }

                        function isInPlotArea(px, py) {
                            return px > chart.plotArea.x
                                    && px < chart.plotArea.x + chart.plotArea.width
                                    && py > chart.plotArea.y
                                    && py < chart.plotArea.y + chart.plotArea.height;
                        }

                        property bool move: false
                        property real ox: 0
                        property real oy: 0

                        onPressed: {
                            if(!isInPlotArea(mouseX, mouseY))
                                return;

                            ox = mouseX;
                            oy = mouseY;
                            rubberBand.x = ox;
                            rubberBand.y = oy;
                            rubberBand.width = 0;
                            rubberBand.height = 0;
                            rubberBand.visible = true;
                        }

                        onPositionChanged: {
                            if(!isInPlotArea(mouseX, mouseY))
                                return;

                            if(ox == mouseX && oy == mouseY) {
                                return;
                            }
                            move = true;
                            rubberBand.width = mouseX - ox;
                            rubberBand.height = mouseY - oy;
                        }

                        onReleased: {
                            //console.debug(mouse.button)
                            if(move) {
                                chart.zoomIn(Qt.rect(rubberBand.x, rubberBand.y,
                                                     rubberBand.width, rubberBand.height));
                                rubberBand.width = 0;
                                rubberBand.height = 0;
                                move = false;
                                return;
                            }

                            if(!isInPlotArea(mouseX, mouseY))
                                return;

                            if(mouse.button === Qt.LeftButton) {
                                chart.zoomIn();
                            } else if (mouse.button === Qt.RightButton) {
                                chart.zoomOut();
                            } else if (mouse.button === Qt.MiddleButton) {
                                chart.zoomReset();
                            }
                        }

                        onWheel: {
                            //if (wheel.modifiers & Qt.ControlModifier) {
                            console.debug(wheel.angleDelta)
                            if(wheel.angleDelta.y > 0) {
                                chart.zoomOut();
                            } else {
                                chart.zoomIn();
                            }
                        }
                    }
                }
            }
        }
    }

    LogParser {
        id: logParser
        onReadyChanged: {
            if(isReady) {
                var prop;

                paramsModel.clear();
                var paramsObj = logParser.getParams();
                for (prop in paramsObj) {
                    paramsModel.append({name: prop, value: paramsObj[prop]});
                }

                infoModel.clear();
                var infoObj = logParser.info;
                for (prop in infoObj) {
                    infoModel.append({name: prop, value: infoObj[prop]});
                }

                itemsModel.clear();
                var itemsObj = logParser.getItemsName();
                //lines = [];
                lines.length = 0;
                scatters.length = 0;
                maxY.length = 0;
                minY.length = 0;
                if(chart.count != 0) {
                    chart.removeAllSeries();
                }
                for (prop in itemsObj) {
                    //var color = hashStringToColor(prop);
                    itemsModel.append({field: prop,
                                          type: itemsObj[prop],
                                          show: false,
                                          color: hashStringToColor(prop),
                                          length: logParser.getDataLength(prop)
                                      });
                }

                axisX.min = logParser.minDate();
                axisX.max = logParser.maxDate();
            }
        }
        onPathChanged: {
            pathSelector.addPath(path.toString())
        }
    }

    MessageDialog {
        id: aboutBox
        title: qsTr("About PX4 Log Viewer")
        text: qsTr("This is a PX4 Log Viewer")
        icon: StandardIcon.Information
    }

    FileDialog {
        id: fileDialog
        folder: shortcuts.home
        nameFilters: ["PX4 log files (*.px4log *.bin)", "All files (*)"]
        selectExisting: true
        selectMultiple: false
        selectFolder: false
        onAccepted: {
            if (fileDialog.selectExisting) {
                //console.debug(fileDialog.fileUrl);
                logParser.path = fileDialog.fileUrl;
            }
        }
    }

    Action {
        id: fileOpenAction
        iconSource: "qrc:/qml/images/fileopen.png"
        iconName: "document-open"
        text: qsTr("Open")
        shortcut: StandardKey.Open
        onTriggered: {
            fileDialog.open()
        }
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            MenuItem { action: fileOpenAction }
            MenuItem { text: "Quit"; onTriggered: Qt.quit() }
        }
        Menu {
            title: "&Help"
            MenuItem { text: "About..." ; onTriggered: aboutBox.open() }
        }
    }

    toolBar: ToolBar {
        id: mainToolBar
        width: parent.width
        RowLayout {
            anchors.fill: parent
            spacing: 0
            ToolButton { action: fileOpenAction }
            Item { Layout.fillWidth: true }
        }
    }

    ToolBar {
        id: pathBar
        width: parent.width
        ComboBox {
            id: pathSelector
            width: parent.width
            editable: true
            model: ListModel {
                id: model
            }
            onAccepted: {
                if (find(currentText) === -1) {
                    model.append({text: editText});
                    currentIndex = find(editText);
                    logParser.path = "file://" + editText;
                }
            }
            onCurrentIndexChanged: {
                if(currentText != "") {
                    logParser.path = "file://" + currentText;
                }
            }

            function addPath(path) {
                //console.debug("--path:", path)
                path = path.replace(/^(file:\/{2})/,"");
                // unescape html codes like '%23' for '#'
                var cleanPath = decodeURIComponent(path);
                //console.debug("--cleanPath:", cleanPath)
                if (find(cleanPath) === -1) {
                    model.append({text: cleanPath});
                    currentIndex = find(cleanPath);
                }
            }
        }
    }

    MessageDialog {
        id: errorDialog
    }

    function djb2(str){
        var hash = Math.floor((Math.random() * 10000) + 1000); //5381;
        for (var i = 0; i < str.length; i++) {
            hash = ((hash << 5) + hash) + str.charCodeAt(i); /* hash * 33 + c */
        }
        return hash;
    }

    function hashStringToColor(str) {
        var hash = djb2(str);
        var r = (hash & 0xFF0000) >> 16;
        var g = (hash & 0x00FF00) >> 8;
        var b = hash & 0x0000FF;
        return "#" + ("0" + r.toString(16)).substr(-2) + ("0" + g.toString(16)).substr(-2) + ("0" + b.toString(16)).substr(-2);
    }
}
