#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QUuid>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStyleFactory>
#include <QScrollBar>
#include <QStandardPaths>
#include <QSettings>
#include <QPainter>
#include <QTemporaryFile>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentShortcut()
{
    // Set window properties first
    setWindowTitle(tr("Shortcut Manager"));
    
    // Setup UI before setting window flags
    ui->setupUi(this);
    
    // Apply dark theme
    setupDarkTheme();
    
    // Setup icons
    setupIcons();
    
    // Set window attributes after UI is set up
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(sizeHint());
    
    // Connect signals and slots
    connect(ui->browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshShortcuts);
    connect(ui->shortcutList, &QListWidget::itemClicked, this, &MainWindow::onShortcutSelected);
    
    // Connect checkboxes
    connect(ui->sudoCheckBox, &QCheckBox::toggled, this, &MainWindow::onSudoToggled);
    connect(ui->backgroundCheckBox, &QCheckBox::toggled, this, &MainWindow::onBackgroundToggled);
    connect(ui->openEndedCheckBox, &QCheckBox::toggled, this, &MainWindow::onOpenEndedToggled);
    
    // Connect command edit field changes to update preview and handle sudo auto-detection
    connect(ui->commandEdit, &QLineEdit::textChanged, this, [this]() {
        QString command = ui->commandEdit->text().trimmed();
        // Auto-check sudo checkbox if command starts with sudo
        if (command.startsWith("sudo ") && !ui->sudoCheckBox->isChecked()) {
            ui->sudoCheckBox->setChecked(true);
        }
        updateCommandPreview();
    });
    
    // Connect additional checkbox signals
    connect(ui->sudoCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCommandPreview);
    connect(ui->backgroundCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCommandPreview);
    connect(ui->openEndedCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCommandPreview);
    
    // Set size policy to prevent unwanted resizing
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // Load shortcuts
    refreshShortcuts();
    
    // Initial clear of fields
    clearFields();

    // Perform first-run setup if needed
    firstRunSetup();
    
    // Ensure the window is properly updated
    adjustSize();
}

