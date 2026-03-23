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
#include <QKeyEvent>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_lineIndex(0)
    , m_charIndex(0)
    , m_totalChars(0)
    , m_correctChars(0)
    , m_errorCount(0)
    , m_isTrainingActive(false)
    , m_lastCharCount(0)
{
    ui->setupUi(this);
    this->setWindowTitle("TypingTrainer");

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTimer);

    installEventFilter(this);

    this->setFocusPolicy(Qt::StrongFocus);

    setupVirtualKeyboard();

    setupInitialData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupVirtualKeyboard()
{
    QList<QPushButton*> allButtons = ui->virtualKeyboard->findChildren<QPushButton*>();

    for (QPushButton *btn : allButtons) {
        connect(btn, &QPushButton::clicked, this, &MainWindow::onVirtualKeyPressed);
    }
}

void MainWindow::onVirtualKeyPressed()
{
    if (!m_isTrainingActive) return;

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString keyText = btn->text();

    if (keyText == "Space") {
        processKeyInput(" ");
        highlightKey("Space");
        markKeyPressed("Space", true);
    }
    else if (keyText == "←" || keyText == "Backspace") {
        handleBackspace();
        highlightKey("Backspace");
    }
    else if (keyText == "↵" || keyText == "Enter") {
        handleEnter();
        highlightKey("Enter");
    }
    else if (keyText == "Tab") {
        highlightKey("Tab");
    }
    else if (keyText == "Caps" || keyText == "Shift" || keyText == "Ctrl" ||
             keyText == "⇑" || keyText == "↑") {
        return;
    }
    else if (keyText.length() == 1) {
        processKeyInput(keyText);
        highlightKey(keyText);
        markKeyPressed(keyText, true);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (m_isTrainingActive && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        QString keyText = keyEvent->text();

        if (key == Qt::Key_Space) {
            processKeyInput(" ");
            highlightKey("Space");
            markKeyPressed("Space", true);
            return true;
        }
        else if (key == Qt::Key_Backspace) {
            handleBackspace();
            highlightKey("Backspace");
            return true;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            handleEnter();
            highlightKey("Enter");
            return true;
        }
        else if (key == Qt::Key_Tab) {
            highlightKey("Tab");
            return true;
        }
        else if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt ||
                 key == Qt::Key_CapsLock || key == Qt::Key_NumLock || key == Qt::Key_ScrollLock) {
            return true;
        }
        else if (!keyText.isEmpty() && keyText.at(0).isPrint()) {
            processKeyInput(keyText);
            highlightKey(keyText);
            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::processKeyInput(const QString &keyText)
{
    if (!m_isTrainingActive) return;

    QString currentLineText = currentLine();
    if (currentLineText.isEmpty()) {
        checkTrainingComplete();
        return;
    }

    if (m_charIndex >= currentLineText.length()) {
        return;
    }

    QChar expectedChar = currentLineText.at(m_charIndex);
    QChar inputChar = keyText.at(0);

    bool isCorrect = (inputChar == expectedChar);

    markKeyPressed(keyText, isCorrect);

    m_totalChars++;
    if (isCorrect) {
        m_correctChars++;
    } else {
        m_errorCount++;
    }

    if (isCorrect) {
        m_charIndex++;

        if (m_charIndex >= currentLineText.length()) {
            if (m_lineIndex + 1 < m_lines.size()) {
                m_lineIndex++;
                m_charIndex = 0;
            } else {
                checkTrainingComplete();
                updateTrainingDisplay();
                updateStats();
                return;
            }
        }
    }

    updateTrainingDisplay();
    updateStats();
    checkTrainingComplete();
}

void MainWindow::handleBackspace()
{
    if (!m_isTrainingActive) return;

    if (m_charIndex > 0) {
        m_charIndex--;
        updateTrainingDisplay();
    }
}

void MainWindow::handleEnter()
{
    if (!m_isTrainingActive) return;

    QString currentLineText = currentLine();
    if (m_charIndex >= currentLineText.length() && !currentLineText.isEmpty()) {
        if (m_lineIndex + 1 < m_lines.size()) {
            m_lineIndex++;
            m_charIndex = 0;
            updateTrainingDisplay();
        }
    }
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

    ui->comboLesson->blockSignals(true);
    ui->comboLesson->clear();

    QDir dir(m_lessonsDir);

    if (!dir.exists()) {
        qWarning() << "lessons/ not found:" << m_lessonsDir;
        statusBar()->showMessage("Folder 'lessons/' not found. Path: " + m_lessonsDir);
        ui->comboLesson->setEnabled(false);
        if (ui->btnStartTraining) ui->btnStartTraining->setEnabled(false);
        ui->comboLesson->blockSignals(false);
        return;
    }

    QStringList filters;
    filters << "*.txt";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

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
        QString title = fi.baseName();
        QString fullPath = fi.absoluteFilePath();
        ui->comboLesson->addItem(title, fullPath);
        qDebug() << "Lesson added:" << title << "->" << fullPath;
    }

    ui->comboLesson->setEnabled(true);
    if (ui->btnStartTraining) ui->btnStartTraining->setEnabled(true);

    ui->comboLesson->blockSignals(false);

    if (ui->comboLesson->count() > 0) {
        QString path = ui->comboLesson->itemData(0).toString();
        if (!path.isEmpty()) {
            loadLessonFromFile(path);
        }
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

    m_lines = m_fullText.split('\n', Qt::SkipEmptyParts);
    for (int i = 0; i < m_lines.size(); i++) {
        m_lines[i] = m_lines[i].trimmed();
        if (m_lines[i].endsWith('\r')) {
            m_lines[i].chop(1);
        }
    }

    m_lines.removeAll("");

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

    for (int i = 0; i < qMin(3, m_lines.size()); i++) {
        qDebug() << "Line" << i << ":" << m_lines[i];
    }
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
}

void MainWindow::on_comboLesson_currentIndexChanged(int index)
{
    if (!ui->comboLesson || index < 0) return;

    QVariant data = ui->comboLesson->itemData(index);
    if (data.isValid()) {
        QString path = data.toString();
        if (!path.isEmpty()) {
            loadLessonFromFile(path);
        }
    }
}

void MainWindow::on_btnRandom_clicked()
{
    chooseRandomLesson();
}

void MainWindow::on_btnReload_clicked()
{
    scanLessons();
}

void MainWindow::resetTraining()
{
    m_lineIndex = 0;
    m_charIndex = 0;
    m_totalChars = 0;
    m_correctChars = 0;
    m_errorCount = 0;
    m_lastCharCount = 0;

    if (ui->comboLesson && ui->comboLesson->count() > 0) {
        QVariant data = ui->comboLesson->currentData();
        if (data.isValid()) {
            QString path = data.toString();
            if (!path.isEmpty()) {
                loadLessonFromFile(path);
            }
        }
    }

    m_sessionTimer.start();
    m_speedTimer.start();
    m_timer->start(1000);
    m_isTrainingActive = true;

    this->setFocus();
}

void MainWindow::on_btnStartTraining_clicked()
{
    if (m_lines.isEmpty() && ui->comboLesson && ui->comboLesson->count() > 0) {
        QVariant data = ui->comboLesson->currentData();
        if (data.isValid()) {
            QString path = data.toString();
            if (!path.isEmpty()) {
                loadLessonFromFile(path);
            }
        }
    }

    resetTraining();

    if (ui->stackedWidget)
        ui->stackedWidget->setCurrentWidget(ui->pageTraining);

    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");

    updateTrainingDisplay();
}

void MainWindow::on_btnRestartTraining_clicked()
{
    resetTraining();

    ui->stackedWidget->setCurrentWidget(ui->pageTraining);
    ui->labelTime->setText("Time: 00:00");
    ui->labelSpeed->setText("Speed: 0 CPM");
    ui->labelAccuracy->setText("Accuracy: 0%");

    updateTrainingDisplay();
}

void MainWindow::on_btnReturnToMain_clicked()
{
    m_isTrainingActive = false;
    m_timer->stop();
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
                       "next to the application executable.\n\n"
                       "Current lessons:\n"
                       "- Starter text\n"
                       "- Common words\n"
                       "- Lorem Ipsum\n"
                       "- Numbers and symbols\n"
                       "- Python code\n\n"
                       "Features:\n"
                       "- Keyboard input with visual feedback\n"
                       "- Virtual keyboard highlighting\n"
                       "- Speed and accuracy tracking\n"
                       "- Backspace and Enter support");
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

QString MainWindow::getFormattedCurrentLine() const
{
    const QString done = doneFragment();
    const QString remain = remainFragment();

    QString formatted;

    for (int i = 0; i < done.length(); i++) {
        formatted += "<span style='color: #4CAF50;'>" + QString(done[i]) + "</span>";
    }

    if (!remain.isEmpty()) {
        formatted += "<span style='background-color: #FFD700; color: #000000; font-weight: bold;'>" + QString(remain[0]) + "</span>";
        if (remain.length() > 1) {
            formatted += "<span style='color: #888888;'>" + remain.mid(1) + "</span>";
        }
    }

    return formatted;
}

void MainWindow::updateTrainingDisplay()
{
    const QString prev = previousLine();
    const QString currentFormatted = getFormattedCurrentLine();

    QString display;

    if (!prev.isEmpty())
        display += "<span style='color: #666666;'>" + prev + "</span>\n\n";

    display += currentFormatted;

    ui->textDisplay->setHtml(display);

    int totalCharsInLesson = m_fullText.length();
    int completedChars = 0;
    for (int i = 0; i < m_lineIndex; i++) {
        completedChars += m_lines[i].length();
    }
    completedChars += m_charIndex;

    statusBar()->showMessage(
        QString("Line %1/%2  |  Char %3/%4  |  Completed: %5%%")
            .arg(m_lineIndex + 1)
            .arg(m_lines.size())
            .arg(completedChars)
            .arg(totalCharsInLesson)
            .arg(totalCharsInLesson > 0 ? (completedChars * 100 / totalCharsInLesson) : 0)
        );
}

void MainWindow::updateTimer()
{
    if (!m_isTrainingActive) return;

    qint64 elapsed = m_sessionTimer.elapsed() / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    ui->labelTime->setText(QString("Time: %1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
}

void MainWindow::updateStats()
{
    if (!m_isTrainingActive) return;

    qint64 elapsedSeconds = m_sessionTimer.elapsed() / 1000;
    int cpm = elapsedSeconds > 0 ? (m_totalChars * 60 / elapsedSeconds) : 0;
    ui->labelSpeed->setText(QString("Speed: %1 CPM").arg(cpm));

    int accuracy = m_totalChars > 0 ? (m_correctChars * 100 / m_totalChars) : 100;
    ui->labelAccuracy->setText(QString("Accuracy: %1%").arg(accuracy));
}

void MainWindow::checkTrainingComplete()
{
    if (!m_isTrainingActive) return;

    bool isComplete = (m_lineIndex >= m_lines.size() - 1 &&
                       m_charIndex >= currentLine().length());

    if (isComplete || (m_lineIndex >= m_lines.size())) {
        showResults();
    }
}

void MainWindow::showResults()
{
    m_isTrainingActive = false;
    m_timer->stop();

    qint64 elapsedSeconds = m_sessionTimer.elapsed() / 1000;
    int minutes = elapsedSeconds / 60;
    int seconds = elapsedSeconds % 60;
    QString timeStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));

    int cpm = elapsedSeconds > 0 ? (m_totalChars * 60 / elapsedSeconds) : 0;
    int accuracy = m_totalChars > 0 ? (m_correctChars * 100 / m_totalChars) : 100;

    if (ui->resultTime)
        ui->resultTime->setText(timeStr);
    if (ui->resultSpeed)
        ui->resultSpeed->setText(QString("%1 CPM").arg(cpm));
    if (ui->resultAccuracy)
        ui->resultAccuracy->setText(QString("%1%").arg(accuracy));

    if (ui->stackedWidget)
        ui->stackedWidget->setCurrentWidget(ui->pageResults);
}

void MainWindow::highlightKey(const QString &key)
{
    QList<QPushButton*> allKeys = ui->virtualKeyboard->findChildren<QPushButton*>();
    for (QPushButton *btn : allKeys) {
        btn->setProperty("active", false);
        btn->style()->polish(btn);
    }

    QString keyName = key.toUpper();
    QPushButton *keyButton = nullptr;

    if (keyName == " ") {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_space");
    }
    else if (keyName == "\t" || keyName == "TAB") {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_tab");
    }
    else if (keyName == "\n" || keyName == "ENTER" || keyName == "\r") {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_enter");
    }
    else if (keyName == "\b" || keyName == "BACKSPACE") {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_backspace");
    }
    else if (keyName == "0") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn11");
    else if (keyName == "1") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn2");
    else if (keyName == "2") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn3");
    else if (keyName == "3") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn4");
    else if (keyName == "4") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn5");
    else if (keyName == "5") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn6");
    else if (keyName == "6") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn7");
    else if (keyName == "7") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn8");
    else if (keyName == "8") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn9");
    else if (keyName == "9") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn10");
    else if (keyName == "-") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn12");
    else if (keyName == "=") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn13");
    else if (keyName == "[") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_opening_bracket");
    else if (keyName == "]") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_closing_bracket");
    else if (keyName == ";") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_colon");
    else if (keyName == ",") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_virgule");
    else if (keyName == ".") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_full_stop");
    else if (keyName == "/") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_slash");
    else {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_" + keyName.toLower());
    }

    if (keyButton) {
        keyButton->setProperty("active", true);
        keyButton->style()->polish(keyButton);

        QTimer::singleShot(150, [keyButton]() {
            keyButton->setProperty("active", false);
            keyButton->style()->polish(keyButton);
        });
    }
}

void MainWindow::markKeyPressed(const QString &key, bool correct)
{
    QString keyName = key.toUpper();
    QPushButton *keyButton = nullptr;

    if (keyName == " ") {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_space");
    }
    else if (keyName == "0") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn11");
    else if (keyName == "1") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn2");
    else if (keyName == "2") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn3");
    else if (keyName == "3") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn4");
    else if (keyName == "4") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn5");
    else if (keyName == "5") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn6");
    else if (keyName == "6") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn7");
    else if (keyName == "7") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn8");
    else if (keyName == "8") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn9");
    else if (keyName == "9") keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn10");
    else {
        keyButton = ui->virtualKeyboard->findChild<QPushButton*>("btn_" + keyName.toLower());
    }

    if (keyButton) {
        QString originalStyle = keyButton->styleSheet();

        if (correct) {
            keyButton->setStyleSheet("background-color: #4caf50; color: white; border: 2px solid #2e7d32;");
        } else {
            keyButton->setStyleSheet("background-color: #f44336; color: white; border: 2px solid #c62828;");
        }

        QTimer::singleShot(200, [keyButton, originalStyle]() {
            keyButton->setStyleSheet(originalStyle);
            keyButton->style()->polish(keyButton);
        });
    }
}
