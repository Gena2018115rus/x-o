#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openglwidget.h"
#include "eth-lib/eth-lib.hpp"

template <typename T>
coord_t<T> coord_t<T>::operator+(const coord_t<T>& other) const
{
    return {{x() + other.x(), y() + other.y()}};
}

template <typename T>
coord_t<T> coord_t<T>::operator*(T n) const
{
    return {{x() * n, y() * n}};
}

template <typename T>
coord_t<T> coord_t<T>::operator-() const
{
    return {{-x(), -y()}};
}

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

OpenGLWidget::~OpenGLWidget()
{
}

void OpenGLWidget::initializeGL() // пофиксить чёрный экран на старых системах и в rdp!
{
    glClearColor(0.0, 0.0, 0.0, 0.0);

    shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader.vsh");
    shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader.fsh");

    if (!shaderProgram.link())
    {
        qWarning("Error: unable to link a shader program.");
        return;
    }

    vertCoordAttr = shaderProgram.attributeLocation("vertCoord");
    vertColorAttr = shaderProgram.attributeLocation("vertColor");
}

size_t& line_t::len()        { return first ; }
content_t& line_t::content() { return second; }
const size_t& line_t::len()        const { return first ; }
const content_t& line_t::content() const { return second; }

bool& cost_t::chance()        { return std::get<0>(*this); }
line_t& cost_t::longestLine() { return std::get<1>(*this); }
const line_t& cost_t::longestLine() const { return std::get<1>(*this); }
size_t& cost_t::sum()         { return std::get<2>(*this); }
size_t& cost_t::Xs_count()    { return std::get<3>(*this); }

std::string cost_t::to_str()
{
    return std::string("[chance: ")
         + std::to_string(chance())
         + ", longestLine: [len: "
         + std::to_string(longestLine().len())
         + ", content: "
         + (longestLine().content() == EMPTY ? 'E' : longestLine().content() == O ? 'O' : 'X')
         + "], sum: "
         + std::to_string(sum())
         + ", Xs_count: "
         + std::to_string(Xs_count())
         + ']';
}

      cost_t& cost_with_icoord_t::cost ()       { return first ; }
      icoord& cost_with_icoord_t::coord()       { return second; }
const cost_t& cost_with_icoord_t::cost () const { return first ; }
const icoord& cost_with_icoord_t::coord() const { return second; }


const double pi = acos(-1);
extern MainWindow *wnd;

//#define sides (((MainWindow *)qApp->topLevelWidgets().at(0))->sides)
#define sides (wnd->sides) // убрал глобальные переменные, когда чей-то конструктор бросал

bool mouseStatusIsDown = 0, isClick;


//dcoord startMapCoord, startCursorCoord, mapCoord = {0.0, 0.0};
//#define mapCoord (((MainWindow *)qApp->topLevelWidgets().at(0))->mapCoord)
#define mapCoord (wnd->mapCoord)
//std::set<cost_with_icoord_t, std::greater<cost_with_icoord_t>> q;
//#define q (((MainWindow *)qApp->topLevelWidgets().at(0))->q)
#define q (wnd->q)
//std::map<icoord, cellInfo> map;
//#define map (((MainWindow *)qApp->topLevelWidgets().at(0))->map)
#define map (wnd->map)
//icoord last_click_coord;
//#define last_click_coord (((MainWindow *)qApp->topLevelWidgets().at(0))->last_click_coord)
#define last_click_coord (wnd->last_click_coord)
//icoord startCursorICoord;

bool stepX = 1, waiting = 1;
extern bool botEnabled, onlineMode;
extern int N4Win;
extern std::string out_buf;
extern std::mutex out_buf_mtx, g_client_mtx, g_connection2host_mtx;
extern client_ref_t *g_client;
extern client_t *g_connection2host;
dcoord cell_size;

line_t getRay(icoord coord, icoord side)
{
    line_t res{};
    if (icoord checking_coord = coord + side; map.count(checking_coord))
        if (res.content() = map[checking_coord].content; res.content())
            do
            {
                ++res.len();
                checking_coord = checking_coord + side;
            } while (map.count(checking_coord) && map[checking_coord].content == res.content());
    return res;
}

