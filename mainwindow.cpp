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
#include <QMenuBar>
#include <QAction>

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
    , m_settings("TypingTrainer", "TypingTrainer")
    , m_speedUnit("CPM")
    , m_currentTheme("light")
    , m_actionCPM(nullptr)
    , m_actionWPM(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("TypingTrainer");

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTimer);

    installEventFilter(this);
    this->setFocusPolicy(Qt::StrongFocus);

    setupVirtualKeyboard();
    createSettingsMenu();

    setupInitialData();
    loadSettings();

    applyTheme(m_currentTheme);
    updateSpeedUnitMenu();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::createSettingsMenu()
{
    QMenu *settingsMenu = ui->menubar->findChild<QMenu*>("menuSettings");
    if (!settingsMenu) return;

    QList<QAction*> existingActions = settingsMenu->actions();
    for (QAction *action : existingActions) {
        if (action->objectName() == "actionSpeed_Unit" ||
            action->objectName() == "actionLight_Theme" ||
            action->objectName() == "actionDark_Theme") {
            settingsMenu->removeAction(action);
        }
    }

    QMenu *speedMenu = settingsMenu->addMenu("Speed Unit");

    m_actionCPM = speedMenu->addAction("CPM (characters per minute)");
    m_actionCPM->setCheckable(true);
    m_actionCPM->setChecked(m_speedUnit == "CPM");
    connect(m_actionCPM, &QAction::triggered, this, &MainWindow::onCPMTriggered);

    m_actionWPM = speedMenu->addAction("WPM (words per minute, 5 chars = 1 word)");
    m_actionWPM->setCheckable(true);
    m_actionWPM->setChecked(m_speedUnit == "WPM");
    connect(m_actionWPM, &QAction::triggered, this, &MainWindow::onWPMTriggered);

    settingsMenu->addSeparator();

    QAction *lightAction = settingsMenu->addAction("Light Theme");
    lightAction->setCheckable(true);
    lightAction->setChecked(m_currentTheme == "light");
    connect(lightAction, &QAction::triggered, this, &MainWindow::onLightThemeTriggered);

    QAction *darkAction = settingsMenu->addAction("Dark Theme");
    darkAction->setCheckable(true);
    darkAction->setChecked(m_currentTheme == "dark");
    connect(darkAction, &QAction::triggered, this, &MainWindow::onDarkThemeTriggered);
}

void MainWindow::onCPMTriggered()
{
    m_speedUnit = "CPM";
    updateSpeedUnitMenu();
    saveSettings();
    updateSpeedDisplay();
    statusBar()->showMessage("Speed unit: CPM (characters per minute)", 2000);
}

void MainWindow::onWPMTriggered()
{
    m_speedUnit = "WPM";
    updateSpeedUnitMenu();
    saveSettings();
    updateSpeedDisplay();
    statusBar()->showMessage("Speed unit: WPM (words per minute, 5 chars = 1 word)", 2000);
}

void MainWindow::updateSpeedUnitMenu()
{
    if (m_actionCPM) m_actionCPM->setChecked(m_speedUnit == "CPM");
    if (m_actionWPM) m_actionWPM->setChecked(m_speedUnit == "WPM");
}

double MainWindow::calculateCPM() const
{
    qint64 elapsedSeconds = m_sessionTimer.elapsed() / 1000;
    if (elapsedSeconds == 0 || m_totalChars == 0) return 0.0;
    return (double)m_totalChars * 60.0 / elapsedSeconds;
}

double MainWindow::calculateWPM() const
{
    int words = m_totalChars / 5;
    qint64 elapsedSeconds = m_sessionTimer.elapsed() / 1000;
    if (elapsedSeconds == 0 || words == 0) return 0.0;
    return (double)words * 60.0 / elapsedSeconds;
}

double MainWindow::calculateAccuracy() const
{
    if (m_totalChars == 0) return 100.0;
    return (double)m_correctChars * 100.0 / m_totalChars;
}

void MainWindow::updateSpeedDisplay()
{
    if (!m_isTrainingActive) return;

    double speed = (m_speedUnit == "CPM") ? calculateCPM() : calculateWPM();
    QString speedText = QString("%1 %2").arg((int)speed).arg(m_speedUnit);
    ui->labelSpeed->setText("Speed: " + speedText);
}

