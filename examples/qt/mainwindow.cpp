#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "device.h"
#include "hidapi.h"
#include <iostream>
#include <iomanip>
#include <sstream>

#include <QMessageBox>

char buf[65] = {0x00};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(about()));
    this->statusBar()->setSizeGripEnabled(false);
    this->setFixedSize(size());
    ui->treeWidget->header()->resizeSection(0, 200);
    ui->treeWidget->header()->resizeSection(1, 85);
    ui->treeWidget->header()->resizeSection(2, 70);

    m_hid.setCallbackArrival([this](HidDevice* d){return arrivalCallback(d);});
    m_hid.setCallbackRemoval([this](HidDevice* d){return removalCallback(d);});
    connect(this, SIGNAL(readCallbackSignal(HidDevice*)), this, SLOT(readCallbackSlot(HidDevice*)));
    connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(setDevice(QTreeWidgetItem*,int)));
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(sendData()));
    connect(ui->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(dataChanged(QString)));
    connect(ui->pushButton, SIGNAL(released()), this, SLOT(sendData()));
    connect(ui->pushButton_2, SIGNAL(released()), this, SLOT(clearDisplay()));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFormat(int)));
    connect(ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(setLength(int)));
    refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refresh()
{
    ui->treeWidget->clear();
    for(auto &x : m_hid.m_devices)
        if((x.second)->isConnected()) {
            Device *device = new Device(QString::fromStdWString((x.second)->getProduct()),
                                        QString::number((x.second)->getVid(), 16).toUpper().rightJustified(4, '0'),
                                        QString::number((x.second)->getPid(), 16).toUpper().rightJustified(4, '0'),
                                        QString::fromStdWString((x.second)->getSerialNumber()),
                                        (x.second));
            ui->treeWidget->insertTopLevelItem(0, device);

            if (x.second == m_d)
                ui->treeWidget->setCurrentItem(device);
        }
    return;
}

void MainWindow::setDevice(QTreeWidgetItem* item, int column)
{
    if (m_d != nullptr && m_d->isOpen()) {
        m_d->setCallbackReadComplete(nullptr);
        m_d->setReadContinuous(false);
        m_d->close();
    }
    m_d = (dynamic_cast<Device*>(item))->getHidDevicePtr();
    if (m_d != nullptr && m_d->open()) {
        m_d->setCallbackReadComplete([this](HidDevice* d){return readCallback(d);});
        m_d->setWriteBlocking(true);
        m_d->setReadBlocking(false);
        m_d->setReadContinuous(true);
        m_d->read();
    }
}

void MainWindow::sendData()
{
    if (m_d != nullptr && m_d->isOpen()) {
    } else {
        return;
    }

    if (m_dataChanged) {
        m_dataChanged = false;
        memset(buf, 0, sizeof(buf));
        if(m_format) { // ASCII
            strncpy(buf+1, ui->lineEdit->text().toStdString().c_str(), sizeof(buf));
        } else { // HEX
            int j = 0;
            QString s = ui->lineEdit->text();
            QString ns;
            for (auto i = s.begin(); i != s.end(); i++) {
                if (i->isLetterOrNumber()) {
                    ns.append(*i);
                    j++;
                    if (j > 1) {
                        j=0;
                        ns.append(' ');
                    }
                }
            }
            QStringList p = ns.split(" ");
            j = 1;
            for (auto i = p.begin(); i != p.end(); i++) {
                bool f;
                unsigned int v = i->toUInt(&f, 16);
                buf[j++] = v;
            }
        }
    }
    m_d->write(buf);
}

void MainWindow::dataChanged(QString s)
{
    m_dataChanged = true;
}

void MainWindow::clearDisplay()
{
    ui->plainTextEdit->clear();
}

void MainWindow::setFormat(int index)
{
    m_format = index;
    m_dataChanged = true;
    return;
}

void MainWindow::setLength(int length)
{
    m_length = length;
    return;
}

void MainWindow::arrivalCallback(HidDevice *d)
{
    /* If the selected device has been connected after disconnection then reopen it */
    if (m_d == d && m_d != nullptr && m_d->open()) {
        m_d->setCallbackReadComplete([this](HidDevice* d){return readCallback(d);});
        m_d->setWriteBlocking(true);
        m_d->setReadBlocking(false);
        m_d->setReadContinuous(true);
        m_d->read();
    }
    refresh();
    return;
}

void MainWindow::removalCallback(HidDevice *d)
{
    refresh();
    return;
}

void MainWindow::readCallback(HidDevice *d)
{
    // GUI can only be modified from the main thread
    // so we use signals to notify main thread of received data
    emit readCallbackSignal(d);
}

void MainWindow::readCallbackSlot(HidDevice *d)
{
    std::ostringstream os;
//    if (d->m_readBuf == nullptr)
//        return;
    for(auto i = 1; i < m_length; i++)
        os << std::hex << std::setw(2) << std::setfill('0') <<  (unsigned int)((d->m_readBuf)[i]) << " ";
    ui->plainTextEdit->appendPlainText(QString::fromStdString(os.str()));
    return;
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Yaha example"),
            tr("<b>Yaha</b> version something."));
}
