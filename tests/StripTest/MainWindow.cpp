/**
* Copyright (C) 2012-2013 Phonations
* License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
*/


#include <QAction>
#include <QFileDialog>
#include <QMenuBar>

#include <QInputDialog>

#include "MainWindow.h"


MainWindow::MainWindow(PhString file)
{
    resize(1280,360);
    createMenus();
    setWindowTitle(tr("Striptest"));
    _strip = new StripWindow(this, file);
    _strip->show();
}

void MainWindow::createMenus()
{
    // The tr("&string") at the begining of a string is for translation tools

    // Add a menu
    fileMenu = menuBar()->addMenu(tr("&File"));
    
    // Create a new Open action
    openAct = new QAction(tr("&Open"), this);
    // Set a shortcut to execute this action
    openAct->setShortcut(QKeySequence::Open);
    // Add the action to the file menu
    fileMenu->addAction(openAct);
    // Connect the action to a function
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile()));

    // Add a menu
    toolMenu = menuBar()->addMenu(tr("&Tools"));

    changeFontAct = new QAction(tr("&Change font..."), this) ;
    toolMenu->addAction(changeFontAct);
    connect(changeFontAct, SIGNAL(triggered()), this, SLOT(changeFont()));

    //Separate 2 parts in the same menu list
    toolMenu->addSeparator();

    exportRythmoAct = new QAction(tr("&Export next minute"), this) ;
    toolMenu->addAction(exportRythmoAct);
    exportRythmoAct->setShortcut(QKeySequence::Print);
    connect(exportRythmoAct, SIGNAL(triggered()), this, SLOT(exportRythomAsPNG()));

    //Separate 2 parts in the same menu list
    toolMenu->addSeparator();

    switchScroll = new QAction(tr("&Toggle scroll"), this) ;
    toolMenu->addAction(switchScroll);
    connect(switchScroll, SIGNAL(triggered()), this, SLOT(switchScrolling()));

}

void MainWindow::openFile()
{
    // The strip should stop scrolling when during a file load
    _strip->setScroll(false);
    // Get the file name thanks to a QFileDialog
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open a script"),QDir::homePath(), "Script File (*.detx)");
    // If the file name isn't empty we load it
    if (!fileName.isEmpty())
        _strip->openFile(fileName);
}

void MainWindow::switchScrolling()
{
    _strip->toggleScrolling();
}

void MainWindow::changeFont()
{

    // This is a routine witch load Apple system TTF fonts

    // Set the location
    PhString sourceFolder = "/Library/Fonts/";
    QDir sourceDir(sourceFolder);
    // List all files
    QStringList files = sourceDir.entryList(QDir::Files);
    QStringList fonts;
    // browse files and select only TTF ones
    for(int i = 0; i< files.count(); i++)
    {
        if(files[i].split(".").at(1) == "ttf")
        {
            fonts.push_back(files[i].split(".").at(0));
        }
    }

    // i & j are used to display the current font in the QInputDialog
    int i = 0;
    int j = 0;
    PhString oldFont = _strip->getCurrentFont()->getFontName();
    for (auto it : fonts ){
        if (it.contains(oldFont))
            j = i;
        i++;
    }
    bool ok;
    QString item = QInputDialog::getItem(this, tr("Font Selection"),tr("fonts loaded"), fonts, j, false, &ok);
    if (true){
        PhString file = sourceFolder + item + ".ttf";
        _strip->setCurrentFont(file);
    }

}

void MainWindow::exportRythomAsPNG()
{
    _strip->getContext()->exportToPng();

    QMessageBox::about(this, "Info",
                       tr("Export ended well! \n") + tr("file(s) saved here : ") + QDir::homePath() + "/Phonations/") ;
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    // Call the resize of the OpenGL context
    _strip->resizeGL(this->width(), this->height());
}

void MainWindow::keyPressEvent( QKeyEvent *keyEvent )
{

    switch(keyEvent->key())
    {
    case Qt::Key_Space:
        _strip->changeScroll();
        break;
    case Qt::Key_Escape:
        close();
        break;
    case Qt::Key_Q:
        close();
        break;
    case Qt::Key_F10:
        toggleFullWindow();
        break;
    case Qt::Key_Right:
        _strip->setScroll(false);
        _strip->setXmove(this->width());
        break;
    case Qt::Key_Left:
        _strip->setScroll(false);
        _strip->setXmove(-this->width());
        break;
    default:
        _strip->keyPressEvent(keyEvent);
    }
}

void MainWindow::wheelEvent(QWheelEvent *wheel)
{
    QPoint numPixels = wheel->pixelDelta();
    _strip->setXmove(numPixels.x());
    _strip->setXmove(-numPixels.y());
}



void MainWindow::toggleFullWindow()
{
    if(this->isFullScreen())
        showNormal();
    else
        showFullScreen();
}
