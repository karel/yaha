#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidget>
#include <QMainWindow>
#include <QTreeWidgetItem>

#include "hidapi.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void arrivalCallback(HidDevice*);
    void removalCallback(HidDevice*);
    void readCallback(HidDevice*);

public slots:
    void readCallbackSlot(HidDevice*);
    void refresh();
    void setDevice(QTreeWidgetItem*,int);
    void sendData();
    void dataChanged(QString);
    void clearDisplay();
    void setFormat(int);
    void setLength(int);

private slots:
    void about();

signals:
    void readCallbackSignal(HidDevice*);

private:
    Ui::MainWindow *ui;
    HidApi m_hid;
    HidDevice *m_d = nullptr;

    int m_format = 0;
    int m_length = 25;
    bool m_dataChanged = true;
};

#endif // MAINWINDOW_H
