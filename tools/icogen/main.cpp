/* SVG -> 多尺寸 ICO 转换工具
 *
 * exe 的内嵌图标由 RC_ICONS 指向 assets/icons/app.ico，而 app.ico 无法由 SVG 在编译期直接生成，
 * 因此修改 assets/icons/app.svg 后需要重新运行本工具导出一次。用法（在 tools/icogen 下）：
 *
 *     qmake icogen.pro -spec win32-g++ CONFIG+=release
 *     mingw32-make -j8
 *     release\icogen.exe [可选的预览图路径]
 */

#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QDebug>
#include <QList>

#ifndef ASSETS_DIR
#define ASSETS_DIR "."
#endif

namespace {

struct Entry {
    int size;
    QByteArray png;
};

/* 把图片编码成 PNG 字节流 */
QByteArray toPng(const QImage &image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const QString assetDir = QString::fromUtf8(ASSETS_DIR);
    const QString svgPath = assetDir + "/app.svg";
    const QString icoPath = assetDir + "/app.ico";

    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        qWarning() << "无法解析 SVG:" << svgPath;
        return 1;
    }

    /* 逐尺寸渲染，256 及以上的尺寸在 ICO 中以 0 表示 */
    QList<Entry> entries;
    for (int size : {16, 20, 24, 32, 40, 48, 64, 128, 256}) {
        QImage image(size, size, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter, QRectF(0, 0, size, size));
        painter.end();

        entries.append(Entry{size, toPng(image)});
        if (size == 256 && argc > 1) image.save(QString::fromLocal8Bit(argv[1]), "PNG");
    }

    /* 按 ICO 格式组装：6 字节目录头 + 每项 16 字节目录项 + 各尺寸 PNG 数据 */
    QByteArray data;
    const auto put16 = [&data](quint16 value) {
        data.append(static_cast<char>(value & 0xFF));
        data.append(static_cast<char>((value >> 8) & 0xFF));
    };
    const auto put32 = [&data](quint32 value) {
        for (int i = 0; i < 4; ++i) data.append(static_cast<char>((value >> (8 * i)) & 0xFF));
    };

    put16(0);  // reserved
    put16(1);  // type = 1 表示图标
    put16(static_cast<quint16>(entries.size()));

    quint32 offset = 6 + 16 * static_cast<quint32>(entries.size());
    for (const Entry &entry : entries) {
        const char dimension = static_cast<char>(entry.size >= 256 ? 0 : entry.size);
        data.append(dimension);                       // 宽
        data.append(dimension);                       // 高
        data.append(static_cast<char>(0));            // 调色板数量
        data.append(static_cast<char>(0));            // reserved
        put16(1);                                     // 颜色平面数
        put16(32);                                    // 位深
        put32(static_cast<quint32>(entry.png.size()));
        put32(offset);
        offset += static_cast<quint32>(entry.png.size());
    }
    for (const Entry &entry : entries) data.append(entry.png);

    QFile file(icoPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法写入:" << icoPath;
        return 1;
    }
    file.write(data);
    file.close();

    qDebug() << "已生成" << icoPath << ":" << data.size() << "字节," << entries.size() << "个尺寸";
    return 0;
}
