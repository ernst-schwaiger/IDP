#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <algorithm>
#include <QGridLayout>
#include <QShortcut>
#include <QLabel>
#include <QFont>

namespace {
inline int clamp(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }
}

using namespace std;
using namespace acc;

MainWindow::MainWindow(bool &bTerminateApp, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), bTerminateApp_(bTerminateApp)
{
    ui->setupUi(this);

    // Schriftgröße Distance/Speed Labels
    {
        QFont f1 = ui->label_Speed->font();
        f1.setPointSize(20);
        f1.setBold(true);
        ui->label_Speed->setFont(f1);

        QFont f2 = ui->label_Distance->font();
        f2.setPointSize(20);
        f2.setBold(true);
        ui->label_Distance->setFont(f2);
    }

    // --- Rechte Spalte layouten (LED-Zeile sichtbar machen) ---
    setupRightGridLayout();

    // --- LED initial sichtbar/grün & nach vorn ---
    if (ui->ledACC) {
        ui->ledACC->setFixedSize(48, 48);
        ui->ledACC->setStyleSheet(
            "background:#32CD32; border:2px solid #444; border-radius:24px;");
        ui->ledACC->raise();
    }

    // --- Startzustände der GUI ---
    currentSpeed_ = 0;
    setSpeedKmh(currentSpeed_);
    setDistanceMeters(0.0, /*accFailed=*/true); // leer bis Messwert „kommt“
    fault_ = false;
    accAvailable_ = true;
    updateAccState(AccState::Off);
    showAlarm(false);
    updateHealthLed();

    // ACC-Button deutlicher
    {
        QFont f = ui->btnACC->font();
        f.setPointSize(18);
        f.setBold(true);
        ui->btnACC->setFont(f);
    }

    // --- Verbindungen zwischen Buttons und Slots ---
    connect(ui->btnACC,       &QPushButton::toggled, this, &MainWindow::onAccToggled);
    connect(ui->btnSpeedUp,   &QPushButton::clicked, this, &MainWindow::onSpeedUp);
    connect(ui->btnSpeedDown, &QPushButton::clicked, this, &MainWindow::onSpeedDown);
    ui->btnACC->setCheckable(true);

    // --- Tastenkürzel zum Schließen (Esc / Ctrl+Q) ---
    auto esc  = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    auto quit = new QShortcut(QKeySequence::Quit, this); // Ctrl+Q
    connect(esc,  &QShortcut::activated, this, &QWidget::close);
    connect(quit, &QShortcut::activated, this, &QWidget::close);

    // --- einfache Distanz-Simulation ---
    simTimer_ = new QTimer(this);
    simTimer_->setInterval(50); // 50ms
    connect(simTimer_, &QTimer::timeout, this, &MainWindow::onSimTick);
    simTimer_->start();
}

MainWindow::~MainWindow() { delete ui; }

// Layout der rechten Spalte fixieren (LED sichtbar machen)
// ------------------------------------------------------------------

void MainWindow::setupRightGridLayout() {
    // Dein rechter Bereich liegt in gridLayout_rechts (Widget: gridLayoutWidget)
    auto right = ui->centralwidget->findChild<QGridLayout*>("gridLayout_rechts");
    if (!right) return;

    // Zeilen: 0=LED, 1/3/5=Linien, 2=ACC, 4=Up, 6=Down
    right->setRowMinimumHeight(0, 90);  // LED-Zeile bekommt fix Höhe
    right->setRowStretch(0, 2);

    // Linien sollen keinen Platz ziehen
    right->setRowStretch(1, 0);
    right->setRowStretch(3, 0);
    right->setRowStretch(5, 0);

    // Große Flächen teilen sich den Rest
    right->setRowStretch(2, 3);   // ACC
    right->setRowStretch(4, 3);   // Up
    right->setRowStretch(6, 3);   // Down

    // Etwas kompaktere Abstände
    right->setContentsMargins(2, 2, 2, 2);
    right->setHorizontalSpacing(1);
    right->setVerticalSpacing(4);
}

// Setter-Methoden für Speed, Distance, ACC-Verfügbarkeit, Fault
// ------------------------------------------------------------------

void MainWindow::setSpeedKmh(int kmh) {
    currentSpeed_ = clamp(kmh, 0, 200);
    ui->lcdNumberSpeed->display(currentSpeed_);
    updateSpeedStyle(currentSpeed_);
}

void MainWindow::setDistanceMeters(double m, bool accFailed) {
    if (accFailed) {
        ui->labelDistanzNummer->setText("");  // nichts anzeigen
        return;
    }
    ui->labelDistanzNummer->setText(QString::number(m, 'f', 2));
}

void MainWindow::setAccAvailable(bool ok) {
    accAvailable_ = ok;
    if (!ok) { updateAccState(AccState::Off); showAlarm(true); }
    else if (!fault_) { showAlarm(false); }
    updateHealthLed();
}

void MainWindow::setFault(bool faultOn) {
    fault_ = faultOn;
    if (fault_) {
        updateAccState(AccState::Fault);
        showAlarm(true);
    } else {
        updateAccState(ui->btnACC->isChecked() ? AccState::On : AccState::Off);
        showAlarm(!accAvailable_);
    }
    updateHealthLed();
}

// Button-Slots
// ------------------------------------------------------------------


void MainWindow::onAccToggled(bool) 
{
    AccState newAccState = AccState::Off;
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    if (vehicleState.accState == AccState::Fault)
    {
        ui->btnACC->blockSignals(true);
        ui->btnACC->setChecked(false);
        ui->btnACC->blockSignals(false);
        newAccState = AccState::Off;
    }
    else
    {
        newAccState = (vehicleState.accState == AccState::Off) ? AccState::On : AccState::Off; 
    }

    // NEW: max. erlaubte Geschwindigkeit beim Einschalten speichern, sonst 0
    uint32_t newMax = (newAccState == AccState::On) ? vehicleState.speedMetersPerHour : 0U;

    // Update GUI
    updateAccState(newAccState);
    // Update Global Vehicle State
    setCurrentVehicleState(&newAccState, nullptr, nullptr, &newMax);
}