void MainWindow::firstRunSetup()
{
    QSettings settings("0hex01", "Shorts");
    if (settings.value("firstRunComplete", false).toBool()) {
        qDebug() << "First run setup already completed";
        return; // Setup has already been done
    }

    qDebug() << "Running first-time setup...";
    
    // Get the application path
    QString appPath = QCoreApplication::applicationFilePath();
    
    // Create the installation script
    QString script = R"(
#!/bin/bash
# Create necessary directories
mkdir -p ~/.local/share/icons/hicolor/256x256/apps
mkdir -p ~/.local/share/applications

# Copy the application icon
cp "%1" ~/.local/share/icons/hicolor/256x256/apps/shorts.png

# Create .desktop file
cat > ~/.local/share/applications/shorts.desktop <<EOL
[Desktop Entry]
Type=Application
Name=Shortcut Manager
Comment=Create and manage command-line shortcuts
Exec="%2" %U
Icon=shorts
Terminal=false
Categories=Utility;System;
StartupWMClass=shorts
EOL

# Update icon cache
gtk-update-icon-cache -f -t ~/.local/share/icons/hicolor

# Make the .desktop file executable
chmod +x ~/.local/share/applications/shorts.desktop
    )";
    
    // Replace placeholders with actual paths
    script = script.arg("/root/CascadeProjects/projects/shorts/icons/shorts.png")
                 .arg(appPath);
    
    // Create a temporary script file
    QString scriptPath = QDir::temp().filePath("shorts_install.sh");
    QFile scriptFile(scriptPath);
    
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << script;
        scriptFile.close();
        
        // Make the script executable
        scriptFile.setPermissions(QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
        
        qDebug() << "Running installation script:" << scriptPath;
        
        // Run the script
        QProcess process;
        process.start("bash", {scriptPath});
        if (!process.waitForFinished(30000)) { // 30 second timeout
            qWarning() << "Installation script timed out";
            scriptFile.remove();
            return;
        }
        
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            qWarning() << "Installation script failed with code" << process.exitCode()
                      << "and error:" << process.readAllStandardError();
            scriptFile.remove();
            return;
        }
        
        // Clean up the script file
        scriptFile.remove();
        
        qDebug() << "First-time setup completed successfully";
        settings.setValue("firstRunComplete", true);
    } else {
        qWarning() << "Failed to create installation script";
    }

    // All setup is now handled by the script
    qDebug() << "Icon and .desktop file installation completed";
    
    // Clean up temporary files if they exist
    QDir tempDir = QDir::temp();
    if (tempDir.exists("shorts_icons")) {
        QDir(tempDir.filePath("shorts_icons")).removeRecursively();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDarkTheme()
{
    // Set style
    qApp->setStyle(QStyleFactory::create("Fusion"));
    
    // Set palette for dark theme
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(18, 18, 18));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    qApp->setPalette(darkPalette);
    
    // Set stylesheet for additional styling
    QString styleSheet = R"(
        QMainWindow, QDialog, QWidget {
            background-color: #1e1e1e;
            color: #ffffff;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 12px;
        }
        
        QLabel, QCheckBox, QGroupBox {
            color: #a0a0a0;
            font-size: 13px;
            font-weight: normal;
        }
        
        QLineEdit, QListWidget, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            padding: 6px 8px;
            selection-background-color: #3daee9;
            selection-color: #ffffff;
        }
        
        QPushButton {
            background-color: #3a3a3a;
            color: #ffffff;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 6px 12px;
            min-width: 80px;
        }
        
        QPushButton:hover {
            background-color: #4a4a4a;
            border: 1px solid #5a5a5a;
        }
        
        QPushButton:pressed {
            background-color: #2a2a2a;
        }
        
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #5a5a5a;
        }
        
        QListWidget {
            background-color: #252525;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            padding: 2px;
            outline: none;
        }
        
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3d3d3d;
            color: #ffffff;
        }
        
        QListWidget::item:selected {
            background-color: #3daee9;
            color: #000000;
            font-weight: 500;
        }
        
        QListWidget::item:hover:!selected {
            background-color: #3d3d3d;
        }
        
        QScrollBar:vertical {
            background: #252525;
            width: 10px;
            margin: 0px;
        }
        
        QScrollBar::handle:vertical {
            background: #3d3d3d;
            min-height: 20px;
            border-radius: 5px;
        }
    )";
    
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::setupIcons()
{
    // List of possible icon paths to try
    QStringList iconPaths = {
        ":/icons/shorts.png",  // Embedded resource
        QDir::homePath() + "/.local/share/icons/hicolor/256x256/apps/shorts.png",  // Installed in user's home
        "/root/CascadeProjects/projects/shorts/icons/shorts.png",  // Absolute path in project
        "icons/shorts.png"  // Relative path in project
    };
    
    QIcon appIcon;
    QString iconPath;
    
    // Try each path until we find a valid icon
    for (const QString &path : iconPaths) {
        if (QFile::exists(path)) {
            appIcon = QIcon(path);
            if (!appIcon.isNull() && !appIcon.availableSizes().isEmpty()) {
                iconPath = path;
                qDebug() << "Loaded icon from:" << path;
                break;
            }
        }
    }
    
    // If no icon found, create a fallback
    if (appIcon.isNull()) {
        qWarning() << "No valid icon found, creating fallback";
        QPixmap pix(64, 64);
        pix.fill(Qt::blue);
        QPainter painter(&pix);
        painter.setPen(Qt::white);
        painter.drawText(pix.rect(), Qt::AlignCenter, "S");
        appIcon = QIcon(pix);
    }
    
    // Set the application icon
    QApplication::setWindowIcon(appIcon);
    
    // Set the window icon
    setWindowIcon(appIcon);
    
    // Force update the window
    setWindowTitle(windowTitle());
    
    qDebug() << "Icon set successfully from:" << (iconPath.isEmpty() ? "fallback" : iconPath);
    qDebug() << "Available icon sizes:" << appIcon.availableSizes();
    
    // Refresh the window
    update();
}

