#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <algorithm>
#include <cstdint> 
#include <QGridLayout>
#include <QShortcut>
#include <QLabel>
#include <QFont>
#include <QApplication>

namespace {
inline std::int32_t clamp(std::int32_t v, std::int32_t lo, std::int32_t hi) { return std::min(hi, std::max(lo, v)); }
}

using namespace std;
using namespace acc;

// Requirements traceability (MISRA Dir 7.3) - Node2 GUI
//
// Safety related requirements in this file: 
//Saf-REQ-1: The ACC shall be able to respond to a measurement error within 1 second by deactivating itself. 
//Saf-REQ-2: The ACC shall automatically detect sensor failure by checking the measured values for plausibility. 
//Saf-REQ-3: The ACC shall inform the driver of a detected sensor failure by means of a red LED indicator on the display. 
//Saf-REQ-4: The ACC shall switch off when a sensor failure is detected. 
//Saf-REQ-6: The ACC shall inform the driver of a detected communication subsystem failure by switching on a red warning LED. 
//Saf-REQ-7: The ACC shall automatically switch off when a communication subsystem (Bluetooth) error is detected. 
//Saf-REQ-9: The ACC shall inform the driver about the status of the ACC via a green status LED and the ACC push button display (ACC ON/ACC OFF). 
//Saf-REQ-10: The ACC shall prevent the user from activating it if the sensors or communication have failed. 
//Saf-REQ-11: Once activated, the ACC shall note the current speed of the vehicle and not exceed it. 
//Saf-REQ-12: The ACC shall deactivate when the vehicle speed falls below 30 km/h. 
//
//Requirements with no influence on Safety and Security:
//REQ-w-no-Saf-Sec-1: The ACC display shall have a black background. 
//REQ-w-no-Saf-Sec-2: The speed shall be displayed in km/h. 
//REQ-w-no-Saf-Sec-3: The distance to the vehicle in front shall be displayed in meters. 
//REQ-w-no-Saf-Sec-4: The speed shall be displayed as an integer value without decimal places. 
//REQ-w-no-Saf-Sec-5: There shall be one button for acceleration and one for deceleration in the GUI. 
//REQ-w-no-Saf-Sec-6: The GUI shall allow the speed to be continuously increased or decreased by holding down the acceleration and deceleration buttons without the user having to tap repeatedly. 
//REQ-w-no-Saf-Sec-7: Pressing the acceleration and one for deceleration buttons while ACC is in the ON state shall put the ACC in the OFF state.

