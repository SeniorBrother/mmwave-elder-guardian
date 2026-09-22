// OneNet 云平台客户端（RESTful API 轮询数据点）
#ifndef ONENETCLIENT_H
#define ONENETCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>
#include "modbusmaster.h"   // 复用 GuardianFrame 作为统一数据结构

// 通过 OneNet 数据点 API 拉取最新值，转成 GuardianFrame 供看板统一显示。
// 文档: 多协议接入 -> 数据点 API (需 master-key / 鉴权头)
class OneNetClient : public QObject
{
    Q_OBJECT
public:
    explicit OneNetClient(QObject *parent = nullptr);

    void configure(const QString &productId, const QString &deviceName,
                   const QString &apiKey);
    void start(int intervalMs = 5000);
    void stop();

signals:
    void dataUpdated(const GuardianFrame &frame);
    void errorOccurred(const QString &msg);

private slots:
    void onFetch();
    void onReply();

private:
    QString urlFor(const QString &datastream) const;

    QNetworkAccessManager m_nam;
    QTimer m_timer;
    QString m_productId, m_deviceName, m_apiKey;
    GuardianFrame m_pending;    // 累积多个 datastream 的结果
    int m_left = 0;             // 还有几个请求未回
};

#endif // ONENETCLIENT_H