void MainWindow::onBrowseClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select Executable"),
        QDir::homePath(),
        tr("Executable Files (*.sh *.py *);;All Files (*)")
    );
    
    if (!filePath.isEmpty()) {
        ui->commandEdit->setText('"' + filePath + '"');
    }
}

bool MainWindow::isValidShortcutName(const QString &name)
{
    // Check if name is empty
    if (name.isEmpty()) {
        return false;
    }
    
    // Check for invalid characters (only allow alphanumeric, underscore, and hyphen)
    static QRegularExpression regex("^[a-zA-Z0-9_-]+$");
    if (!regex.match(name).hasMatch()) {
        return false;
    }
    
    // Check if name is a reserved name
    static const QStringList reservedNames = {"..", ".", "/", ""};
    if (reservedNames.contains(name)) {
        return false;
    }
    
    return true;
}

void MainWindow::onSaveClicked()
{
    QString name = ui->nameEdit->text().trimmed();
    QString command = ui->commandEdit->text().trimmed();
    
    if (name.isEmpty() || command.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Name and command are required!"));
        return;
    }
    
    // Validate the shortcut name
    if (!isValidShortcutName(name)) {
        QMessageBox::warning(this, tr("Invalid Name"), 
            tr("Shortcut name can only contain letters, numbers, underscores, and hyphens.\n"
               "It cannot be empty or contain spaces or special characters."));
        return;
    }
    
    // Create the shortcut file path
    QString shortcutPath = QString("%1/%2").arg(SHORTCUT_DIR, name);
    
    // Check if the shortcut already exists
    QFileInfo existingFile(shortcutPath);
    if (existingFile.exists() && existingFile.fileName() != currentShortcut) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Overwrite Shortcut"),
            tr("A shortcut named '%1' already exists. Do you want to overwrite it?").arg(name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply != QMessageBox::Yes) {
            return; // User chose not to overwrite
        }
    }
    
    // Check if the directory exists and is writable
    QDir dir(SHORTCUT_DIR);
    if (!dir.exists()) {
        QMessageBox::critical(this, tr("Error"), 
            tr("The shortcuts directory does not exist. Please create %1 and ensure it's writable.")
            .arg(SHORTCUT_DIR));
        return;
    }
    
    // Check if we have write permission
    QFileInfo dirInfo(SHORTCUT_DIR);
    if (!dirInfo.isWritable()) {
        // Try to use pkexec to gain root privileges
        QProcess process;
        QStringList args;
        args << "--disable-internal-agent" << "--user" << "root" << "--" << "/bin/sh" << "-c" 
             << QString("mkdir -p %1 && chmod 755 %1").arg(SHORTCUT_DIR);
        
        process.start("pkexec", args);
        if (!process.waitForFinished(5000)) {
            QMessageBox::critical(this, tr("Error"), 
                tr("Failed to create or set permissions on the shortcuts directory.\n"
                   "Please ensure you have the necessary permissions to write to %1.")
                .arg(SHORTCUT_DIR));
            return;
        }
    }
    
    // Create the script content with header comments
    QString scriptContent = "#!/bin/bash\n";
    scriptContent += "# Shortcut created with Shorts -- Shortcut Manager Gui\n";
    scriptContent += "# Created by 0hex01 (Michael McClure)\n";
    scriptContent += "# Feel free to copy, manipulate, and distribute Shorts and shortcuts created with shorts\n";
    scriptContent += "# This shortcut comes with no Guarantees or Warranties, use at your own risk\n";
    scriptContent += "# shortcut command is below this line\n\n";
    
    // Add nohup if background mode is enabled
    if (commandOptions.runInBackground) {
        scriptContent += "nohup ";
    }
    
    // Check if command already contains sudo
    bool hasSudo = command.contains("sudo ");
    
    // Only add sudo if the checkbox is checked and the command doesn't already have sudo
    if (commandOptions.useSudo && !hasSudo) {
        scriptContent += "sudo ";
    }
    
    // Add the command exactly as entered
    scriptContent += command;
    
    if (commandOptions.openEnded) {
        scriptContent += " \$@";
    }
    
    if (commandOptions.runInBackground) {
        scriptContent += " &";
    }
    
    scriptContent += "\n";
    
    // Create a temporary file in /tmp with a random name
    QString tempPath = QString("/tmp/shortcut_%1.sh").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QFile tempFile(tempPath);
    
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to create temporary file: %1").arg(tempFile.errorString()));
        return;
    }
    
    // Write the script content to the temporary file
    QTextStream out(&tempFile);
    out << scriptContent;
    tempFile.close();
    
    // Set executable permissions on the temp file
    tempFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                           QFile::ReadGroup | QFile::ExeGroup | 
                           QFile::ReadOther | QFile::ExeOther);
    
    // Create the target directory if it doesn't exist
    QDir().mkpath(SHORTCUT_DIR);
    
    // Use pkexec to copy the file with root privileges
    QProcess process;
    
    // Create a shell script to perform the copy and chmod
    QString scriptPath = "/tmp/shortcut_install_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sh";
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to create installation script."));
        tempFile.remove();
        return;
    }
    
    scriptFile.write(QString("#!/bin/bash\n").toUtf8());
    scriptFile.write(QString("cp -f \"%1\" \"%2\" && chmod 755 \"%2\"\n").arg(tempPath).arg(shortcutPath).toUtf8());
    scriptFile.close();
    
    // Make the script executable
    scriptFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                             QFile::ReadGroup | QFile::ExeGroup | 
                             QFile::ReadOther | QFile::ExeOther);
    
    // Run the script with pkexec
    process.start("pkexec", {"--disable-internal-agent", scriptPath});
    
    // Wait for the process to finish with a longer timeout
    if (!process.waitForFinished(10000)) { // 10 second timeout
        QMessageBox::critical(this, tr("Error"), 
            tr("Timeout while trying to save shortcut with root privileges."));
        tempFile.remove();
        scriptFile.remove();
        return;
    }
    
    // Clean up the script file
    scriptFile.remove();
    
    // Clean up the temp file
    tempFile.remove();
    
    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to save shortcut. Error: %1").arg(errorOutput));
        return;
    }
    
    showStatusMessage(tr("Shortcut '%1' saved successfully!").arg(name));
    refreshShortcuts();
    clearFields();
}