cost_t calcCost(icoord coord) // не создавать новых клеток и вернуть cost, а не записывать в map
{
    if (map.count(coord)) // created
        if (map[coord].content) // used
            return {{false, {}, 0, 0}}; // can i remove it from q?

    cost_t res{{true, {}, 0, 0}};
    std::for_each(sides.cbegin() + 1, sides.cend() - 4, [&res, &coord](auto&& side)
    {
        auto p_ray{getRay(coord, side)}, n_ray{getRay(coord, -side)};

        line_t line;
        if (p_ray.content() == n_ray.content())
        {
            line.len() = p_ray.len() + n_ray.len();
            line.content() = p_ray.content();
        }
        else line = std::max(p_ray, n_ray);
        if (res.longestLine() < line) res.longestLine() = line;

        res.sum() += p_ray.len() + n_ray.len();

        if (p_ray.content() == X) res.Xs_count() += p_ray.len();
        if (n_ray.content() == X) res.Xs_count() += n_ray.len();
    });
//    std::cout << "new cost: " << res.to_str() << std::endl;
    return res;
}

void setCost(icoord coord, cost_t new_cost)
{
    if (!map.count(coord)) map.insert({coord, {EMPTY, new_cost}});

    cost_with_icoord_t key{{map[coord].cost, coord}};

    map[coord].cost = new_cost;

    if (q.count(key)) q.erase(key);

    key.cost() = new_cost;

    q.insert(key);
}

void setContent(icoord coord, content_t new_content)
{
    if (!map.count(coord))
        q.insert({{map.insert({coord, {new_content, calcCost(coord)}}).first->second.cost, coord}});
    else
        map[coord].content = new_content;

    setCost(coord, {{false, {}, 0, 0}});
    // ^v соединить заюзав sides[0]   getRay может потребовать изменений, возможно, слишком больших
    std::for_each(sides.cbegin() + 1, sides.cend(), [&coord](auto&& side)
    {
        icoord need_recalcCost = coord + side * (long long)(1 + getRay(coord, side).len());
        setCost(need_recalcCost, calcCost(need_recalcCost));
    });
}

cellInfo getCellInfo(icoord coord)
{
    if (!map.count(coord))
//        q.insert({{map.insert({coord, {EMPTY, calcCost(coord)}}).first->second.cost, coord}});
        return {EMPTY, {{true, {}, 0, 0}}};

    return map[coord];
}

dcoord i2dcoord(icoord coord)
{
    return {{
        (double)(coord.x() * 2) / wnd->geometry().width() - 1,
        -((double)(coord.y() * 2) / wnd->geometry().height() - 1)
    }};
}

dcoord MainWindow::getCursorCoord()
{
    return i2dcoord({{this->cursor().pos().rx() - this->geometry().x(),
                      this->cursor().pos().ry() - this->geometry().y()}});
}

std::pair<double, double> myModN(double val, double N)
{
    double second = std::modf(val / N, &val);
    return {val * N, second * N};
}

void OpenGLWidget::drawLines(std::vector<dcoord> points, double r, double g, double b)
{
    std::vector<float> coords/*(points.size() * 2)*/;
    for (auto&& point : std::move(points))
    {
        coords.push_back(point.x());
        coords.push_back(point.y());
    }

    shaderProgram.setAttributeArray(vertCoordAttr, coords.data(), 2);
    shaderProgram.setAttributeValue(vertColorAttr, r, g, b);

    shaderProgram.enableAttributeArray(vertCoordAttr);

    glDrawArrays(GL_LINES, 0, points.size());

    shaderProgram.disableAttributeArray(vertCoordAttr);
}

void OpenGLWidget::drawLineLoop(std::vector<dcoord> points, double r, double g, double b)
{
    std::vector<float> coords/*(points.size() * 2)*/;
    for (auto&& point : std::move(points))
    {
        coords.push_back(point.x());
        coords.push_back(point.y());
    }

    shaderProgram.setAttributeArray(vertCoordAttr, coords.data(), 2);
    shaderProgram.setAttributeValue(vertColorAttr, r, g, b);

    shaderProgram.enableAttributeArray(vertCoordAttr);

    glDrawArrays(GL_LINE_LOOP, 0, points.size());

    shaderProgram.disableAttributeArray(vertCoordAttr);
}

