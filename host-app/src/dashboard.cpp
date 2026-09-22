// 看板控件实现
#include "dashboard.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QValueAxis>
#include <QApplication>
#include <QSound>

static QLabel *makeCard(const QString &title, QLabel **valueOut, QGridLayout *g,
                        int row, int col)
{
    auto *box = new QLabel;
    box->setFrameStyle(QFrame::Panel | QFrame::Raised);
    box->setAlignment(Qt::AlignCenter);
    auto *inner = new QVBoxLayout(box);
    auto *t = new QLabel(title);
    t->setAlignment(Qt::AlignCenter);
    auto *v = new QLabel("--");
    v->setAlignment(Qt::AlignCenter);
    v->setStyleSheet("font-size:26px;font-weight:bold;");
    inner->addWidget(t);
    inner->addWidget(v);
    g->addWidget(box, row, col);
    if (valueOut) *valueOut = v;
    return box;
}

Dashboard::Dashboard(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void Dashboard::setupUi()
{
    auto *root = new QVBoxLayout(this);

    m_lblAlarm = new QLabel("状态正常");
    m_lblAlarm->setAlignment(Qt::AlignCenter);
    m_lblAlarm->setStyleSheet("font-size:20px;font-weight:bold;padding:8px;"
                              "background:#2e7d32;color:white;border-radius:6px;");
    root->addWidget(m_lblAlarm);

    auto *grid = new QGridLayout;
    makeCard("心率 (次/分)",  &m_lblHr,     grid, 0, 0);
    makeCard("呼吸 (次/分)",  &m_lblResp,   grid, 0, 1);
    makeCard("体温 (℃)",      &m_lblTemp,   grid, 0, 2);
    makeCard("存在状态",      &m_lblPeople, grid, 0, 3);
    makeCard("室温 (℃)",      &m_lblEnv,    grid, 1, 0);
    makeCard("湿度 (%RH)",    &m_lblHumi,   grid, 1, 1);
    makeCard("CO (%)",        &m_lblCo,     grid, 1, 2);
    root->addLayout(grid);

    // 实时曲线：心率 + 呼吸
    m_hrSeries   = new QLineSeries; m_hrSeries->setName("心率");
    m_respSeries = new QLineSeries; m_respSeries->setName("呼吸");
    m_chart = new QChart;
    m_chart->addSeries(m_hrSeries);
    m_chart->addSeries(m_respSeries);
    m_chart->createDefaultAxes();
    m_chart->setTitle("生命体征实时趋势");
    m_chart->legend()->setVisible(true);
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    root->addWidget(m_chartView, 1);
}

void Dashboard::appendPoint(QLineSeries *s, double x, double y)
{
    s->append(x, y);
    // 保留最近 120 个点，滚动窗口
    while (s->count() > 120) s->remove(0);
    auto *ax = qobject_cast<QValueAxis *>(m_chart->axisX(s));
    if (ax && s->count() > 1) {
        double minx = s->at(0).x(), maxx = s->at(s->count() - 1).x();
        ax->setRange(minx, maxx > minx ? maxx : minx + 1);
    }
}

void Dashboard::setAlarm(bool on, const QString &reason)
{
    if (on) {
        m_lblAlarm->setText("⚠ 报警: " + reason);
        m_lblAlarm->setStyleSheet("font-size:20px;font-weight:bold;padding:8px;"
                                  "background:#c62828;color:white;border-radius:6px;");
        QApplication::beep();
    } else {
        m_lblAlarm->setText("状态正常");
        m_lblAlarm->setStyleSheet("font-size:20px;font-weight:bold;padding:8px;"
                                  "background:#2e7d32;color:white;border-radius:6px;");
    }
}

void Dashboard::updateData(const GuardianFrame &f)
{
    m_lblHr->setText(QString::number(f.heartRate));
    m_lblResp->setText(QString::number(f.respRate));
    m_lblTemp->setText(QString::number(f.bodyTemp, 'f', 1));
    m_lblEnv->setText(QString::number(f.envTemp, 'f', 1));
    m_lblHumi->setText(QString::number(f.envHumi));
    m_lblCo->setText(QString::number(f.co));
    m_lblPeople->setText(f.people ? "有人" : "无人");

    m_x += 1.0;
    appendPoint(m_hrSeries, m_x, f.heartRate);
    appendPoint(m_respSeries, m_x, f.respRate);

    // 报警位图（与固件 ALARM_* 一致）
    QString reason;
    if (f.alarmStatus & 0x01) reason += "心率过快 ";
    if (f.alarmStatus & 0x02) reason += "体温过高 ";
    if (f.alarmStatus & 0x04) reason += "CO超标 ";
    if (f.alarmStatus & 0x08) reason += "摔倒 ";
    if (f.alarmStatus & 0x10) reason += "SOS ";
    setAlarm(f.alarmStatus != 0 || f.fall, reason.trimmed());
}
