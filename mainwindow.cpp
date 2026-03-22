#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QRandomGenerator>
#include <QCoreApplication>

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
    m_lessonsDir = QCoreApplication::applicationDirPath() + "/lessons";

    if (ui->stackedWidget)
        ui->stackedWidget->setCurrentIndex(0);

    scanLessons();
}
void MainWindow::scanLessons()
{
    if (!ui->comboLesson) return;

    QString previousPath = ui->comboLesson->currentData().toString();

    ui->comboLesson->blockSignals(true);
    ui->comboLesson->clear();
    QDir dir(m_lessonsDir);
    if (!dir.exists()) {
        qWarning() << "lessons/ not found:" << m_lessonsDir;
        statusBar()->showMessage(
            "Folder 'lessons/' not found. Path: " + m_lessonsDir
            );
        ui->comboLesson->setEnabled(false);
        if (ui->btnStartTraining) ui->btnStartTraining->setEnabled(false);
        ui->comboLesson->blockSignals(false);
        return;
    }

    QStringList files = dir.entryList({"*.txt"}, QDir::Files, QDir::Name);

    if (files.isEmpty()) {
        qWarning() << "No .txt files in" << m_lessonsDir;
        statusBar()->showMessage("No lessons found in 'lessons/' folder.");
        ui->comboLesson->setEnabled(false);
        if (ui->btnStartTraining) ui->btnStartTraining->setEnabled(false);
        ui->comboLesson->blockSignals(false);
        return;
    }
    for (const QString &fileName : files) {
        QFileInfo fi(dir.filePath(fileName));
        QString title    = fi.baseName();
        QString fullPath = fi.absoluteFilePath();
        ui->comboLesson->addItem(title, fullPath);
        qDebug() << "Lesson:" << title << "->" << fullPath;
    }

    ui->comboLesson->setEnabled(true);
    if (ui->btnStartTraining) ui->btnStartTraining->setEnabled(true);

    int restoreIdx = 0;
    if (!previousPath.isEmpty()) {
        int found = ui->comboLesson->findData(previousPath);
        if (found >= 0) restoreIdx = found;
    }

    ui->comboLesson->blockSignals(false);
    ui->comboLesson->setCurrentIndex(restoreIdx);

    if (restoreIdx == 0) {
        QString path = ui->comboLesson->itemData(0).toString();
        if (!path.isEmpty()) loadLessonFromFile(path);
    }

    statusBar()->showMessage(
        "Found " + QString::number(files.size()) + " lesson(s)."
        );
}
void MainWindow::loadLessonFromFile(const QString &path)
{
    if (path.isEmpty()) {
        qWarning() << "loadLessonFromFile: empty path";
        return;
    }

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open:" << path << "-" << file.errorString();
        QMessageBox::warning(this, "File error",
                             "Cannot open lesson file:\n" + path +
                                 "\n\nReason: " + file.errorString()
                             );
        return;
    }

    QTextStream in(&file);
    m_fullText = in.readAll();
    file.close();

    m_lines     = QString(m_fullText).replace("\r\n", "\n").split('\n');
    m_lineIndex = 0;
    m_charIndex = 0;

    QFileInfo fi(path);
    if (ui->labelLessonDescription) {
        ui->labelLessonDescription->setText(
            fi.baseName() +
            "  |  " + QString::number(m_lines.size()) + " lines" +
            "  |  " + QString::number(m_fullText.length()) + " chars"
            );
    }

    qDebug() << "Loaded file:" << fi.baseName()
             << "| lines:" << m_lines.size()
             << "| chars:" << m_fullText.length();
}
void MainWindow::chooseRandomLesson()
{
    if (!ui->comboLesson) return;

    int count = ui->comboLesson->count();

    if (count <= 0) {
        statusBar()->showMessage("No lessons available for random selection.");
        return;
    }
    int idx = static_cast<int>(QRandomGenerator::global()->bounded(count));
    ui->comboLesson->setCurrentIndex(idx);

    statusBar()->showMessage("Random: " + ui->comboLesson->currentText());
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
void MainWindow::on_comboLesson_currentIndexChanged(int index)
{
    if (!ui->comboLesson || index < 0) return;

    QString path = ui->comboLesson->itemData(index).toString();
    if (!path.isEmpty())
        loadLessonFromFile(path);
}
void MainWindow::on_btnRandom_clicked()
{
    chooseRandomLesson();
}
void MainWindow::on_btnReload_clicked()
{
    scanLessons();
}
void MainWindow::on_btnStartTraining_clicked()
{
    if (m_lines.isEmpty()) {
        if (ui->comboLesson) {
            QString path = ui->comboLesson->currentData().toString();
            if (!path.isEmpty())
                loadLessonFromFile(path);
        }
    }
    m_lineIndex = 0;
    m_charIndex = 0;

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
    if (ui->comboLesson) {
        QString path = ui->comboLesson->currentData().toString();
        if (!path.isEmpty())
            loadLessonFromFile(path);
    }

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
                       "Practical Works #6–8\n\n"
                       "Place .txt lesson files in the 'lessons/' folder\n"
                       "next to the application executable.");
}
void MainWindow::updateLessonDescription(const QString &lesson)
{
    if (ui->labelLessonDescription)
        ui->labelLessonDescription->setText(lesson);
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
        ++m_charIndex;
    } else {
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
