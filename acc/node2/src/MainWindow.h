#pragma once

#include <QMainWindow>
#include <QTimer>

#include "Node2Types.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool &bTerminateApp, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setSpeedKmh(int kmh);
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
    void updateSpeedStyle(int kmh);  // Farbe der LCD-Anzeige
    void showAlarm(bool on);         // ggf. später für Alarm-Frame

    Ui::MainWindow *ui;
    bool &bTerminateApp_;

    int  currentSpeed_  = 0;
    bool accAvailable_  = true;
    bool fault_         = false;

    QTimer* simTimer_   = nullptr;

    // NEW: letzter bekannter ACC-Zustand
    acc::AccState lastAccState_ = acc::AccState::Off;
};
