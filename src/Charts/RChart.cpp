/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "RChart.h"

#include "Colors.h"
#include "TabView.h"

extern QString gc_RVersion;

RConsole::RConsole(Context *context, QWidget *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setFrameStyle(QFrame::NoFrame);
    document()->setMaximumBlockCount(1024);
    QPalette p = palette();
    p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
    p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(p);
    putData(GColor(CPLOTMARKER), QString(tr("R Console (%1)").arg(gc_RVersion)));
    putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "\n> ");

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    configChanged(0);
}

void
RConsole::configChanged(qint32)
{
    setStyleSheet(TabView::ourStyleSheet());
}


void RConsole::putData(QColor color, QString string)
{
    // change color...
    setTextColor(color);
    putData(string);
}


void RConsole::putData(QString data)
{
    insertPlainText(QString(data));

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void RConsole::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void RConsole::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
        break;

    // you can always go to the right
    case Qt::Key_Right:
        QTextEdit::keyPressEvent(e);
        break;

    // you can only delete or move left from past first character
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        if (textCursor().columnNumber() > 2) QTextEdit::keyPressEvent(e);
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
        QString line = currentLine();
        if (line.length() > 1) line = line.mid(2, line.length()-2);
        else line = "";

        putData("\n");

        if (line != "") {
            // lets run it
            //qDebug()<<"RUN:" << line;
            (*gc_RInside)["GC.athlete"] = context->athlete->cyclist.toStdString();
            (*gc_RInside)["GC.athlete.home"] = context->athlete->home->root().absolutePath().toStdString();

            try {
                SEXP ret = gc_RInside->parseEval(line.toStdString());

                // if this isn't an assignment then print the result
                if(!line.contains("<-")) Rcpp::print(ret);

                QStringList &response = gc_RCallbacks->getConsoleOutput();
                putData(GColor(CPLOTMARKER), response.join(""));
                response.clear();

            } catch(std::exception& ex) {

                putData(QColor(Qt::red), QString("%1\n").arg(QString(ex.what())));
                QStringList &response = gc_RCallbacks->getConsoleOutput();
                putData(QColor(Qt::red), response.join(""));
                response.clear();

            } catch(...) {

                putData(QColor(Qt::red), "error: general exception.\n");
                QStringList &response = gc_RCallbacks->getConsoleOutput();
                putData(QColor(Qt::red), response.join(""));
                response.clear();

            }

        }

        // next prompt
        putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "> ");
    }
    break;

    default:
        if (localEchoEnabled) QTextEdit::keyPressEvent(e);
        emit getData(e->text().toLocal8Bit());
    }
}

QString
RConsole::currentLine()
{
    return textCursor().block().text().trimmed();
}

void RConsole::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void RConsole::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void RConsole::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}

RChart::RChart(Context *context) : GcChartWindow(context)
{
    setControls(NULL);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2,0,2,2);
    setChartLayout(mainLayout);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->setHandleWidth(1);
    mainLayout->addWidget(splitter);

    console = new RConsole(context, this);
    splitter->addWidget(console);
    QWidget *widget = new QWidget(this);
    widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->addWidget(new QWidget);
}