void OpenGLWidget::drawContent(icoord coord, double r, double g, double b)
{
    std::vector<dcoord> points;
    switch (getCellInfo(coord).content)
    {
        case X:
        {
            points.push_back({{mapCoord.x() + cell_size.x() * (coord.x() + 0.2),
                               mapCoord.y() + cell_size.y() * (coord.y() + 0.2)}});
            points.push_back({{mapCoord.x() + cell_size.x() * (coord.x() + 0.8),
                               mapCoord.y() + cell_size.y() * (coord.y() + 0.8)}});

            points.push_back({{mapCoord.x() + cell_size.x() * (coord.x() + 0.2),
                               mapCoord.y() + cell_size.y() * (coord.y() + 0.8)}});
            points.push_back({{mapCoord.x() + cell_size.x() * (coord.x() + 0.8),
                               mapCoord.y() + cell_size.y() * (coord.y() + 0.2)}});

            drawLines(std::move(points), r, g, b);
        }
        break;
        case O:
        {
            for (size_t i = 0; i < 120; ++i)
                points.push_back({{mapCoord.x() + cell_size.x() * ((double)coord.x() + 0.5 + 0.3 * cos(pi * 2 * i / 120)),
                                   mapCoord.y() + cell_size.y() * ((double)coord.y() + 0.5 + 0.3 * sin(pi * 2 * i / 120))}});

            drawLineLoop(std::move(points), r, g, b);
        }
        default: {}
    }
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shaderProgram.bind())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if defined(GL_LINE_SMOOTH)
    glEnable(GL_LINE_SMOOTH);
#endif

    {
        std::vector<dcoord> points;

        for (double y = -(long long)(1.0 / cell_size.y()) * cell_size.y() - cell_size.y(); y <= 1.0 + cell_size.y(); y += cell_size.y()) // горизонтальные
        {
            points.push_back({{-1.0, myModN(mapCoord.y(), cell_size.y()).second + y}});
            points.push_back({{ 1.0, myModN(mapCoord.y(), cell_size.y()).second + y}});
        }

        for (double x = -(long long)(1.0 / cell_size.x()) * cell_size.x() - cell_size.x(); x <= 1.0 + cell_size.x(); x += cell_size.x()) // вертикальные
        {
            points.push_back({{myModN(mapCoord.x(), cell_size.x()).second + x,  1.0}});
            points.push_back({{myModN(mapCoord.x(), cell_size.x()).second + x, -1.0}});
        }

        drawLines(std::move(points), 0.0, 0.50, 0.0);
    }

    //               надо floor(                                                          ) и в других местах ?                                   //  v надо прибавлять больше на бОльших расстояниях. найти другой способ
    for     (long long bottom = (myModN(-mapCoord.y(), cell_size.y()).first - 1.0) / cell_size.y() - 1, y = bottom; y <= bottom + 2.0 / cell_size.y() + 1; ++y)
        for (long long left   = (myModN(-mapCoord.x(), cell_size.x()).first - 1.0) / cell_size.x() - 1, x = left  ; x <= left   + 2.0 / cell_size.x() + 1; ++x)
            drawContent({{x, y}}, 1.0, 1.0, 1.0);

    drawContent(last_click_coord, 1.0, 0.0, 0.0);
}

OpenGLWidget *MainWindow::get_openGLWidget()
{
    return ui->openGLWidget;
}

icoord MainWindow::d2icoord(dcoord coord)
{
    return {{(long long)floor(coord.x() / cell_size.x()),
             (long long)floor(coord.y() / cell_size.y())}};
}

bool isTouch;

void MainWindow::mousePressEvent(QMouseEvent *event)
{
//    std::cout << "mouse down " << event->x() << 'x' << event->y() << std::endl;
    mouseStatusIsDown = 1;
//    startMapCoord = mapCoord;
//    startCursorCoord = getCursorCoord();

    isClick = 1;
    isTouch = 0;

    startCursorICoord = d2icoord(-mapCoord + getCursorCoord());
}

size_t calcLongestLineLen(icoord coord) // if (map[coord].content == map[coord].cost.longestLine.content) return map[coord].cost.longestLine.content + 1; else return 0;
{
    size_t res = 0;
    std::for_each(sides.cbegin() + 1, sides.cend() - 4, [&res, &coord, checking_content = map[coord].content](auto&& side)
    {
        size_t sum = 0;
        if (line_t ray{getRay(coord,  side)}; ray.content() == checking_content) sum += ray.len();
        if (line_t ray{getRay(coord, -side)}; ray.content() == checking_content) sum += ray.len();
        if (res < sum) res = sum;
    });
    return res + 1;
}

