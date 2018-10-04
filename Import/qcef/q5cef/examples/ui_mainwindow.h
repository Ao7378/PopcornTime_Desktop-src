/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.3.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionAbout;
    QAction *actionExit;
    QAction *actionSendMessage;
    QAction *actionLoadHtml;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *backButton;
    QPushButton *forwardButton;
    QLineEdit *lineEdit;
    QPushButton *reloadButton;
    QPushButton *stopButton;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QMenu *menuTests;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(800, 600);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        actionSendMessage = new QAction(MainWindow);
        actionSendMessage->setObjectName(QStringLiteral("actionSendMessage"));
        actionSendMessage->setEnabled(true);
        actionLoadHtml = new QAction(MainWindow);
        actionLoadHtml->setObjectName(QStringLiteral("actionLoadHtml"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        backButton = new QPushButton(centralwidget);
        backButton->setObjectName(QStringLiteral("backButton"));
        backButton->setEnabled(false);

        horizontalLayout->addWidget(backButton);

        forwardButton = new QPushButton(centralwidget);
        forwardButton->setObjectName(QStringLiteral("forwardButton"));
        forwardButton->setEnabled(false);

        horizontalLayout->addWidget(forwardButton);

        lineEdit = new QLineEdit(centralwidget);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setEnabled(false);

        horizontalLayout->addWidget(lineEdit);

        reloadButton = new QPushButton(centralwidget);
        reloadButton->setObjectName(QStringLiteral("reloadButton"));
        reloadButton->setEnabled(false);

        horizontalLayout->addWidget(reloadButton);

        stopButton = new QPushButton(centralwidget);
        stopButton->setObjectName(QStringLiteral("stopButton"));
        stopButton->setEnabled(false);

        horizontalLayout->addWidget(stopButton);


        verticalLayout->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 23));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        menuTests = new QMenu(menubar);
        menuTests->setObjectName(QStringLiteral("menuTests"));
        menuTests->setEnabled(true);
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menubar->addAction(menuTests->menuAction());
        menuFile->addAction(actionExit);
        menuHelp->addAction(actionAbout);
        menuTests->addAction(actionSendMessage);
        menuTests->addAction(actionLoadHtml);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "cefclient", 0));
        actionAbout->setText(QApplication::translate("MainWindow", "About ...", 0));
        actionExit->setText(QApplication::translate("MainWindow", "Exit", 0));
        actionSendMessage->setText(QApplication::translate("MainWindow", "SendMessage", 0));
        actionLoadHtml->setText(QApplication::translate("MainWindow", "LoadHtml", 0));
        backButton->setText(QApplication::translate("MainWindow", "Back", 0));
        forwardButton->setText(QApplication::translate("MainWindow", "Forward", 0));
        reloadButton->setText(QApplication::translate("MainWindow", "Reload", 0));
        stopButton->setText(QApplication::translate("MainWindow", "Stop", 0));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", 0));
        menuTests->setTitle(QApplication::translate("MainWindow", "Tests", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
