import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BlePresence 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 480
    height: 720
    title: "BLE Attendance (Sessão compartilhada)"

    // Lados C++
    PresenceAdvertiser { id: advertiser; rollingPeriodSec: 20 }
    PresenceScanner {
        id: scanner
        scanTimeoutMs: 6000
        rssiMin: -90
        onScanStarted: console.log("[SCAN] started")
        onScanFinished: console.log("[SCAN] finished")
        onPresenceMarked: (studentId, rssi) => {
            console.log("[SCAN] ✓", studentId, "RSSI", rssi)
            presentListModel.insert(0, { studentId: studentId, rssi: rssi, ts: new Date().toLocaleTimeString() })
            console.log("[UI] append count=", presentListModel.count, "first=", JSON.stringify(presentListModel.get(0)))
        }
    }

    // Parâmetros base
    property int courseId: 1731
    property string nonceSessao: "A1B2C3" // pode ser texto; ambos os lados devem usar o mesmo valor!
    property int periodSec: advertiser.rollingPeriodSec

    // "Código de sessão" que o professor gera e o aluno cola
    // O professor gera como floor(now/period)*period
    property int sessionCode: 0

    ListModel { id: presentListModel }

    header: TabBar {
        id: tabbar
        TabButton { text: "Aluno" }
        TabButton { text: "Professor" }
    }

    SwipeView {
        id: pages
        anchors.fill: parent
        currentIndex: tabbar.currentIndex
        onCurrentIndexChanged: tabbar.currentIndex = currentIndex

        // --------------------- ALUNO ---------------------
        Item {
            Column {
                spacing: 10
                anchors.fill: parent
                anchors.margins: 16

                Text { text: "Configuração do Aluno"; font.pixelSize: 18 }

                TextField {
                    id: tfSecret
                    placeholderText: "Secret do aluno (texto/hex/base64)"
                    text: "segredo-aluno-001"
                }

                Row {
                    spacing: 8
                    SpinBox {
                        id: sbCourse
                        from: 1; to: 65535
                        value: courseId
                        editable: true
                        onValueChanged: courseId = value
                    }
                    SpinBox {
                        id: sbPeriod
                        from: 5; to: 120
                        value: advertiser.rollingPeriodSec
                        editable: true
                        onValueChanged: advertiser.rollingPeriodSec = value
                    }
                }

                TextField {
                    id: tfSessionCode
                    placeholderText: "Cole o CÓDIGO DE SESSÃO do professor"
                    inputMethodHints: Qt.ImhDigitsOnly
                }

                Button {
                    text: advertiser.advertisingActive ? "Parar anúncio" : "Iniciar anúncio"
                    onClicked: {
                        const start = parseInt(tfSessionCode.text)
                        if (!start || start <= 0) {
                            console.warn("[ALUNO] Código de sessão inválido")
                            return
                        }
                        advertiser.configure(sbCourse.value, nonceSessao, start, tfSecret.text)
                        advertiser.advertisingActive ? advertiser.stop() : advertiser.start()
                    }
                }
                Label { text: advertiser.advertisingActive ? "Anunciando presença..." : "Parado" }
            }
        }

        // --------------------- PROFESSOR ---------------------
        Item {
            Column {
                spacing: 10
                anchors.fill: parent
                anchors.margins: 16

                Text { text: "Painel do Professor"; font.pixelSize: 18 }

                Row {
                    spacing: 8
                    Button {
                        text: "Carregar roster de exemplo"
                        onClicked: {
                            const roster = [
                                { id: "aluno01", secret: "segredo-aluno-001" },
                                { id: "aluno02", secret: "segredo-aluno-002" },
                                { id: "aluno03", secret: "segredo-aluno-003" }
                            ]
                            scanner.setRoster(roster)
                        }
                    }
                    Button {
                        text: "Limpar presenças"
                        onClicked: presentListModel.clear()
                    }
                }

                Row {
                    spacing: 8
                    SpinBox {
                        id: sbCourse2
                        from: 1; to: 65535
                        value: courseId
                        editable: true
                        onValueChanged: courseId = value
                    }
                    SpinBox {
                        id: sbRssi
                        from: -100; to: 0
                        value: scanner.rssiMin
                        editable: true
                        onValueChanged: scanner.rssiMin = value
                    }
                    SpinBox {
                        id: sbTimeout
                        from: 1000; to: 15000
                        stepSize: 1000
                        value: scanner.scanTimeoutMs
                        editable: true
                        onValueChanged: scanner.scanTimeoutMs = value
                    }
                }

                // Geração e compartilhamento do CÓDIGO DE SESSÃO
                Row {
                    spacing: 8
                    Button {
                        text: "Gerar CÓDIGO DE SESSÃO"
                        onClicked: {
                            const nowSec = Math.floor(Date.now()/1000)
                            const p = advertiser.rollingPeriodSec
                            sessionCode = Math.floor(nowSec / p) * p  // âncora = múltiplo do período
                        }
                    }
                    TextField {
                        id: tfShowCode
                        text: sessionCode > 0 ? String(sessionCode) : ""
                        readOnly: true
                        placeholderText: "Clique em Gerar"
                        width: 180
                    }
                    Button {
                        text: "Copiar"
                        enabled: sessionCode > 0
                        onClicked: {
                            Qt.callLater(() => Qt.application.clipboard.setText(String(sessionCode)))
                        }
                    }
                }

                Row {
                    spacing: 8
                    Button {
                        text: "Escanear 1x";
                        onClicked:  {
                            if (!sessionCode) {
                                console.warn("[PROF] Gere o CÓDIGO DE SESSÃO primeiro")
                                return
                            }
                            scanner.configure(sbCourse2.value, nonceSessao, sessionCode, advertiser.rollingPeriodSec)
                            console.log("[UI] sessão configurada")
                            scanner.startScan()
                        }
                    }
                    Button { text: "Parar"; onClicked: scanner.stopScan() }
                }

                // Auto-escanear (janelas de 6s a cada 12s)
                Row {
                    spacing: 8
                    CheckBox {
                        id: cbAuto
                        text: "Auto-escanear"
                    }
                    Label { text: "Janela 6s / ciclo 12s" }
                }

                Timer {
                    id: autoScanTimer
                    interval: 12000; running: cbAuto.checked; repeat: true
                    onTriggered: {
                        if (!scanner.scanning) {
                            scanner.configure(sbCourse2.value, nonceSessao, sessionCode, advertiser.rollingPeriodSec)
                            console.log("[UI] sessão configurada")
                            scanner.startScan()
                        }
                    }
                }

                GroupBox {
                    title: "Presenças detectadas"
                    anchors.left: parent.left
                    anchors.right: parent.right
                    // defina uma altura para a área da lista
                    height: 280

                    ListView {
                        id: list
                        model: presentListModel

                        // >>> garante que a lista ocupe o espaço visível
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true

                        delegate: Row {
                            // faça o delegate ocupar a largura da lista
                            width: list.width
                            spacing: 12

                            // use as roles do modelo explicitamente via 'model.'
                            Text { text: model.studentId; width: 140; elide: Text.ElideRight }
                            Text { text: "RSSI " + model.rssi; width: 70 }
                            Text { text: "(" + model.ts + ")"; elide: Text.ElideRight }
                        }
                    }
                }
            }
        }
    }
}
