#pragma once

#include <QMainWindow>
#include <QTimer>
#include <cstdint> 

#include "Node2Types.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Requirements traceability (MISRA Dir 7.3) – Node2 GUI
//
// Safety related requirements: 
//Saf-REQ-3: The ACC shall inform the driver of a detected sensor failure by means of a red LED indicator on the display. 
//Saf-REQ-6: The ACC shall inform the driver of a detected communication subsystem failure by switching on a red warning LED. 
//Saf-REQ-9: The ACC shall inform the driver about the status of the ACC via a green status LED and the ACC push button display (ACC ON/ACC OFF). 
//Saf-REQ-10: The ACC shall prevent the user from activating it if the sensors or communication have failed. 
//Saf-REQ-11: Once activated, the ACC shall note the current speed of the vehicle and not exceed it. 
//
//Requirements with no influence on Safety and Security:
//REQ-w-no-Saf-Sec-1: The ACC display shall have a black background. 
//REQ-w-no-Saf-Sec-2: The speed shall be displayed in km/h. 
//REQ-w-no-Saf-Sec-3: The distance to the vehicle in front shall be displayed in meters. 
//REQ-w-no-Saf-Sec-4: The speed shall be displayed as an integer value without decimal places. 
//REQ-w-no-Saf-Sec-5: There shall be one button for acceleration and one for deceleration in the GUI. 
//REQ-w-no-Saf-Sec-6: The GUI shall allow the speed to be continuously increased or decreased by holding down the acceleration and deceleration buttons without the user having to tap repeatedly. 
//REQ-w-no-Saf-Sec-7: Pressing the acceleration and one for deceleration buttons while ACC is in the ON state shall put the ACC in the OFF state.

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool &bTerminateApp, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setSpeedKmh(std::int32_t kmh);
    void setDistanceMeters(double m, bool accFailed = false);
    void setAccAvailable(bool ok);
    void setFault(bool faultOn);

private slots:
    void onAccToggled(bool on);
    void onSpeedUp();
    void onSpeedDown();
    void onSimTick();                // einfache Distanz-Simulation

private:

    void setupRightGridLayout();     // Zeilen/Spalten für gridLayout_rechts setzen
    void updateAccState(acc::AccState s); // Button-Text/Farbe/Enable
    void updateHealthLed();          // LED rot/grün je nach fault_
    void updateSpeedStyle(std::int32_t kmh);  // Farbe der LCD-Anzeige
    void showAlarm(bool on);         // ggf. später für Alarm-Frame

    Ui::MainWindow *ui;
    bool &bTerminateApp_;

    QTimer* simTimer_;

    // NEW: letzter bekannter ACC-Zustand
    acc::AccState lastAccState_;
};