bool OpenGLWidget::myClickEvent(icoord coord)
{
//    std::cout << "cost: " << getCellInfo(coord).cost.to_str() << " N: " << calcLongestLineLen(coord) << std::endl;

    setContent(last_click_coord = coord, stepX ? X : O);

    if (onlineMode && !waiting)
    {
        std::string line = std::to_string(coord.x()) + '&' + std::to_string(coord.y());
        std::cout << line << std::endl;

        g_client_mtx.lock();
        out_buf_mtx.lock();
        out_buf += line + "\r\n";
        if (g_client)
        {
            if (g_client->write(out_buf)) {
                out_buf.clear();
            } else {
                std::cerr << "Проблема с отправкой данных из буфера клиенту" << std::endl;
                exit(-101);
            }
        }
        out_buf_mtx.unlock();
        g_client_mtx.unlock();

        g_connection2host_mtx.lock();
        if (g_connection2host)
            g_connection2host->write(line);
        g_connection2host_mtx.unlock();
    }
    waiting ^= 1;

    if (calcLongestLineLen(coord) >= size_t(N4Win))
    {
        QMessageBox::about(this, "Игра окончена", QString(stepX ? "Крест" : "Нол") + "ики победили!");
        return 1;
    }
    else
    {
        stepX ^= 1;
        return 0;
    }
}

