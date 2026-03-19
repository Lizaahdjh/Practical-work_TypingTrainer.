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
    , m_lineIndex(0)
    , m_charIndex(0)
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
    m_lessonTexts.clear();

    m_lessonTexts << QString(
        "The quick brown fox jumps over the lazy dog.\n"
        "Pack my box with five dozen liquor jugs.\n"
        "How vexingly quick daft zebras jump!\n"
        "The five boxing wizards jump quickly."
        );

    m_lessonTexts << QString(
        "1234567890 + - * / = % & @ # !\n"
        "Price: $12.99, discount: 15%.\n"
        "Call us: +1 (800) 555-0199.\n"
        "Email: support@example.com"
        );

    m_lessonTexts << QString(
        "the be to of and a in that have it.\n"
        "not on with he as you do at this but.\n"
        "from they we say her she or an will my.\n"
        "one all would there their what so up out if."
        );

    m_lessonTexts << QString(
        "about who get which go me when make can like.\n"
        "time no just him know take people into year your.\n"
        "good some could them see other than then now look.\n"
        "only come its over think also back after use two."
        );

    m_lessonTexts << QString(
        "def greet(name):\n"
        "    print('Hello, ' + name + '!')\n"
        "\n"
        "for i in range(5):\n"
        "    greet('User ' + str(i))"
        );

    m_lessonTexts << QString(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
        "Sed do eiusmod tempor incididunt ut labore et dolore magna.\n"
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco.\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit."
        );

    if (ui->comboLesson) {
        ui->comboLesson->clear();
        ui->comboLesson->addItem("Starter Text");
        ui->comboLesson->addItem("Numbers and Symbols");
        ui->comboLesson->addItem("Common Words Part 1");
        ui->comboLesson->addItem("Common Words Part 2");
        ui->comboLesson->addItem("Python Code Sample");
        ui->comboLesson->addItem("Lorem Ipsum");
    }

    if (ui->stackedWidget)
        ui->stackedWidget->setCurrentIndex(0);

    updateLessonDescription("Starter Text");
}

void MainWindow::loadLesson(int index)
{
    if (index < 0 || index >= m_lessonTexts.size()) {
        qWarning() << "loadLesson: index out of range:" << index;
        m_fullText  = QString();
        m_lines     = QStringList();
        m_lineIndex = 0;
        m_charIndex = 0;
        return;
    }

    m_fullText  = m_lessonTexts.at(index);
    m_lines     = QString(m_fullText).replace("\r\n", "\n").split('\n');
    m_lineIndex = 0;
    m_charIndex = 0;

    qDebug() << "Loaded lesson" << index
             << "| lines:" << m_lines.size()
             << "| first:" << (m_lines.isEmpty() ? "" : m_lines.first());
}

QString MainWindow::previousLine() const
{
    if (m_lineIndex > 0 && m_lineIndex - 1 < m_lines.size())
        return m_lines.at(m_lineIndex - 1);
    return QString();
}

QString MainWindow::currentLine() const
{
    if (m_lines.isEmpty() || m_lineIndex < 0 || m_lineIndex >= m_lines.size())
        return QString();
    return m_lines.at(m_lineIndex);
}

QString MainWindow::doneFragment() const
{
    const QString line = currentLine();
    if (line.isEmpty()) return QString();
    return line.left(qBound(0, m_charIndex, line.size()));
}

QString MainWindow::remainFragment() const
{
    const QString line = currentLine();
    if (line.isEmpty()) return QString();
    return line.mid(qBound(0, m_charIndex, line.size()));
}

void MainWindow::updateTrainingDisplay()
{
    const QString prev   = previousLine();
    const QString done   = doneFragment();
    const QString remain = remainFragment();

    QString display;
    if (!prev.isEmpty())
        display += "[prev] " + prev + "\n";
    display += "[curr] " + done + "|" + remain;

    ui->textDisplay->setPlainText(display);

    statusBar()->showMessage(
        QString("Line %1/%2  Char %3/%4  done='%5'  remain='%6'")
            .arg(m_lineIndex + 1)
            .arg(m_lines.size())
            .arg(m_charIndex)
            .arg(currentLine().size())
            .arg(done)
            .arg(remain)
        );
}

void MainWindow::on_btnNextChar_clicked()
{
    if (m_lines.isEmpty()) {
        qDebug() << "No text loaded.";
        return;
    }

    const QString line = currentLine();

    if (m_charIndex < line.size()) {
        // Крок вперед у поточному рядку
        ++m_charIndex;
    } else {
        // Рядок завершено — переходимо на наступний
        if (m_lineIndex + 1 < m_lines.size()) {
            ++m_lineIndex;
            m_charIndex = 0;
            qDebug() << "Next line:" << m_lineIndex;
        } else {
            statusBar()->showMessage("End of text!");
            qDebug() << "End of text.";
            return;
        }
    }

    updateTrainingDisplay();
}

void MainWindow::on_btnStartTraining_clicked()
{
    int idx = ui->comboLesson ? ui->comboLesson->currentIndex() : 0;
    loadLesson(idx);

    if (ui->pageTraining)
        ui->stackedWidget->setCurrentWidget(ui->pageTraining);
    else
        ui->stackedWidget->setCurrentIndex(2);

    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");

    updateTrainingDisplay();
}

void MainWindow::on_btnRestartTraining_clicked()
{
    int idx = ui->comboLesson ? ui->comboLesson->currentIndex() : 0;
    loadLesson(idx);

    ui->stackedWidget->setCurrentWidget(ui->pageTraining);
    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");

    updateTrainingDisplay();
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
                       "Created with Qt Widgets\n"
                       "Practical Work #7: Qt Core strings & collections");
}

void MainWindow::on_comboLesson_currentTextChanged(const QString &text)
{
    updateLessonDescription(text);
}

void MainWindow::updateLessonDescription(const QString &lesson)
{
    QString description;

    if (lesson == "Starter Text")
        description = "Basic text for beginners with simple words and sentences.";
    else if (lesson == "Numbers and Symbols")
        description = "Practice typing numbers and special characters.";
    else if (lesson == "Common Words Part 1")
        description = "Most frequently used English words - part 1.";
    else if (lesson == "Common Words Part 2")
        description = "Most frequently used English words - part 2.";
    else if (lesson == "Python Code Sample")
        description = "Type actual Python code with proper indentation.";
    else if (lesson == "Lorem Ipsum")
        description = "Classic Lorem Ipsum text for typing practice.";
    else
        description = "Select a lesson to start typing practice.";

    if (ui->labelLessonDescription)
        ui->labelLessonDescription->setText(description);
}

void MainWindow::highlightKey(const QString &key)
{
    QList<QPushButton*> allKeys = ui->virtualKeyboard->findChildren<QPushButton*>();
    for (QPushButton *btn : allKeys) {
        btn->setProperty("current", false);
        btn->style()->polish(btn);
    }

    QPushButton *keyButton = findChild<QPushButton*>("btn_" + key.toLower());
    if (keyButton) {
        keyButton->setProperty("current", true);
        keyButton->style()->polish(keyButton);
    }
}

void MainWindow::markKeyPressed(const QString &key, bool correct)
{
    QPushButton *keyButton = findChild<QPushButton*>("btn_" + key.toLower());
    if (keyButton) {
        keyButton->setProperty("pressed", true);
        keyButton->setProperty("error", !correct);
        keyButton->style()->polish(keyButton);

        QTimer::singleShot(200, [keyButton]() {
            keyButton->setProperty("pressed", false);
            keyButton->setProperty("error", false);
            keyButton->style()->polish(keyButton);
        });
    }
}
