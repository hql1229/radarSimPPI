#ifndef RADARWIDGET_H
#define RADARWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QVector>
#include <random>

class RadarWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateRadar();

private:
    float currentAngle;
    QTimer *timer;

    // 目标结构体（新增RCS）
    struct Target {
        float distance;  // 距离（km）
        float angle;     // 角度（度）
        bool visible;
        float lifetime;  // 剩余显示时间（秒）
        float maxLifetime; // 最大显示时间（秒）
        float rcs;       // 雷达截面积（m²）
    };
    QVector<Target> targets;

    // 动态杂波结构体
    struct Clutter {
        float distance;  // 距离（km）
        float angle;     // 角度（度）
        float lifetime;  // 剩余显示时间（秒）
        float maxLifetime; // 最大显示时间（秒）
    };
    QVector<Clutter> clutterPoints;

    // 固定地杂波结构体
    struct GroundClutter {
        float distance;  // 距离（km）
        float angle;     // 角度（度）
        float amplitude; // K分布生成的幅度
    };
    QVector<GroundClutter> groundClutter;

    // 雷达参数
    const float beamWidth = 2.0f;    // 波束宽度（度）
    const float pulseLength = 0.3f;  // 脉冲长度（km）
    const float maxRange = 10.0f;    // 最大探测距离（km）
    const float updateInterval = 0.05f; // 更新间隔（秒）
    const int maxClutterPerScan = 6; // 每次扫描最大动态杂波数
    const float clutterMaxLifetime = 2.0f; // 动态杂波最大寿命（秒）
    const int groundClutterCount = 5000; // 固定地杂波点数量
    const float kShape = 1.0f; // K分布形状参数
    const float kScale = 2.0f; // K分布尺度参数
    const float rcsBase = 10.0f; // RCS基准值，用于归一化

    // 随机数生成器
    std::mt19937 rng;
    std::gamma_distribution<float> gammaDist;
};

#endif // RADARWIDGET_H
