// 看板控件：QtCharts 实时曲线 + 数字卡片 + 报警条
#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QLabel>
#include <QChartView>
#include <QLineSeries>
#include "modbusmaster.h"

QT_CHARTS_USE_NAMESPACE

class Dashboard : public QWidget
{
    Q_OBJECT
public:
    explicit Dashboard(QWidget *parent = nullptr);

public slots:
    void updateData(const GuardianFrame &f);

private:
    void setupUi();
    void setAlarm(bool on, const QString &reason);
    void appendPoint(QLineSeries *s, double x, double y);

    QLabel *m_lblHr, *m_lblResp, *m_lblTemp, *m_lblEnv, *m_lblHumi,
           *m_lblCo, *m_lblPeople, *m_lblAlarm;
    QLineSeries *m_hrSeries, *m_respSeries;
    QChart *m_chart;
    QChartView *m_chartView;
    double m_x = 0.0;
};

#endif // DASHBOARD_H
