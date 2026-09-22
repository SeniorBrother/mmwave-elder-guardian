// OTA 面板：选择本地固件 bin，触发 OneNet FOTA 下发，显示进度/版本
#ifndef OTAPANEL_H
#define OTAPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>

class OtaPanel : public QWidget
{
    Q_OBJECT
public:
    explicit OtaPanel(QWidget *parent = nullptr);

public slots:
    void setDeviceVersion(int ver);       // 从 Modbus/云端读到的固件版本
    void setProgress(int percent);
    void log(const QString &msg);

signals:
    void requestUpload(const QString &binPath, const QString &productId,
                       const QString &deviceName);

private slots:
    void onBrowse();
    void onUpload();

private:
    QLineEdit *m_binPath, *m_productId, *m_deviceName;
    QPushButton *m_btnBrowse, *m_btnUpload;
    QProgressBar *m_bar;
    QLabel *m_lblVer, *m_log;
};

#endif // OTAPANEL_H