MainWindow::MainWindow(bool &bTerminateApp, QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    bTerminateApp_(bTerminateApp),
    simTimer_(nullptr),
    lastAccState_(acc::AccState::Off)
{
    ui->setupUi(this);

    // Font size Distance/Speed Labels
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

    // --- Layout right column (make LED row visible) ---
    setupRightGridLayout();

    // --- LED initially visible/green & forward ---
    if (ui->ledACC) {
        ui->ledACC->setFixedSize(48, 48);
        ui->ledACC->setStyleSheet(
            "background:#32CD32; border:2px solid #444; border-radius:24px;");
        ui->ledACC->raise();
    }

    // --- GUI startup states ---
    // Saf-REQ-9, REQ-w-no-Saf-Sec-2, 4
    setSpeedKmh(0);
    // Saf-REQ-9, REQ-w-no-Saf-Sec-3
    setDistanceMeters(0.0, /*accFailed=*/true); // empty until measured value "arrives"
    updateAccState(AccState::Off);
    updateHealthLed();

    // ACC-Button deutlicher (Saf-REQ-9 - Statusanzeige ACC ON/OFF)
    {
        QFont f = ui->btnACC->font();
        f.setPointSize(18);
        f.setBold(true);
        ui->btnACC->setFont(f);
    }

    // --- Connections between buttons and slots ---
    // REQ-w-no-Saf-Sec-5: one button for +Speed, one for -Speed.
    // REQ-w-no-Saf-Sec-6: Auto-repeat is enabled via .ui settings, here every click is processed.
    connect(ui->btnACC,       &QPushButton::toggled, this, &MainWindow::onAccToggled);
    connect(ui->btnSpeedUp,   &QPushButton::clicked, this, &MainWindow::onSpeedUp);
    connect(ui->btnSpeedDown, &QPushButton::clicked, this, &MainWindow::onSpeedDown);
    ui->btnACC->setCheckable(true);

    // --- Keyboard shortcut to close (Esc / Ctrl+Q) ---
    auto esc  = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    auto quit = new QShortcut(QKeySequence::Quit, this); // Ctrl+Q
    connect(esc,  &QShortcut::activated, this, &QWidget::close);
    connect(quit, &QShortcut::activated, this, &QWidget::close);

    // --- Periodic updating of the GUI from the global VehicleState ---
    simTimer_ = new QTimer(this);
    simTimer_->setInterval(50); // 50ms
    connect(simTimer_, &QTimer::timeout, this, &MainWindow::onSimTick);
    simTimer_->start();
}

MainWindow::~MainWindow() { delete ui; }

// Fix the layout of the right column (make LED visible)
// ------------------------------------------------------------------

void MainWindow::setupRightGridLayout() {
    // right area is located in gridLayout_rechts (Widget: gridLayoutWidget)
    auto right = ui->centralwidget->findChild<QGridLayout*>("gridLayout_rechts");
    if (!right) return;

    // Rows: 0=LED, 1/3/5=Lines, 2=ACC, 4=Up, 6=Down
    right->setRowMinimumHeight(0, 90);  // LED strip gets fixed height
    right->setRowStretch(0, 2);

    // Lines should not take up space
    right->setRowStretch(1, 0);
    right->setRowStretch(3, 0);
    right->setRowStretch(5, 0);

    // Large areas share the rest
    right->setRowStretch(2, 3);   // ACC
    right->setRowStretch(4, 3);   // Up
    right->setRowStretch(6, 3);   // Down

    // Slightly more compact spacing
    right->setContentsMargins(2, 2, 2, 2);
    right->setHorizontalSpacing(1);
    right->setVerticalSpacing(4);
}

// Setter methods for speed, distance, ACC availability, fault
// ------------------------------------------------------------------

// Saf-REQ-9, REQ-w-no-Saf-Sec-2, 4
void MainWindow::setSpeedKmh(std::int32_t kmh) {
    std::int32_t clamped = clamp(kmh, 0, 200);
    ui->lcdNumberSpeed->display(clamped);
    updateSpeedStyle(clamped);
}

// Saf-REQ-9, REQ-w-no-Saf-Sec-3
void MainWindow::setDistanceMeters(double m, bool accFailed) {
    if (accFailed) {
        ui->labelDistanzNummer->setText("");  // show nothing
        return;
    }
    ui->labelDistanzNummer->setText(QString::number(m, 'f', 2));
}

// Saf-REQ-3, Saf-REQ-6, Saf-REQ-9, Saf-REQ-10
void MainWindow::setAccAvailable(bool ok) {
    
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Communication/sensor problem -> ACC off, driver is warned.
    // (Saf-REQ-6, Saf-REQ-7, Saf-REQ-10 - in combination with global state)
    if (!ok) { updateAccState(AccState::Off); }
    else
    {
        // Availability restored --> Adjust display to global status
        // Saf-REQ-9
        updateAccState(vehicleState.accState);
    }
    // Saf-REQ-3, Saf-REQ-6, Saf-REQ-9
    updateHealthLed();
}

// Saf-REQ-3, Saf-REQ-6, Saf-REQ-9, Saf-REQ-10
void MainWindow::setFault(bool faultOn) {
    
    // faultOn signals that an error has occurred or been corrected.
    // However, the actual ACC status is located in the global VehicleState.
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    if (faultOn) {
        // Fault --> Red indicator LED, ACC not operable. (Saf-REQ-3, 6, 7, 10)
        updateAccState(AccState::Fault);
    } 
    else {
        // Fault Fixed --> Display back to current ACC status.
        updateAccState(vehicleState.accState);
    }
    updateHealthLed();
}

// Button-Slots
// ------------------------------------------------------------------

// Saf-REQ-9, Saf-REQ-10, Saf-REQ-11
void MainWindow::onAccToggled(bool) 
{
    AccState newAccState = AccState::Off;
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    if (vehicleState.accState == AccState::Fault)
    {
        // Saf-REQ-10: Prevent activation when ACC is in fault mode.
        ui->btnACC->blockSignals(true);
        ui->btnACC->setChecked(false);
        ui->btnACC->blockSignals(false);
        newAccState = AccState::Off;
    }
    else
    {
        newAccState = (vehicleState.accState == AccState::Off) ? AccState::On : AccState::Off; 
    }

    // Saf-REQ-11: Save ACC set speed when switching on
    uint32_t newMax = (newAccState == AccState::On) ? vehicleState.speedMetersPerHour : 0U;

    // Update GUI (Saf-REQ-9)
    updateAccState(newAccState);
    // Update Global Vehicle State (Saf-REQ-11)
    setCurrentVehicleState(&newAccState, nullptr, nullptr, &newMax);
}

// Saf-REQ-11, REQ-w-no-Saf-Sec-5, 6, 7
void MainWindow::onSpeedUp() 
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Increase current speed, up to VEHICLE_SPEED_MAX
    vehicleState.speedMetersPerHour += 5000U;
    vehicleState.speedMetersPerHour = min<uint32_t>(vehicleState.speedMetersPerHour, VEHICLE_SPEED_MAX * 1000U);

    // Saf-REQ-11, REQ-w-no-Saf-Sec-7
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
    setSpeedKmh(static_cast<std::int32_t>(vehicleState.speedMetersPerHour / 1000U));
    // write back global vehicle state
    setCurrentVehicleState(pAccState, &vehicleState.speedMetersPerHour, nullptr);
}

