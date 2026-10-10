#include "toggleswitch.h"

#include <QEasingCurve>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>

namespace {
constexpr int kWidth = 46;
constexpr int kHeight = 26;
constexpr int kMargin = 3;
const QColor kOffColor("#cbd5e1");
const QColor kOnColor("#2563eb");
} // namespace

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(kWidth, kHeight);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);

    animation = new QPropertyAnimation(this, "knobPosition", this);
    animation->setDuration(140);
    animation->setEasingCurve(QEasingCurve::InOutCubic);
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(kWidth, kHeight);
}

void ToggleSwitch::setKnobPosition(qreal value)
{
    position = qBound(0.0, value, 1.0);
    update();
}

void ToggleSwitch::setChecked(bool value)
{
    if (checked == value) return;
    checked = value;

    animation->stop();
    if (isVisible()) {
        animation->setStartValue(position);
        animation->setEndValue(checked ? 1.0 : 0.0);
        animation->start();
    } else {
        // 窗口尚未显示（初始化阶段），直接落位，避免出现多余动画
        setKnobPosition(checked ? 1.0 : 0.0);
    }

    emit toggled(checked);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 轨道颜色随滑块位置在“关/开”两色之间平滑过渡
    const QColor track(
        static_cast<int>(qRound(kOffColor.red()   + (kOnColor.red()   - kOffColor.red())   * position)),
        static_cast<int>(qRound(kOffColor.green() + (kOnColor.green() - kOffColor.green()) * position)),
        static_cast<int>(qRound(kOffColor.blue()  + (kOnColor.blue()  - kOffColor.blue())  * position)));

    const qreal radius = height() / 2.0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(track);
    painter.drawRoundedRect(QRectF(rect()), radius, radius);

    const qreal knobDiameter = height() - kMargin * 2.0;
    const qreal knobX = kMargin + position * (width() - knobDiameter - kMargin * 2.0);
    painter.setBrush(QColor("#ffffff"));
    painter.drawEllipse(QRectF(knobX, kMargin, knobDiameter, knobDiameter));
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
        setChecked(!checked);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
