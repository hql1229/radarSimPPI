#include "radarwidget.h"
#include <cmath>
#include <QDebug>
#include <QRadialGradient>
#include <QRandomGenerator>

RadarWidget::RadarWidget(QWidget *parent)
    : QWidget(parent), currentAngle(0.0f), rng(std::random_device{}()), gammaDist(kShape, kScale) {
    // 初始化目标（添加RCS）
    Target fisherman;
    fisherman.distance = 5.0f;
    fisherman.angle = 45.0f;
    fisherman.visible = false;
    fisherman.lifetime = 0.0f;
    fisherman.maxLifetime = 15.0f;
    fisherman.rcs = 10.0f; // 渔船RCS
    targets.push_back(fisherman);

    Target cargoShip;
    cargoShip.distance = 3.0f;
    cargoShip.angle = 120.0f;
    cargoShip.visible = false;
    cargoShip.lifetime = 0.0f;
    cargoShip.maxLifetime = 15.0f;
    cargoShip.rcs = 100.0f; // 货船RCS
    targets.push_back(cargoShip);

    Target smallBoat;
    smallBoat.distance = 7.0f;
    smallBoat.angle = 270.0f;
    smallBoat.visible = false;
    smallBoat.lifetime = 0.0f;
    smallBoat.maxLifetime = 15.0f;
    smallBoat.rcs = 1.0f; // 小艇RCS
    targets.push_back(smallBoat);

    // 初始化固定地杂波（K分布）
    for (int i = 0; i < groundClutterCount; ++i) {
        GroundClutter gc;
        float gammaValue = gammaDist(rng);
        gc.amplitude = std::sqrt(gammaValue);
        gc.distance = std::min(maxRange * gc.amplitude / kScale, maxRange);
        gc.angle = QRandomGenerator::global()->bounded(360.0f);
        groundClutter.push_back(gc);
    }

    // 设置定时器
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RadarWidget::updateRadar);
    timer->start(50);
}

void RadarWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.fillRect(rect(), Qt::black);



    // 设置原点为中心
    painter.translate(width() / 2, height() / 2);
    float scale = qMin(width(), height()) / (2.0f * maxRange);

    // 绘制雷达圆环（移到地杂波之后）
    painter.setPen(Qt::green);
    for (int r = 2; r <= static_cast<int>(maxRange); r += 2) {
        painter.drawEllipse(QPointF(0, 0), r * scale, r * scale);
    }

    // 绘制固定地杂波（K分布）
    painter.setPen(Qt::NoPen);
    for (const GroundClutter &gc : groundClutter) {
        float x = gc.distance * scale * cos(gc.angle * M_PI / 180.0f);
        float y = gc.distance * scale * sin(gc.angle * M_PI / 180.0f);
        QColor groundColor = Qt::darkGray;
        float opacity = 0.1f + 0.4f * (gc.amplitude / kScale);
        groundColor.setAlphaF(std::min(opacity, 0.5f));
        painter.setBrush(groundColor);
        painter.drawEllipse(QPointF(x, y), 2, 2);
    }

    // 绘制扫描线
    painter.setPen(QPen(Qt::green, 2));
    float scanX = maxRange * scale * cos(currentAngle * M_PI / 180.0f);
    float scanY = maxRange * scale * sin(currentAngle * M_PI / 180.0f);
    painter.drawLine(QPointF(0, 0), QPointF(scanX, scanY));

    // 绘制目标回波（考虑RCS）
    for (const Target &target : targets) {
        if (target.visible && target.lifetime > 0) {
            float centerX = target.distance * scale * cos(target.angle * M_PI / 180.0f);
            float centerY = target.distance * scale * sin(target.angle * M_PI / 180.0f);

            // 根据RCS调整回波尺寸
            float rcsFactor = std::sqrt(target.rcs / rcsBase); // RCS归一化并开平方根
            float radialLength = pulseLength * scale / 2.0f * (1.0f + rcsFactor)*0.1; // 径向长度随RCS增加
            float tangentialWidth = target.distance * scale * (beamWidth * M_PI / 360.0f) * (1.0f + rcsFactor); // 切向宽度随RCS增加

            painter.save();
            painter.translate(centerX, centerY);
            painter.rotate(target.angle + 90.0f);

            float lifetimeOpacity = target.lifetime / target.maxLifetime;
            float rcsOpacity = 0.5f + 0.5f * std::min(target.rcs / rcsBase, 2.0f); // RCS影响初始透明度（0.5~1.0）
            float opacity = lifetimeOpacity * rcsOpacity; // 综合透明度
            QRadialGradient gradient(0, 0, qMax(radialLength, tangentialWidth));
            QColor centerColor = Qt::yellow;
            centerColor.setAlphaF(opacity);
            gradient.setColorAt(0, centerColor);
            gradient.setColorAt(1, Qt::transparent);
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(0, 0), tangentialWidth, radialLength);

            painter.restore();
        }
    }

    // 绘制动态杂波
    for (const Clutter &clutter : clutterPoints) {
        if (clutter.lifetime > 0) {
            float centerX = clutter.distance * scale * cos(clutter.angle * M_PI / 180.0f);
            float centerY = clutter.distance * scale * sin(clutter.angle * M_PI / 180.0f);

            painter.save();
            float opacity = clutter.lifetime / clutter.maxLifetime;
            QColor clutterColor;
            int colorIndex = QRandomGenerator::global()->bounded(3);
            switch (colorIndex) {
                case 0: clutterColor = Qt::lightGray; break;
                case 1: clutterColor = QColor(150, 200, 255); break;
                case 2: clutterColor = QColor(150, 255, 150); break;
            }
            clutterColor.setAlphaF(opacity * 0.6f);
            painter.setBrush(clutterColor);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(centerX, centerY), 2, 2);
            painter.restore();
        }
    }
}

void RadarWidget::updateRadar() {
    currentAngle += 1.0f;
    if (currentAngle >= 360.0f) {
        currentAngle = 0.0f;
    }

    // 更新目标寿命
    for (Target &target : targets) {
        if (target.lifetime > 0) {
            target.lifetime -= updateInterval;
            if (target.lifetime <= 0) {
                target.visible = false;
            }
        }

        float angleDiff = fabs(currentAngle - target.angle);
        if (angleDiff < beamWidth / 2.0f) {
            target.visible = true;
            target.lifetime = target.maxLifetime;
            qDebug() << "Target detected at angle:" << target.angle << "distance:" << target.distance << "RCS:" << target.rcs;
        }
    }

    // 更新动态杂波寿命并生成新杂波
    for (int i = 0; i < clutterPoints.size(); ) {
        clutterPoints[i].lifetime -= updateInterval;
        if (clutterPoints[i].lifetime <= 0) {
            clutterPoints.remove(i);
        } else {
            ++i;
        }
    }

    int clutterCount = QRandomGenerator::global()->bounded(maxClutterPerScan);
    for (int i = 0; i < clutterCount; ++i) {
        Clutter clutter;
        clutter.distance = QRandomGenerator::global()->bounded(maxRange);
        clutter.angle = currentAngle + (QRandomGenerator::global()->bounded(-100, 101) * beamWidth / 200.0f);
        clutter.lifetime = clutterMaxLifetime;
        clutter.maxLifetime = clutterMaxLifetime;
        clutterPoints.push_back(clutter);
    }

    update();
}
