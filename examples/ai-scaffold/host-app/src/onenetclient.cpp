// OneNet 云平台客户端实现
#include "onenetclient.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>

// OneNet 多协议接入数据点 API（按实际平台版本调整）
static const char *API_BASE = "https://api.heclouds.com/devices/%2/datapoints";

// 物模型标识符（须与固件 onenet_json 一致）
static const char *DS[] = {
    "HeartRate", "RespiratoryRate", "People", "Fall",
    "MLX90614", "DHT11_T", "DHT11_H", "MQ7"
};

OneNetClient::OneNetClient(QObject *parent) : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &OneNetClient::onFetch);
}

void OneNetClient::configure(const QString &productId, const QString &deviceName,
                             const QString &apiKey)
{
    m_productId = productId;
    m_deviceName = deviceName;
    m_apiKey = apiKey;
}

void OneNetClient::start(int intervalMs) { m_timer.start(intervalMs); onFetch(); }
void OneNetClient::stop() { m_timer.stop(); }

QString OneNetClient::urlFor(const QString &datastream) const
{
    // 拉取某数据流最新一个点: ?datastream_id=xxx&limit=1
    return QString(API_BASE).arg(m_deviceName)
           + "?datastream_id=" + datastream + "&limit=1";
}

void OneNetClient::onFetch()
{
    if (m_apiKey.isEmpty() || m_deviceName.isEmpty()) return;
    m_pending = GuardianFrame();
    m_left = int(sizeof(DS) / sizeof(DS[0]));

    for (const char *ds : DS) {
        QNetworkRequest req{QUrl(urlFor(QString(ds)))};
        req.setRawHeader("api-key", m_apiKey.toUtf8());
        QNetworkReply *reply = m_nam.get(req);
        reply->setProperty("ds", QString(ds));
        connect(reply, &QNetworkReply::finished, this, &OneNetClient::onReply);
    }
}

void OneNetClient::onReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        if (--m_left <= 0) emit dataUpdated(m_pending);
        return;
    }

    const QString ds = reply->property("ds").toString();
    const QByteArray body = reply->readAll();
    // 解析 OneNet 返回 JSON，取 data.datastreams[0].datapoints[0].value
    double v = 0.0;
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject()) {
        QJsonObject data = doc.object().value("data").toObject();
        QJsonObject dp0 = data.value("datastreams").toArray().isEmpty()
                          ? QJsonObject()
                          : data.value("datastreams").toArray()[0].toObject();
        if (!dp0.isEmpty() && !dp0.value("datapoints").toArray().isEmpty())
            v = dp0.value("datapoints").toArray()[0].toObject().value("value").toDouble();
    }

    if      (ds == "HeartRate")       m_pending.heartRate = int(v);
    else if (ds == "RespiratoryRate") m_pending.respRate  = int(v);
    else if (ds == "People")          m_pending.people    = int(v);
    else if (ds == "Fall")            m_pending.fall      = int(v);
    else if (ds == "MLX90614")        m_pending.bodyTemp  = float(v);
    else if (ds == "DHT11_T")         m_pending.envTemp   = float(v);
    else if (ds == "DHT11_H")         m_pending.envHumi   = int(v);
    else if (ds == "MQ7")             m_pending.co        = int(v);

    if (--m_left <= 0) emit dataUpdated(m_pending);
}
