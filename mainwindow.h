#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include <QTimer>
#include <QtOpenGL>
#include <QMouseEvent>
#include <QMessageBox>
#include <iostream>
#include <set>
#include <algorithm>
#include <QOpenGLShaderProgram>
#include <valarray>

template <typename T>
struct coord_t : std::pair<T, T>
{
    T& x() { return this->first; }
    T& y() { return this->second; }
    const T& x() const { return this->first; }
    const T& y() const { return this->second; }

    coord_t<T> operator+(const coord_t<T>& other) const;
    coord_t<T> operator*(T n) const;
    coord_t<T> operator-() const;
//    bool operator<(const coord_t<T>& other) const;
//    bool operator!=(const coord_t<T>& other) const;
};

using icoord = coord_t<long long>;
using dcoord = coord_t<double>;

enum content_t
{
    EMPTY = 0,
    O,
    X
};

class OpenGLWidget;

struct line_t : std::pair<size_t, content_t>
{
    size_t& len();
    const size_t& len() const;
    content_t& content();
    const content_t& content() const;
};

struct cost_t : std::tuple<bool, line_t, size_t, size_t>
{
    bool& chance();
    line_t& longestLine();
    const line_t& longestLine() const;
    size_t& sum();
    size_t& Xs_count();

    std::string to_str();
};

struct cost_with_icoord_t : std::pair<cost_t, icoord>
{
          cost_t& cost ();
          icoord& coord();
    const cost_t& cost () const;
    const icoord& coord() const;
};

struct cellInfo
{
    content_t content;
    cost_t cost;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    dcoord getCursorCoord();
    icoord d2icoord(dcoord coord);
    OpenGLWidget *get_openGLWidget();

    std::array<coord_t<long long>, 9> sides
    {
        coord_t<long long>{{0, 0}},
        {{0, 1}}, {{1, 1}}, {{1, 0}}, {{1, -1}},
        {{0, -1}}, {{-1, -1}}, {{-1, 0}}, {{-1, 1}}
    };

    dcoord startMapCoord, startCursorCoord, mapCoord = {{0.0, 0.0}};
    std::set<cost_with_icoord_t, std::greater<cost_with_icoord_t>> q;
    std::map<icoord, cellInfo> map;
    icoord last_click_coord;
    icoord startCursorICoord;

private:

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
//    QTimer *timer;
};
#endif // MAINWINDOW_H
