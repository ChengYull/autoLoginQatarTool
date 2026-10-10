#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QWidget>

class QPropertyAnimation;

/* 现代化的滑动开关：全部由 QPainter 绘制，不依赖任何图片资源 */
class ToggleSwitch : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal knobPosition READ knobPosition WRITE setKnobPosition)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    bool isChecked() const { return checked; }
    qreal knobPosition() const { return position; }
    void setKnobPosition(qreal value);

    QSize sizeHint() const override;

public slots:
    void setChecked(bool value);

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool checked = false;
    qreal position = 0.0;   // 0.0 = 关闭，1.0 = 打开
    QPropertyAnimation *animation = nullptr;
};

#endif // TOGGLESWITCH_H
