#include "mainwindow.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QProcess>
#include <QMessageBox>
#include <unistd.h>
#include <QStandardPaths>
#include <QDir>

bool isRunningAsRoot() {
    return geteuid() == 0;
}

bool restartWithPrivileges() {
    QString appPath = QCoreApplication::applicationFilePath();
    QProcess process;
    
    // First try pkexec (preferred as it shows a GUI password prompt)
    process.start("which", {"pkexec"});
    process.waitForFinished();
    if (process.exitCode() == 0) {
        QStringList args = QCoreApplication::arguments();
        args.removeFirst(); // Remove the program name
        return QProcess::startDetached("pkexec", QStringList() << appPath << args);
    }
    
    // Fall back to sudo if pkexec is not available
    process.start("which", {"sudo"});
    process.waitForFinished();
    if (process.exitCode() == 0) {
        QStringList args = QCoreApplication::arguments();
        args.prepend(appPath);
        return QProcess::startDetached("sudo", args);
    }
    
    return false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties for better window manager integration
    app.setApplicationName("shorts");  // Single word for WM_CLASS
    app.setApplicationDisplayName("shorts");
    app.setOrganizationName("Windsurf");
    app.setApplicationVersion("1.0");
    
    // Check if running as root
    if (!isRunningAsRoot()) {
        // If not root, try to restart with sudo or pkexec
        if (restartWithPrivileges()) {
            return 0; // Successfully restarted with privileges
        } else {
            QMessageBox::critical(nullptr, "Permission Required",
                                "This application requires root privileges to manage system shortcuts.\n\n"
                                "Please run with:\n"
                                "  pkexec " + QCoreApplication::applicationName() + "\n\n"
                                "or\n\n"
                                "  sudo " + QCoreApplication::applicationFilePath());
            return 1;
        }
    }
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Set window icon with fallback
    QIcon appIcon = QIcon::fromTheme("shortcut-manager");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/icon_64x64.png");
    }
    app.setWindowIcon(appIcon);
    
    MainWindow window;
    
    // Set window properties for better panel integration
    window.setWindowTitle("shorts");
    window.setWindowIcon(appIcon);
    window.setWindowFlags(window.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    window.show();
    
    return app.exec();
}