void MainWindow::onBackgroundToggled(bool checked)
{
    commandOptions.runInBackground = checked;
}

void MainWindow::onClearClicked()
{
    clearFields();
    showStatusMessage(tr("Form cleared"));
}

void MainWindow::onDeleteClicked()
{
    if (currentShortcut.isEmpty()) {
        return;
    }

    // Create a confirmation dialog
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Delete Shortcut"),
                                tr("Are you sure you want to delete the shortcut '%1'?").arg(currentShortcut),
                                QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString shortcutPath = QString("%1/%2").arg(SHORTCUT_DIR, currentShortcut);
        QFile file(shortcutPath);
        
        if (file.exists()) {
            if (QFile::remove(shortcutPath)) {
                showStatusMessage(tr("Shortcut '%1' deleted").arg(currentShortcut));
                refreshShortcuts();
                clearFields();
            } else {
                QMessageBox::critical(this, tr("Error"), 
                                    tr("Failed to delete shortcut '%1'. Make sure you have the necessary permissions.")
                                    .arg(currentShortcut));
            }
        } else {
            QMessageBox::warning(this, tr("Not Found"),
                               tr("The shortcut file '%1' was not found.").arg(currentShortcut));
        }
    } else {
        showStatusMessage(tr("Deletion cancelled"));
    }
}

void MainWindow::onShortcutSelected(QListWidgetItem *item)
{
    if (item) {
        loadShortcut(item->text());
    }
}

void MainWindow::onSudoToggled(bool checked)
{
    commandOptions.useSudo = checked;
    updateCommandPreview();
}