icoord getBestCoord()
{
    for (const cost_with_icoord_t& el : q)
    {
        if (el.cost().longestLine().len() < std::size_t(N4Win - 1))
            break;

        if (el.cost().longestLine().content() == X)
            return el.coord();

        if (el.cost().longestLine().len() == std::size_t(N4Win - 1))
            break;
    }

    return q.cbegin()->coord();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
//    std::cout << "mouse up " << event->x() << 'x' << event->y() << std::endl;
    mouseStatusIsDown = 0;
    if (isTouch) return;


    static bool weHaveAWinner;
    if (isClick && !weHaveAWinner && (!onlineMode || !waiting))
    {
        auto&& click_coord = d2icoord(getCursorCoord() + -mapCoord);

//        std::cout << click_coord.x() << 'x' << click_coord.y() << std::endl;

        if (!getCellInfo(click_coord).content && !(weHaveAWinner = this->ui->openGLWidget->myClickEvent(click_coord)) && botEnabled)
        {
            // человек походил, не выиграл, очередь бота
            weHaveAWinner = ui->openGLWidget->myClickEvent(getBestCoord());
        }


//        printf("%lfx%lf\n", x, y);
        this->ui->openGLWidget->update();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
//    std::cout << event->x() << 'x' << event->y() << std::endl;

    if (isTouch) return;

    if (isClick && startCursorICoord != d2icoord(getCursorCoord() + -mapCoord))
    {
        startMapCoord = mapCoord;
        startCursorCoord = getCursorCoord();
        isClick = 0;
    }

    if (!isClick)
    {
        mapCoord = startMapCoord + getCursorCoord() + -startCursorCoord;
        this->ui->openGLWidget->update();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)/*, timer(new QTimer)*/
{
    ui->setupUi(this);
//    timer->setInterval(1000 / 60);
//    timer->setSingleShot(false);
////    QObject::connect(window(), MainWindow::resize);
//    QObject::connect(timer, &QTimer::timeout, [this]{

////        glFlush();

//        this->ui->openGLWidget->update();
////        this->ui->openGLWidget->repaint();
////        this->request_update();
////        QOpenGLContext *PGLC = QOpenGLContext::currentContext();
////        if (PGLC) PGLC->swapBuffers(PGLC->surface());
////        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
//    });
//    timer->start();
//    this->ui->openGLWidget->grabMouse();
//    this->ui->openGLWidget->setMouseTracking(1);

    cell_size.x() = 0.1;
    cell_size.y() = cell_size.x() * (double(geometry().width()) / double(geometry().height()));
}

MainWindow::~MainWindow()
{
//    delete timer;
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
//    std::cout << event->size().width() << 'x' << event->size().height() << std::endl;
//    ui->openGLWidget->resize(event->size().width(), event->size().height());
    if (static bool isFirstCall = true; isFirstCall)
    {
        isFirstCall = false;
    }
    else
    {
        cell_size.x() /= (double(event->size().width()) / double(ui->openGLWidget->geometry().width())); // камера уезжает
        cell_size.y() /= (double(event->size().height()) / double(ui->openGLWidget->geometry().height()));
    }
    ui->openGLWidget->setGeometry(ui->openGLWidget->geometry().x(), ui->openGLWidget->geometry().y(), event->size().width(), event->size().height());
}

void zoom(dcoord center, double coef)
{
/*    double old_size = cell_size,
           new_size = */cell_size = cell_size * coef/*,
           diff = new_size - old_size*/;
    mapCoord = mapCoord + -center;
    mapCoord = mapCoord * coef;
    mapCoord = mapCoord + center;
//    std::cout << "cell_size = " << cell_size << std::endl;
    wnd->get_openGLWidget()->update();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case '+':
        zoom(getCursorCoord(), 11.0 / 10.0);
        break;
    case '-':
        zoom(getCursorCoord(), 10.0 / 11.0);
    default: {}
    }
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    zoom(getCursorCoord(),  1.0 + double(event->delta()) / 1000.0);
}

bool pinchZoom(QTouchEvent *event)
{
    if (event->touchPoints().size() == 2) // можно сделать лучше?
    {
//        std::cout << '(' << event->touchPoints()[0].lastPos().x() << ", " << event->touchPoints()[0].lastPos().y() << ") ("
//                         << event->touchPoints()[1].lastPos().x() << ", " << event->touchPoints()[1].lastPos().y() << ") => ("
//                         << event->touchPoints()[0].    pos().x() << ", " << event->touchPoints()[0].    pos().y() << ") ("
//                         << event->touchPoints()[1].    pos().x() << ", " << event->touchPoints()[1].    pos().y() << ")"
//                         << std::endl;

        double x01   = event->touchPoints()[0].lastPos().x(),
               y01   = event->touchPoints()[0].lastPos().y(),
               x02   = event->touchPoints()[1].lastPos().x(),
               y02   = event->touchPoints()[1].lastPos().y(),
               x1    = event->touchPoints()[0].    pos().x(),
               y1    = event->touchPoints()[0].    pos().y(),
               x2    = event->touchPoints()[1].    pos().x(),
               y2    = event->touchPoints()[1].    pos().y(),
               k01   = x01 - x02,
               k02   = y01 - y02,
               k1    = x1  - x2 ,
               k2    = y1  - y2 ,
               diff0 = sqrt(k01 * k01 + k02 * k02),
               diff  = sqrt(k1  * k1  + k2  * k2 );

        zoom(i2dcoord({{(x1 + x2) / 2, (y1 + y2) / 2}}), diff / diff0); // сделать возможность пригвоздить центр    двигать двумя пальцами     вращение?

        isTouch = 1;
        return true;
    }
    else
    {
        event->ignore();
        return false;
    }
}

bool myTouchBeginEvent(QTouchEvent *event)
{
    std::cout << "Begin " << event->touchPoints().size() << std::endl;
    return false;
}

bool myTouchUpdateEvent(QTouchEvent *event)
{
//    std::cout << "Update " << std::endl;
    return pinchZoom(event);
}

bool myTouchEndEvent(QTouchEvent *event)
{
//    std::cout << "End " << event->touchPoints().size() << std::endl;
    return pinchZoom(event);
}

bool myTouchCancelEvent(QTouchEvent *event)
{
    std::cout << "Cancel " << event->touchPoints().size() << std::endl;
    return false;
}

bool OpenGLWidget::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::TouchBegin:
        return myTouchBeginEvent(static_cast<QTouchEvent *>(event)); // TODO: если вернулся 0, то goto default
    case QEvent::TouchUpdate:
        return myTouchUpdateEvent(static_cast<QTouchEvent *>(event));
    case QEvent::TouchEnd:
        return myTouchEndEvent(static_cast<QTouchEvent *>(event));
    case QEvent::TouchCancel:
        return myTouchCancelEvent(static_cast<QTouchEvent *>(event));
    default:
        return QOpenGLWidget::event(event);
    }
}
