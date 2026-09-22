// OTA 面板实现
#include "otapanel.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>

OtaPanel::OtaPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_binPath = new QLineEdit;
    m_btnBrowse = new QPushButton("浏览...");
    auto *pathRow = new QHBoxLayout;
    pathRow->addWidget(m_binPath, 1);
    pathRow->addWidget(m_btnBrowse);

    m_productId  = new QLineEdit("8w7dxp3Pj8");
    m_deviceName = new QLineEdit("dev1");

    form->addRow("固件 bin:", pathRow);
    form->addRow("产品 ID:", m_productId);
    form->addRow("设备名:", m_deviceName);
    root->addLayout(form);

    m_lblVer = new QLabel("设备当前版本: 未知");
    root->addWidget(m_lblVer);

    m_btnUpload = new QPushButton("上传并下发升级");
    root->addWidget(m_btnUpload);

    m_bar = new QProgressBar;
    m_bar->setRange(0, 100);
    m_bar->setValue(0);
    root->addWidget(m_bar);

    m_log = new QLabel("就绪");
    m_log->setWordWrap(true);
    root->addWidget(m_log);
    root->addStretch(1);

    connect(m_btnBrowse, &QPushButton::clicked, this, &OtaPanel::onBrowse);
    connect(m_btnUpload, &QPushButton::clicked, this, &OtaPanel::onUpload);
}

void OtaPanel::onBrowse()
{
    QString f = QFileDialog::getOpenFileName(this, "选择固件",
                                             QString(), "Firmware (*.bin)");
    if (!f.isEmpty()) m_binPath->setText(f);
}

void OtaPanel::onUpload()
{
    if (m_binPath->text().isEmpty()) {
        QMessageBox::warning(this, "OTA", "请先选择固件 bin 文件");
        return;
    }
    log("发起 OneNet FOTA 升级任务...");
    emit requestUpload(m_binPath->text(), m_productId->text(), m_deviceName->text());
}

void OtaPanel::setDeviceVersion(int ver)
{
    m_lblVer->setText(QString("设备当前版本: 0x%1 (v%2.%3)")
                      .arg(ver, 4, 16, QChar('0'))
                      .arg((ver >> 8) & 0xFF).arg(ver & 0xFF));
}

void OtaPanel::setProgress(int percent) { m_bar->setValue(percent); }

void OtaPanel::log(const QString &msg)
{
    m_log->setText(QDateTime::currentDateTime().toString("HH:mm:ss ") + msg);
}