void MainWindow::onSpeedUp() 
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Increase current speed, up to VEHICLE_SPEED_MAX
    vehicleState.speedMetersPerHour += 5000;
    vehicleState.speedMetersPerHour = min<uint32_t>(vehicleState.speedMetersPerHour, VEHICLE_SPEED_MAX * 1000);

    // If ACC is turned on, turn it off now
    AccState *pAccState = nullptr;
    if (vehicleState.accState == AccState::On)
    {
        vehicleState.accState = AccState::Off;
        pAccState = &vehicleState.accState;
        ui->btnACC->setChecked(false);
        updateAccState(AccState::Off);
    }

    // set speed in GUI (calc back to kilometers per hour)
    setSpeedKmh(vehicleState.speedMetersPerHour / 1000);
    // write back global vehicle state
    setCurrentVehicleState(pAccState, &vehicleState.speedMetersPerHour, nullptr);
}

void MainWindow::onSpeedDown()
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Decrease current speed, not below 0
    uint32_t speedReduction = min(vehicleState.speedMetersPerHour, static_cast<uint32_t>(5000U));
    vehicleState.speedMetersPerHour -= speedReduction;

    // If ACC is turned on, turn it off now
    AccState *pAccState = nullptr;
    if (vehicleState.accState == AccState::On)
    {
        vehicleState.accState = AccState::Off;
        pAccState = &vehicleState.accState;
        ui->btnACC->setChecked(false);
        updateAccState(AccState::Off);
    }

    // set speed in GUI (calc back to kilometers per hour)
    setSpeedKmh(vehicleState.speedMetersPerHour / 1000);
    // write back global vehicle state
    setCurrentVehicleState(pAccState, &vehicleState.speedMetersPerHour, nullptr);
}

// Darstellung von ACC-Status, LED, Speed-Farbe, Alarm
// ------------------------------------------------------------------

void MainWindow::updateAccState(AccState s) {
    const char* baseBtn =
        "border:1px solid #666; border-radius:8px; background:transparent;";

    switch (s) {
    case AccState::On:
        ui->btnACC->setText("ACC ON");
        ui->btnACC->setStyleSheet(QString("%1 color:#32CD32; font-weight:600;").arg(baseBtn));
        break;
    case AccState::Fault:
        ui->btnACC->setText("ACC OFF");
        ui->btnACC->setStyleSheet(QString("%1 color:#ff3b30; font-weight:600;").arg(baseBtn));
        break;
    case AccState::Off:
    default:
        ui->btnACC->setText("ACC OFF");
        ui->btnACC->setStyleSheet(QString("%1 color:#ff3b30; font-weight:600;").arg(baseBtn));
        break;
    }

    if (s != AccState::Fault)
    {
        ui->btnACC->setEnabled(accAvailable_ && !fault_);
    }
    else
    {
        ui->btnACC->setEnabled(false);
    }
}

void MainWindow::updateHealthLed() {
    if (!ui->ledACC) return;

    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // "Fehler" liegt vor, wenn entweder ein allgemeiner Fault (fault_)
    // oder ein ACC-Fault (accState == Fault) aktiv ist.
    const bool anyFault = fault_ || (vehicleState.accState == AccState::Fault);

    const char* base = "border:2px solid #444; border-radius:24px;";
    ui->ledACC->setStyleSheet(QString("background-color:%1; %2").arg(anyFault ? "#ff3b30" : "#32CD32").arg(base));
    ui->ledACC->raise();
}

void MainWindow::updateSpeedStyle(int kmh) {
    // 0..70 grün, 71..130 gelb, >130 rot (nur Beispiel)
    QString base = "background: black; color:%1;";
    QString col  = (kmh <= 70) ? "#32CD32" : (kmh <= 130) ? "#ffd166" : "#ff3b30";
    ui->lcdNumberSpeed->setStyleSheet(base.arg(col));
}

void MainWindow::showAlarm(bool on) {
    Q_UNUSED(on);
    // Platzhalter – könnte später Alarm-Frame sichtbar machen
}

// Timer-Callback für Distanz-Simulation
// ------------------------------------------------------------------

void MainWindow::onSimTick() 
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // NEW: Business-Logik - wenn ACC von Fault auf wieder "operabel" wechselt,
    //      erzwingen wir ACC OFF, unabhängig vom Button-Zustand.
    if (lastAccState_ == AccState::Fault &&
        vehicleState.accState != AccState::Fault)
    {
        // globaler Zustand auf OFF
        AccState offState = AccState::Off;
        vehicleState.accState = AccState::Off;

        // Button sicher auf "nicht gedrückt" setzen, ohne Signal auszulösen
        ui->btnACC->blockSignals(true);
        ui->btnACC->setChecked(false);
        ui->btnACC->blockSignals(false);

        // nur den ACC-Zustand ins globale VehicleState zurückschreiben
        setCurrentVehicleState(&offState, nullptr, nullptr);
    }

    // aktuellen Zustand für nächsten Tick merken
    lastAccState_ = vehicleState.accState;

    setDistanceMeters(vehicleState.distanceMeters, !isValidDistance(vehicleState.distanceMeters));
    updateAccState(vehicleState.accState);
    updateHealthLed();

    if (vehicleState.accState == AccState::On)
    {
        setSpeedKmh(vehicleState.speedMetersPerHour / 1000);
    }

    if (bTerminateApp_)
    {
        QApplication::quit();
    }
}
