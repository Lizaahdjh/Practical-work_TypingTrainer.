#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("TypingTrainer");

    setupInitialData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupInitialData()
{
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(0);
    }

    if (ui->comboLesson) {
        ui->comboLesson->clear();
        ui->comboLesson->addItem("Starter Text");
        ui->comboLesson->addItem("Numbers and Symbols");
        ui->comboLesson->addItem("Common Words Part 1");
        ui->comboLesson->addItem("Common Words Part 2");
        ui->comboLesson->addItem("Python Code Sample");
        ui->comboLesson->addItem("Lorem Ipsum");
    }

    updateLessonDescription("Starter Text");
}

void MainWindow::on_btnStartTraining_clicked()
{
    QWidget* trainingPage = ui->pageTraining;
    if (trainingPage) {
        ui->stackedWidget->setCurrentWidget(trainingPage);
        qDebug() << "Switched to training page";
    } else {
        ui->stackedWidget->setCurrentIndex(2);
        qDebug() << "Using index 2";
    }

    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");
}

void MainWindow::on_btnRestartTraining_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageTraining);

    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");
}

void MainWindow::on_btnReturnToMain_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageStart);
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v1.0\n\n"
                       "A simple application to improve your typing skills.\n\n"
                       "Created with Qt Widgets");
}

void MainWindow::on_comboLesson_currentTextChanged(const QString &text)
{
    updateLessonDescription(text);
}

void MainWindow::updateLessonDescription(const QString &lesson)
{
    QString description;

    if (lesson == "Starter Text") {
        description = "Basic text for beginners with simple words and sentences.";
    } else if (lesson == "Numbers and Symbols") {
        description = "Practice typing numbers and special characters.";
    } else if (lesson == "Common Words Part 1") {
        description = "Most frequently used English words - part 1.";
    } else if (lesson == "Common Words Part 2") {
        description = "Most frequently used English words - part 2.";
    } else if (lesson == "Python Code Sample") {
        description = "Type actual Python code with proper indentation.";
    } else if (lesson == "Lorem Ipsum") {
        description = "Classic Lorem Ipsum text for typing practice.";
    } else {
        description = "Select a lesson to start typing practice.";
    }

    if (ui->labelLessonDescription) {
        ui->labelLessonDescription->setText(description);
    }
}

void MainWindow::highlightKey(const QString &key)
{
    QList<QPushButton*> allKeys = ui->virtualKeyboard->findChildren<QPushButton*>();
    for(QPushButton* btn : allKeys) {
        btn->setProperty("current", false);
        btn->style()->polish(btn);
    }

    QPushButton* keyButton = findChild<QPushButton*>("btn_" + key.toLower());
    if(keyButton) {
        keyButton->setProperty("current", true);
        keyButton->style()->polish(keyButton);
    }
}

void MainWindow::markKeyPressed(const QString &key, bool correct)
{
    QPushButton* keyButton = findChild<QPushButton*>("btn_" + key.toLower());
    if(keyButton) {
        if(correct) {
            keyButton->setProperty("pressed", true);
            keyButton->setProperty("error", false);
        } else {
            keyButton->setProperty("pressed", true);
            keyButton->setProperty("error", true);
        }
        keyButton->style()->polish(keyButton);

        QTimer::singleShot(200, [keyButton]() {
            keyButton->setProperty("pressed", false);
            keyButton->setProperty("error", false);
            keyButton->style()->polish(keyButton);
        });
    }
}
