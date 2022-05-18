#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include "mainwindow.h"

class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:

    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();
    void initializeGL() override;
    void paintGL() override;
//    void resizeGL(int w, int h) override;
    bool event(QEvent *event) override;

    void drawLines(std::vector<dcoord> points, double r, double g, double b);
    void drawLineLoop(std::vector<dcoord> points, double r, double g, double b);
    void drawContent(icoord coord, double r, double g, double b);

    QOpenGLShaderProgram shaderProgram;
    int vertCoordAttr;
    int vertColorAttr;

public slots:
    bool myClickEvent(icoord e);
};

#endif // OPENGLWIDGET_H