QString MainWindow::getLightThemeStyle()
{
    return R"(
        QMainWindow { background-color: #f5f5f5; }
        QMenuBar { background-color: #ffffff; border-bottom: 2px solid #cccccc; }
        QMenuBar::item { padding: 4px 8px; }
        QMenuBar::item:selected { background-color: #e0e0e0; }
        QMenu { background-color: #ffffff; border: 1px solid #cccccc; }
        QMenu::item:selected { background-color: #e0e0e0; }

        #labelLessonDescription {
            font-size: 18px; font-weight: bold; color: #2c3e50;
            background-color: #ecf0f1; padding: 12px;
            border-left: 5px solid #e74c3c; border-radius: 4px;
        }
        QComboBox { background-color: #ffffff; border: 1px solid #cccccc; border-radius: 4px; padding: 6px; }
        QPushButton { background-color: #4a90e2; color: #ffffff; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; }
        QPushButton:hover { background-color: #357abd; }
        #btnReturnToMain { background-color: #95a5a6; }

        #labelTime, #labelSpeed, #labelAccuracy {
            background-color: #ffffff; border: 1px solid #e0e0e0; border-radius: 4px;
            padding: 8px 15px; font-size: 14px; font-weight: bold; color: #333333;
        }
        #labelTime { border-left: 3px solid #4a90e2; }
        #labelSpeed { border-left: 3px solid #27ae60; }
        #labelAccuracy { border-left: 3px solid #e67e22; }

        #textDisplay {
            background-color: #ffffff; border: 2px solid #e0e0e0; border-radius: 6px;
            padding: 15px; font-family: 'Courier New', monospace; font-size: 18px;
        }

        #virtualKeyboard { background-color: #f0f0f0; border: 1px solid #cccccc; border-radius: 8px; }
        #virtualKeyboard QPushButton {
            background-color: #ffffff; border: 1px solid #bdbdbd; border-radius: 4px;
            padding: 8px; font-size: 14px; font-weight: bold; color: #333333;
            min-width: 40px; min-height: 40px;
        }
        #virtualKeyboard QPushButton:hover { background-color: #e0e0e0; }
        #btn_space { min-width: 200px; background-color: #e8e8e8; }
        #btn_enter { background-color: #27ae60; color: white; }
        #btn_backspace { background-color: #e67e22; color: white; }

        #label { font-size: 30px; font-weight: bold; color: #2c3e50; border-bottom: 4px solid #e74c3c; }
        #resultTime, #resultSpeed, #resultAccuracy { font-size: 20px; font-weight: bold; color: #4a90e2; }

        QProgressBar { border: 1px solid #cccccc; border-radius: 4px; background-color: #f0f0f0; }
        QProgressBar::chunk { background-color: #27ae60; border-radius: 3px; }
    )";
}

QString MainWindow::getDarkThemeStyle()
{
    return R"(
        QMainWindow { background-color: #1e1e1e; }
        QMenuBar { background-color: #2d2d2d; border-bottom: 2px solid #3d3d3d; color: #ffffff; }
        QMenuBar::item { color: #ffffff; }
        QMenuBar::item:selected { background-color: #3d3d3d; }
        QMenu { background-color: #2d2d2d; border: 1px solid #3d3d3d; color: #ffffff; }
        QMenu::item:selected { background-color: #3d3d3d; }

        #labelLessonDescription {
            font-size: 18px; font-weight: bold; color: #ecf0f1;
            background-color: #2c3e50; padding: 12px;
            border-left: 5px solid #e74c3c; border-radius: 4px;
        }
        QComboBox { background-color: #2d2d2d; border: 1px solid #3d3d3d; border-radius: 4px; color: #ffffff; }
        QPushButton { background-color: #4a90e2; color: #ffffff; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; }
        #btnReturnToMain { background-color: #5a6b6c; }

        #labelTime, #labelSpeed, #labelAccuracy {
            background-color: #2d2d2d; border: 1px solid #3d3d3d; border-radius: 4px;
            padding: 8px 15px; font-size: 14px; font-weight: bold; color: #ffffff;
        }

        #textDisplay {
            background-color: #2d2d2d; border: 2px solid #3d3d3d; border-radius: 6px;
            padding: 15px; font-family: 'Courier New', monospace; font-size: 18px; color: #ffffff;
        }

        #virtualKeyboard { background-color: #2d2d2d; border: 1px solid #3d3d3d; border-radius: 8px; }
        #virtualKeyboard QPushButton {
            background-color: #3d3d3d; border: 1px solid #4d4d4d; border-radius: 4px;
            padding: 8px; font-size: 14px; font-weight: bold; color: #ffffff;
            min-width: 40px; min-height: 40px;
        }
        #virtualKeyboard QPushButton:hover { background-color: #4d4d4d; }
        #btn_space { min-width: 200px; background-color: #4d4d4d; }

        #label { font-size: 30px; font-weight: bold; color: #ecf0f1; border-bottom: 4px solid #e74c3c; }
        #resultTime, #resultSpeed, #resultAccuracy { font-size: 20px; font-weight: bold; color: #4a90e2; }

        QLabel { color: #ffffff; }
        QProgressBar { border: 1px solid #3d3d3d; border-radius: 4px; background-color: #2d2d2d; color: #ffffff; }
        QProgressBar::chunk { background-color: #27ae60; border-radius: 3px; }
    )";
}

void MainWindow::applyTheme(const QString &theme)
{
    if (theme == "dark") {
        qApp->setStyleSheet(getDarkThemeStyle());
        m_currentTheme = "dark";
    } else {
        qApp->setStyleSheet(getLightThemeStyle());
        m_currentTheme = "light";
    }

    QMenu *settingsMenu = ui->menubar->findChild<QMenu*>("menuSettings");
    if (settingsMenu) {
        QList<QAction*> actions = settingsMenu->actions();
        for (QAction *action : actions) {
            if (action->text() == "Light Theme") action->setChecked(theme == "light");
            if (action->text() == "Dark Theme") action->setChecked(theme == "dark");
        }
    }

    saveSettings();
}

void MainWindow::onLightThemeTriggered()
{
    applyTheme("light");
    statusBar()->showMessage("Light theme applied", 2000);
}

void MainWindow::onDarkThemeTriggered()
{
    applyTheme("dark");
    statusBar()->showMessage("Dark theme applied", 2000);
}

void MainWindow::loadSettings()
{
    QString lastLesson = m_settings.value("lastLesson", "").toString();
    if (!lastLesson.isEmpty() && ui->comboLesson) {
        for (int i = 0; i < ui->comboLesson->count(); ++i) {
            if (ui->comboLesson->itemData(i).toString() == lastLesson) {
                ui->comboLesson->setCurrentIndex(i);
                break;
            }
        }
    }

    m_speedUnit = m_settings.value("speedUnit", "CPM").toString();

    m_currentTheme = m_settings.value("theme", "light").toString();

    qDebug() << "Settings loaded: speedUnit=" << m_speedUnit << "theme=" << m_currentTheme;
}

void MainWindow::saveSettings()
{
    if (ui->comboLesson && ui->comboLesson->currentIndex() >= 0) {
        QString currentLesson = ui->comboLesson->currentData().toString();
        m_settings.setValue("lastLesson", currentLesson);
    }

    m_settings.setValue("speedUnit", m_speedUnit);

    m_settings.setValue("theme", m_currentTheme);

    qDebug() << "Settings saved: speedUnit=" << m_speedUnit << "theme=" << m_currentTheme;
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

    statusBar()->showMessage("Found " + QString::number(files.size()) + " lesson(s).");
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
                                 "\n\nReason: " + file.errorString());
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
            "  |  " + QString::number(m_fullText.length()) + " chars");
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
    Q_UNUSED(index);
}

void MainWindow::on_comboLesson_currentIndexChanged(int index)
{
    if (!ui->comboLesson || index < 0) return;

    QVariant data = ui->comboLesson->itemData(index);
    if (data.isValid()) {
        QString path = data.toString();
        if (!path.isEmpty()) {
            loadLessonFromFile(path);
            saveSettings();
        }
    }
}

void MainWindow::on_btnRandom_clicked()
{
    chooseRandomLesson();
    saveSettings();
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

void MainWindow::updateTimer()
{
    if (!m_isTrainingActive) return;

    qint64 elapsed = m_sessionTimer.elapsed() / 1000;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    ui->labelTime->setText(QString("Time: %1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));

    updateSpeedDisplay();
}

void MainWindow::updateStats()
{
    if (!m_isTrainingActive) return;

    int accuracy = (int)calculateAccuracy();
    ui->labelAccuracy->setText(QString("Accuracy: %1%").arg(accuracy));

    updateSpeedDisplay();
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

    int cpm = (int)calculateCPM();
    int accuracy = (int)calculateAccuracy();

    double speed = (m_speedUnit == "CPM") ? cpm : (m_totalChars / 5 * 60 / (elapsedSeconds > 0 ? elapsedSeconds : 1));
    QString speedText = QString("%1 %2").arg((int)speed).arg(m_speedUnit);

    if (ui->resultTime)
        ui->resultTime->setText(timeStr);
    if (ui->resultSpeed)
        ui->resultSpeed->setText(speedText);
    if (ui->resultAccuracy)
        ui->resultAccuracy->setText(QString("%1%").arg(accuracy));

    if (ui->stackedWidget)
        ui->stackedWidget->setCurrentWidget(ui->pageResults);
}

void MainWindow::on_actionExit_triggered()
{
    saveSettings();
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v1.0\n\n"
                       "Practical Works #6–10\n\n"
                       "Features:\n"
                       "- Load lessons from .txt files\n"
                       "- Keyboard input with visual feedback\n"
                       "- Virtual keyboard highlighting\n"
                       "- Speed tracking (CPM/WPM)\n"
                       "- Accuracy tracking\n"
                       "- Light/Dark themes\n"
                       "- Settings persistence\n\n"
                       "Place .txt lesson files in the 'lessons/' folder");
}

void MainWindow::updateLessonDescription(const QString &lesson)
{
    Q_UNUSED(lesson);
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