// Saf-REQ-11, REQ-w-no-Saf-Sec-5, 6, 7
void MainWindow::onSpeedDown()
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Decrease current speed, not below 0
    uint32_t speedReduction = min(vehicleState.speedMetersPerHour, static_cast<uint32_t>(5000U));
    vehicleState.speedMetersPerHour -= speedReduction;

    // Saf-REQ-11, REQ-w-no-Saf-Sec-7
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
    setSpeedKmh(static_cast<std::int32_t>(vehicleState.speedMetersPerHour / 1000U));
    // write back global vehicle state
    setCurrentVehicleState(pAccState, &vehicleState.speedMetersPerHour, nullptr);
}

// Display of ACC status, LED, speed color
// ------------------------------------------------------------------

// Saf-REQ-9, Saf-REQ-10
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
        
    // Saf-REQ-10: Button can only be operated if there is no fault.
    ui->btnACC->setEnabled(s != AccState::Fault);
    
}

// Saf-REQ-3, Saf-REQ-6, Saf-REQ-9
void MainWindow::updateHealthLed() {
    if (!ui->ledACC) return;

    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Error state is represented by global VehicleState.accState == Fault.
    const bool anyFault = (vehicleState.accState == AccState::Fault);

    const char* base = "border:2px solid #444; border-radius:24px;";
    ui->ledACC->setStyleSheet(QString("background-color:%1; %2").arg(anyFault ? "#ff3b30" : "#32CD32").arg(base));
    ui->ledACC->raise();
}

void MainWindow::updateSpeedStyle(std::int32_t kmh) {
    // 0..70 green, 71..130 yellow, >130 red
    QString base = "background: black; color:%1;";
    QString col  = (kmh <= 70) ? "#32CD32" : (kmh <= 130) ? "#ffd166" : "#ff3b30";
    ui->lcdNumberSpeed->setStyleSheet(base.arg(col));
}

// Saf-REQ-1, 2, 3, 4, 6, 9, 10, 11, 12
// updates the GUI
void MainWindow::onSimTick() 
{
    VehicleStateInfoType vehicleState;
    getCurrentVehicleState(&vehicleState);

    // Business logic - when ACC changes from Fault to "operable" again, we force ACC OFF, regardless of the button status. (Saf-REQ-10)
    if (lastAccState_ == AccState::Fault &&
        vehicleState.accState != AccState::Fault)
    {
        // global status set to OFF
        AccState offState = AccState::Off;
        vehicleState.accState = AccState::Off;

        // Set button to "not pressed" without triggering a signal
        ui->btnACC->blockSignals(true);
        ui->btnACC->setChecked(false);
        ui->btnACC->blockSignals(false);

        // Only write the ACC status back to the global VehicleState.
        setCurrentVehicleState(&offState, nullptr, nullptr);
    }

    // Remember current state for next tick
    lastAccState_ = vehicleState.accState;

    // Saf-REQ-2 (Plausibility check), Saf-REQ-9, REQ-w-no-Saf-Sec-3:
    setDistanceMeters(vehicleState.distanceMeters, !isValidDistance(vehicleState.distanceMeters));
    // Saf-REQ-9, Saf-REQ-10
    updateAccState(vehicleState.accState);
    // Saf-REQ-3, Saf-REQ-6, Saf-REQ-9
    updateHealthLed();

    if (vehicleState.accState == AccState::On)
    {
        // Saf-REQ-9, Saf-REQ-11, Saf-REQ-12
        setSpeedKmh(static_cast<std::int32_t>(vehicleState.speedMetersPerHour / 1000U));
    }

    if (bTerminateApp_)
    {
        QApplication::quit();
    }
}
