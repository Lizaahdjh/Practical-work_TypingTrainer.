#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <QStringList>

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

private slots:
    void on_btnStartTraining_clicked();
    void on_btnRestartTraining_clicked();
    void on_btnReturnToMain_clicked();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void on_comboLesson_currentTextChanged(const QString &text);

    void on_btnNextChar_clicked();

private:
    Ui::MainWindow *ui;

    QString     m_fullText;
    QStringList m_lines;
    int         m_lineIndex;
    int         m_charIndex;

    QStringList m_lessonTexts;

    void setupInitialData();
    void updateLessonDescription(const QString &lesson);
    void loadLesson(int index);

    QString previousLine()  const;
    QString currentLine()   const;
    QString doneFragment()  const;
    QString remainFragment() const;

    void updateTrainingDisplay();
};

#endif // MAINWINDOW_H