void MainWindow::refreshShortcuts()
{
    QDir dir(SHORTCUT_DIR);
    if (!dir.exists()) {
        showStatusMessage(tr("Shortcuts directory does not exist: %1").arg(SHORTCUT_DIR));
        return;
    }
    
    ui->shortcutList->clear();
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Executable | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &file : files) {
        if (!file.fileName().startsWith('.')) {
            ui->shortcutList->addItem(file.fileName());
        }
    }
    
    ui->shortcutList->sortItems();
    
    if (ui->shortcutList->count() > 0) {
        ui->shortcutList->setCurrentRow(0);
        loadShortcut(ui->shortcutList->currentItem()->text());
    }
}

void MainWindow::onOpenEndedToggled(bool checked)
{
    commandOptions.openEnded = checked;
    updateCommandPreview();
}

void MainWindow::updateCommandPreview()
{
    QString command = ui->commandEdit->text().trimmed();
    
    if (command.isEmpty()) {
        ui->previewEdit->clear();
        return;
    }
    
    // Check if command starts with sudo
    bool startsWithSudo = command.startsWith("sudo ");
    
    // If command starts with sudo, make sure the checkbox is checked
    if (startsWithSudo) {
        ui->sudoCheckBox->setChecked(true);
        command = command.mid(5).trimmed(); // Remove sudo from the command
    }
    
    QString preview;
    
    // Add sudo if the checkbox is checked
    if (ui->sudoCheckBox->isChecked()) {
        preview += "sudo ";
    }
    
    // Add nohup if running in background
    if (ui->backgroundCheckBox->isChecked()) {
        preview += "nohup ";
    }
    
    // Add the command (without sudo)
    preview += command;
    
    // Add $@ if open-ended
    if (ui->openEndedCheckBox->isChecked()) {
        preview += " $@";
    }
    
    // Add & if running in background
    if (ui->backgroundCheckBox->isChecked()) {
        preview += " &";
    }
    
    // Update the preview
    ui->previewEdit->setText(preview);
}

void MainWindow::loadShortcut(const QString &name)
{
    if (name.isEmpty()) {
        return;
    }
    
    QString shortcutPath = QString("%1/%2").arg(SHORTCUT_DIR, name);
    QFile file(shortcutPath);
    
    if (!file.exists()) {
        showStatusMessage(tr("Shortcut not found: %1").arg(name));
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showStatusMessage(tr("Cannot open shortcut: %1").arg(name));
        return;
    }
    
    // Read the file content
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    // Set the current shortcut
    currentShortcut = name;
    
    // Update UI
    ui->nameEdit->setText(name);
    
    // Parse the command (get the last non-empty, non-comment line)
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    QString command;
    
    for (int i = lines.size() - 1; i >= 0; --i) {
        QString line = lines[i].trimmed();
        if (!line.isEmpty() && !line.startsWith('#')) {
            command = line;
            break;
        }
    }
    
    // Remove shebang if present
    if (command.startsWith("#!")) {
        command = command.section(' ', 1);
    }
    
    // Parse options without modifying the command
    commandOptions.useSudo = command.contains("sudo ");
    commandOptions.runInBackground = command.contains("nohup ") || command.endsWith(" &");
    commandOptions.openEnded = command.contains("$@") || command.contains("\"$@\"");
    
    // Update UI with the original command
    ui->commandEdit->setText(command);
    ui->sudoCheckBox->setChecked(commandOptions.useSudo);
    ui->backgroundCheckBox->setChecked(commandOptions.runInBackground);
    ui->openEndedCheckBox->setChecked(commandOptions.openEnded);
    ui->deleteButton->setEnabled(true);
    
    // Update the command preview
    updateCommandPreview();
    
    showStatusMessage(tr("Loaded shortcut: %1").arg(name));
}

void MainWindow::clearFields()
{
    ui->nameEdit->clear();
    ui->commandEdit->clear();
    ui->sudoCheckBox->setChecked(false);
    ui->backgroundCheckBox->setChecked(false);
    ui->openEndedCheckBox->setChecked(false);
    ui->deleteButton->setEnabled(false);
    currentShortcut.clear();
    
    // Reset command options
    commandOptions = CommandOptions();
    
    // Clear the preview
    ui->previewEdit->clear();
}

void MainWindow::showStatusMessage(const QString &message, int timeout)
{
    statusBar()->showMessage(message, timeout);
}
