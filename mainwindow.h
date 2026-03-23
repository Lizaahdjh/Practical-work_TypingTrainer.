#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QElapsedTimer>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void highlightKey(const QString &key);
    void markKeyPressed(const QString &key, bool correct);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_btnStartTraining_clicked();
    void on_btnRestartTraining_clicked();
    void on_btnReturnToMain_clicked();

    void on_actionExit_triggered();
    void on_actionAbout_triggered();

    void on_comboLesson_currentIndexChanged(int index);
    void on_btnRandom_clicked();
    void on_btnReload_clicked();

    void updateTimer();
    void updateStats();

    void onVirtualKeyPressed();

private:
    Ui::MainWindow *ui;

    QString     m_fullText;
    QStringList m_lines;
    int         m_lineIndex;
    int         m_charIndex;

    int         m_totalChars;
    int         m_correctChars;
    int         m_errorCount;
    QElapsedTimer m_sessionTimer;
    QTimer*     m_timer;
    bool        m_isTrainingActive;

    int         m_lastCharCount;
    QElapsedTimer m_speedTimer;

    QStringList m_lessonTexts;
    QString     m_lessonsDir;

    void setupInitialData();
    void updateLessonDescription(const QString &lesson);
    void loadLesson(int index);
    void scanLessons();
    void loadLessonFromFile(const QString &path);
    void chooseRandomLesson();
    void setupVirtualKeyboard();

    QString previousLine()   const;
    QString currentLine()    const;
    QString doneFragment()   const;
    QString remainFragment() const;
    QString getFormattedCurrentLine() const;
    void    updateTrainingDisplay();
    void    resetTraining();
    void    checkTrainingComplete();
    void    processKeyInput(const QString &keyText);
    void    handleBackspace();
    void    handleEnter();
    void    showResults();
};

#endif // MAINWINDOW_H
