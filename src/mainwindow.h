#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QString>
#include <QMap>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    
    // Override to ensure consistent window behavior
    QSize sizeHint() const override { return QSize(800, 600); }

private slots:
    void onBrowseClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onClearClicked();
    void onShortcutSelected(QListWidgetItem *item);
    void onSudoToggled(bool checked);
    void onBackgroundToggled(bool checked);
    void onOpenEndedToggled(bool checked);
    void refreshShortcuts();
    void updateCommandPreview();

private:
    void setupUi();
    void loadShortcuts();
    void loadShortcut(const QString &name);
    void clearFields();
    void showStatusMessage(const QString &message, int timeout = 3000);
    void setupDarkTheme();
    void setupIcons();
    void firstRunSetup();
    bool isValidShortcutName(const QString &name);
    
    Ui::MainWindow *ui;
    QString currentShortcut;
    
    struct CommandOptions {
        bool useSudo = false;
        bool runInBackground = false;
        bool openEnded = false;
    } commandOptions;
    
    static constexpr const char* SHORTCUT_DIR = "/usr/local/bin";
};

#endif // MAINWINDOW_H
